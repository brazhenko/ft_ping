#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/ip_icmp.h>
#include <ctype.h>
#include "ping.h"
#include <fcntl.h>
#include <limits.h>
#include <byteswap.h>

// getpid
// getuid
// getaddrinfo
// gettimeofday
// inet_ntop		struct in_addr -> 192.168.0.1
// inet_pton 		192.168.0.1 -> struct in_addr
// setsockopt
// recvmsg
// sendto
// socket

/// USED:
// exit
// signal
// alarm
// *printf


ping_context_t ping_ctx = {};

void interrupt(int param) {
	printf("exit: %d\n", param);
	exit(EXIT_SUCCESS);
}

int sock_;
struct addrinfo* add_;

static uint16_t ipv4_checksum(const uint16_t *words, int wordcount) {
	uint32_t tmp = 0;

	for (int i = 0; i< wordcount*2; i++) {
		for (int j = 7; j > -1; j--) {
			printf(((unsigned char*)words)[i] >> j & 1 ? "1" : "0");
		}
		printf("|");

	}

//	0100 0101 | 0000 0000
//	0000 0000 | 0101 0100
//	0000 0000 | 0000 0001
//	0000 0000 | 0000 0000
//	0100 0000 | 0000 0001
//	0000 0000 | 0000 0000
//	1100 0011 | 1000 0101
//	1110 1111 | 0101 0011
//	0101 0111 | 1111 1010
//	1111 1010 | 1111 0010
//	0111 0100 1110 0010
//  0111 0100 1110 0010

	for (int i = 0; i < wordcount; i++) {
		int a = __bswap_16(words[i]);
		tmp += a;
		tmp += (tmp >> 16);
		tmp &= UINT16_MAX;
	}

	return (tmp) ^ UINT16_MAX;
}

void ping() {
	printf("pinging...\n");

	size_t sz = sizeof (struct ip) + sizeof (struct icmphdr) + ping_ctx.payload_size;

	char arr[sz];
	memset(arr, 0, sz);


	struct ip *ip_header = (struct ip *)arr;
	ip_header->ip_v = 4;
	ip_header->ip_hl = 5;
	ip_header->ip_tos = 0;
	ip_header->ip_len = __bswap_16(sz);
	ip_header->ip_id = __bswap_16(1);
	ip_header->ip_off = 0; //??
	ip_header->ip_ttl = ping_ctx.ttl;
	ip_header->ip_p = IPPROTO_ICMP;



	struct in_addr addr;
	if (inet_pton(AF_INET, "195.133.239.83", &addr) < 0) {
		perror("my address");
		exit(EXIT_FAILURE);
	}
	ip_header->ip_src = addr;
	if (inet_pton(AF_INET, "87.250.250.242", &addr) < 0) {
		perror("dest address");
		exit(EXIT_FAILURE);
	}

	ip_header->ip_dst = addr;

	ip_header->ip_sum = ipv4_checksum((uint16_t *)ip_header, sizeof(*ip_header) / 2);

	printf("%d\n", ip_header->ip_sum);

	struct icmp *icmp_header = (struct icmp *)(arr + sizeof (struct ip));
	icmp_header->icmp_type = ICMP_ECHO;
	icmp_header->icmp_code = 0;
	icmp_header->icmp_cksum = 0;
	icmp_header->icmp_seq = htons(1);



	struct sockaddr_in dest;
	memset(&dest, 0, sizeof dest);

	dest.sin_addr.s_addr = ((struct sockaddr_in *) add_->ai_addr)->sin_addr.s_addr;
	dest.sin_family = AF_INET;

	char *payload_ptr = (char *)(arr + sizeof (struct iphdr) + sizeof (struct icmphdr));
	memset(payload_ptr, 0x42, ping_ctx.payload_size);

	printf("size: %ld\n", sz);

	char tmpb[] = "\x45\x00\x00\x54\x32\xf2\x40\x00\x40\x01\x08\xb7\xac\x11\x00\x02"
				"\x57\xfa\xfa\xf2\x08\x00\xde\xa6\x0e\x28\x00\x01\xdd\x74\xdb\x60"
				"\x00\x00\x00\x00\x8c\x87\x07\x00\x00\x00\x00\x00\x10\x11\x12\x13"
				"\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23"
				"\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33"
				"\x34\x35\x36\x37";

//	int ret = sendto(sock_, arr, sz, 0, &dest, sizeof(dest));
	int ret = sendto(sock_, tmpb, sz, 0, &dest, sizeof(dest));

	if (ret < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}

	printf("ret: %d\n", ret);

	alarm(5);
}

