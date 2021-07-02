#include "ping.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>


struct addrinfo* ping_lookup(const char *bin_name, const char *host);
ping_context_t ping_ctx = {};

static void dump_usage(const char *bin_name) {
    fprintf(stderr,
        "\n"
        "Usage\n"
        "  %s [options] <destination>\n"
        "\n"
        "Options:\n"
        "  <destination>      dns name or ip address\n"
        "  -a                 use audible ping\n"
        "  -c <count>         stop after <count> replies\n"
        "  -D                 print timestamps\n"
        "  -f                 flood ping\n"
        "  -h                 print help and exit\n"
        "  -i <interval>      seconds between sending each packet\n"
        "  -n                 no dns name resolution\n"
        "  -q                 quiet output\n"
        "  -s <size>          use <size> as number of data bytes to be sent\n"
        "  -t <ttl>           define time to live\n"
        "  -v                 verbose output\n"
        "  -V                 print version and exit\n"
        "  -w <deadline>      reply wait <deadline> in seconds\n"
        , bin_name);
}

static void dump_version() {
    printf("%s\n", PING_VERSION_STR);
}


static void set_default_args() {
    // Clear up the structure
    memset(&ping_ctx, 0, sizeof ping_ctx);

    // Set default payload size
    ping_ctx.payload_size = 56;

    // Set default ttl
    int ttl_fd = open(PING_IPV4_DEFAULT_TTL_PATH, O_RDONLY);
    char arr[1024] = {0};
    if (ttl_fd == -1) {
        perror("cannot open ttl var file");
        exit(EXIT_FAILURE);
    }
    if (read(ttl_fd, arr,15
            /* some random digit num which is greater than possible ttl*/ ) == -1) {
        perror("cannot read ttl var file");
        exit(EXIT_FAILURE);
    }
    close(ttl_fd);
    ping_ctx.ttl = atoi(arr);

    // Set default interval between echoes
    ping_ctx.interval_between_echoes = 1;
}

void initialize_context(int argc, char **argv) {
    set_default_args();

    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, PING_AVL_FLAGS)) != -1)
        switch (c)
        {
        case PING_QUIET:
        case PING_NO_DNS_NAME:
        case PING_AUDIBLE:
        case PING_FLOOD:
        case PING_VERBOSE:
        case PING_TIMESTAMP_PREF:
            ping_ctx.flags[c] = true;
            break;
        case PING_HELP:
            dump_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case PING_RESPONSE_LIM:
            ping_ctx.flags[c] = true;
            ping_ctx.response_count_limit = atoi(optarg);
            if (!(PING_RESPONSE_COUNT_MIN <= ping_ctx.response_count_limit && ping_ctx.response_count_limit <= PING_RESPONSE_COUNT_MAX)) {
                fprintf(stderr, "%s: invalid argument: '%s': out of range: %d <= value <= %d\n",
                        argv[0], optarg, PING_RESPONSE_COUNT_MIN, PING_RESPONSE_COUNT_MAX);
                exit(EXIT_FAILURE);
            }
            break;
        case PING_LIFETIME_LIM:
            ping_ctx.flags[c] = true;
            ping_ctx.seconds_to_work = atoi(optarg);
            if (!(PING_LIFETIME_SEC_MIN <= ping_ctx.seconds_to_work && ping_ctx.seconds_to_work <= PING_LEFETIME_SEC_MAX)) {
                fprintf(stderr, "%s: invalid argument: '%s': out of range: %d <= value <= %d\n",
                        argv[0], optarg, PING_LIFETIME_SEC_MIN  , PING_LEFETIME_SEC_MAX);
                exit(EXIT_FAILURE);
            }
            break;
        case PING_INTERVAL:
            ping_ctx.flags[c] = true;
            ping_ctx.interval_between_echoes = atoi(optarg);
            if (!(PING_INTERVAL_MIN <= ping_ctx.interval_between_echoes && ping_ctx.interval_between_echoes <= PING_INTERVAL_MAX)) {
                fprintf(stderr, "%s: invalid argument: '%s': out of range: %d <= value <= %d\n",
                        argv[0], optarg, PING_INTERVAL_MIN, PING_INTERVAL_MAX);
                exit(EXIT_FAILURE);
            }
            break;
        case PING_PACKET_SZ:
            ping_ctx.flags[c] = true;
            ping_ctx.payload_size = atoi(optarg);
            if (!(PING_MIN_PAYLOAD_SZ <= ping_ctx.payload_size && ping_ctx.payload_size <= PING_MAX_PAYLOAD_SZ)) {
                fprintf(stderr, "%s: invalid argument: '%s': out of range: %d <= value <= %d\n",
                        argv[0], optarg, PING_MIN_PAYLOAD_SZ, PING_MAX_PAYLOAD_SZ);
                exit(EXIT_FAILURE);
            }
            break;
        case PING_TTL:
            ping_ctx.flags[c] = true;
            ping_ctx.ttl = atoi(optarg);
            if (!(PING_TTL_MIN <= ping_ctx.ttl && ping_ctx.ttl <= PING_TTL_MAX)) {
                fprintf(stderr, "%s: invalid argument: '%s': out of range: %d <= value <= %d\n",
                        argv[0], optarg, PING_TTL_MIN, PING_TTL_MAX);
                exit(EXIT_FAILURE);
            }
            break;
        case PING_VERSION:
            dump_version();
            exit(EXIT_FAILURE);
        case '?':
            if (optopt == 'c') {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if (isprint(optopt)) {
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            }
            else {
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            }
            exit(EXIT_FAILURE);
        default:
            abort ();
        }

    if (optind == argc) {
        // No dest address
        fprintf(stderr, "%s: usage error: Destination address required\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ping_ctx.dest = argv[optind];

    int icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }
    printf("socket: %d\n", icmp_sock);

    if (setsockopt(icmp_sock, IPPROTO_IP, IP_HDRINCL, (int[1]){1}, sizeof(int)) == -1) {
        perror("cannot set sock option");
        exit(EXIT_FAILURE);
    }

    ping_ctx.icmp_sock = icmp_sock;

    ping_ctx.dest_addr_info = ping_lookup(argv[0], ping_ctx.dest);

    char hostname[1024] = {0};
    gethostname(hostname, sizeof hostname - 1);

    struct addrinfo hints, *info, *p;;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags |= AI_CANONNAME;

    int errcode = getaddrinfo(hostname, NULL, &hints, &info);
    if (errcode != 0) {
        perror ("getaddrinfo");
        exit(EXIT_FAILURE);
    }
    ping_ctx.src_addr_info = info;

    for(p = info; p != NULL; p = p->ai_next) {
        printf("hostname: %s\n", p->ai_canonname);
    }

    // Dump argv data
    for (int i = 0; i < 256; i++) {
        if (ping_ctx.flags[i]) {
            printf("%c", i);
        }
    }
    printf("\n");

}