void pong(int sock) {
	char			buffer[512];
	ssize_t			ret;

	struct iovec	io = {
		.iov_base = buffer,
		.iov_len = sizeof buffer
	};
	struct msghdr	msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = &io,
		.msg_iovlen = 1,
		.msg_control = buffer,
		.msg_controllen = sizeof(buffer),
		.msg_flags = 0
	};

	while (true) {
		// reading answer...
		ret = recvmsg(sock, &msg, 0);

		if (ret < 0) {
			perror("Holy shit!");
			exit(EXIT_FAILURE);
		}
		else if (ret == 0) {
			perror("Connection closed");
		}
		else {
			printf("Something arrived!\n");
		}
	}
}

struct addrinfo* lookup_host (const char *host)
{
	struct addrinfo hints, *res, *result;
	int errcode;
	char addrstr[100];
	void *ptr;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_flags |= AI_CANONNAME;

	errcode = getaddrinfo(host, NULL, &hints, &result);
	if (errcode != 0) {
		perror ("getaddrinfo");
		exit(EXIT_FAILURE);
	}

	res = result;

	printf ("Host: %s\n", host);
	if (res) {
		switch (res->ai_family)
		{
			case AF_INET:
				ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
				break;
			case AF_INET6:
				ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
				break;
		}
		inet_ntop(res->ai_family, ptr, addrstr, 100);
		printf("IPv%d address: %s %s\n", res->ai_family == PF_INET6 ? 6 : 4, addrstr, res->ai_canonname);

		res = res->ai_next;
	}
	else {
		printf("No host...\n");
		exit(EXIT_FAILURE);
	}

	return res;
}


void set_default_args() {
	// Set default payload size
	ping_ctx.payload_size = 56;

	// Set default system ttl of packets
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
	ping_ctx.ttl = atoi(arr);


}

void parse_argv(int argc, char **argv) {
	set_default_args();

	int c;
	opterr = 0;

	while ((c = getopt(argc, argv, PING_AVL_FLAGS)) != -1)
		switch (c)
		{
		case PING_VERBOSE:
			ping_ctx.flags[c] = true;
			break;
		case PING_HELP:
			// print_help_message
			exit(EXIT_SUCCESS);
		case PING_PACKET_SZ:
			ping_ctx.flags[c] = true;
			ping_ctx.payload_size = atoi(optarg);
			if (!(PING_MIN_PAYLOAD_SZ <= ping_ctx.payload_size && ping_ctx.payload_size <= PING_MAX_PAYLOAD_SZ)) {
				fprintf(stderr,
						"%s: invalid argument: '%s': out of range: %d <= value <= %d",
						argv[0], optarg, PING_MIN_PAYLOAD_SZ, PING_MAX_PAYLOAD_SZ);
				exit(EXIT_FAILURE);
			}
			break;
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
		fprintf(stderr, "%s: usage error: Destination address required", argv[0]);
		exit(EXIT_FAILURE);
	}

	ping_ctx.dest = argv[optind];

	// Dump argv data
	for (int i = 0; i < 256; i++) {
		if (ping_ctx.flags[i]) {
			printf("%c", i);
		}
	}
	printf("\n");
	printf("destination: %s\n", ping_ctx.dest);
	printf("ttl: %d\n", ping_ctx.ttl);
	printf("payloadsize: %d\n", ping_ctx.payload_size);
}

int main(int argc, char **argv) {
	parse_argv(argc, argv);


	if (signal(SIGALRM, ping) == SIG_ERR) {
		perror("alarm error");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, interrupt) == SIG_ERR) {
		perror("interruption error");
		exit(EXIT_FAILURE);
	}

	// filling message
	printf("header size: %zu\n", sizeof (struct iphdr));
	struct icmphdr icmp_header;

	icmp_header.type = ICMP_ECHO;
	icmp_header.code = 0;





	struct addrinfo* adr = lookup_host(ping_ctx.dest);


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



	sock_ = icmp_sock;
	add_ = adr;


	ping();
	pong(icmp_sock);
}
