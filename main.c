#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/ip_icmp.h>
#include "ping.h"
#include <byteswap.h>
#include <pthread.h>
#include <sys/time.h>

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


extern ping_context_t ping_ctx;


static uint16_t ipv4_icmp_checksum(const uint16_t *words, size_t wordcount) {
    uint32_t acc = 0;

    for (int i = 0; i < wordcount; i++) {
        acc += words[i];
        acc += (acc >> 16);
        acc &= UINT16_MAX;
    }

    return (acc ^ UINT16_MAX);
}

int send_echo_msg_v4(
        int sock,
        uint16_t id,
        uint8_t ttl,
        uint16_t icmp_seq_num) {

    const size_t
            entire_msg_size = ip_hdr_size
            + icmp_hdr_size
            + ping_ctx.payload_size;

    char message[entire_msg_size + 1]; // God sorry for VLA...
    memset(message, 0, entire_msg_size + 1); // Clear buffer

    // Filling the IP header
    struct ip *ip_header = (struct ip *)message;
    ip_header->ip_v = 4;
    ip_header->ip_hl = 5;
    ip_header->ip_tos = 0;
    ip_header->ip_len = htons(entire_msg_size);
    ip_header->ip_id = id;
    ip_header->ip_off = 0; //??
    ip_header->ip_ttl = ttl;
    ip_header->ip_p = IPPROTO_ICMP;
    ip_header->ip_dst = ((struct sockaddr_in *)ping_ctx.dest_addr_info->ai_addr)->sin_addr;
    ip_header->ip_src = ((struct sockaddr_in *)ping_ctx.src_addr_info->ai_addr)->sin_addr;
    ip_header->ip_sum = ipv4_icmp_checksum((uint16_t *)ip_header,
            icmp_hdr_size / 2);

    // Filling the ICMP header
    struct icmp *icmp_header = (struct icmp *)(message + ip_hdr_size);
    icmp_header->icmp_type = ICMP_ECHO;
    icmp_header->icmp_code = 0;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_id = __bswap_16(id);
    icmp_header->icmp_seq = __bswap_16(icmp_seq_num);

    // Filling ICMP payload with random data
    char *payload_ptr = (char *)(message + ip_hdr_size + icmp_hdr_size);
    memset(payload_ptr, 0x42, ping_ctx.payload_size);

    // Pushing send timestamp at the beginning of the payload
    if (ping_ctx.payload_size >= sizeof (struct  timeval)) {
        // Only if enough room in buffer
        struct timeval  current_time;
        if (gettimeofday(&current_time, NULL) != 0) {
            perror("cannot get time");
            exit(EXIT_FAILURE);
        }
        memcpy(payload_ptr, &current_time, sizeof current_time);
    }


    // Calculating ICMP checksum after filling payload
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_cksum = ipv4_icmp_checksum((uint16_t *)icmp_header,
            (icmp_hdr_size + ping_ctx.payload_size) / 2
            + ((icmp_hdr_size + ping_ctx.payload_size) & 1));

    // Actually send our message
    ssize_t ret = sendto(
            sock,
            message,
            entire_msg_size,
            0,
            (struct sockaddr *)ping_ctx.dest_addr_info,
            sizeof(struct sockaddr_in));

    if (ret < 0) {
        perror("send");
        return 1;
    }

    return 0;
}

void ping() {
    int seq_number = 1;

    while (true) {
        if (send_echo_msg_v4(ping_ctx.icmp_sock, getpid(), ping_ctx.ttl, seq_number) != 0) {
            exit(EXIT_FAILURE);
        }

        seq_number++;
        sleep(ping_ctx.interval_between_echoes);
    }
}

void pong() {
    char            buffer[512];
    ssize_t            ret;
    int             response_count = 0;
    struct timeval  current_time, send_time;
    char            output[1024];

    struct iovec    io = {
        .iov_base = buffer,
        .iov_len = sizeof buffer
    };
    struct msghdr    msg = {
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
        ret = recvmsg(ping_ctx.icmp_sock, &msg, 0);

        if (ret < 0) {
            perror("Holy shit!");
            exit(EXIT_FAILURE);
        }
        else if (ret == 0) {
            perror("Connection closed");
        }
//        if (ret < MIN_ICMP_MSG_SIZE) {
//              govno1;
//        }

        // Get current time for different needs
        if (gettimeofday(&current_time, NULL) != 0) {
            perror("cannot get time");
            exit(EXIT_FAILURE);
        }


        // Clear output print buffer...
        memset(output, 0, sizeof output);

        if (ping_ctx.flags[PING_TIMESTAMP_PREF]) {
            sprintf(output, "[%zu.%06zu] ", current_time.tv_sec, current_time.tv_usec);
        }

        // Parse response msg
        struct ip* ip_hdr = (struct ip*)buffer;
        struct icmp *icmp_hdr = (struct icmp*)(&buffer[0] + ip_hdr_size);

        sprintf(output + strlen(output), "%ld bytes ", ntohs(ip_hdr->ip_len) - sizeof (struct iphdr));
        sprintf(output + strlen(output), "from %s (%s): ", "YA.RU", "87.250.250.242"); // TODO сделать
        sprintf(output + strlen(output), "icmp_seq=%d ", ntohs(icmp_hdr->icmp_seq));
        sprintf(output + strlen(output), "ttl=%d ", ip_hdr->ip_ttl);

        if (ret - ip_hdr_size - icmp_hdr_size >= sizeof (struct timeval)) {
            // There is timestamp? Using it!
            char *icmp_payload_ptr = buffer + ip_hdr_size + icmp_hdr_size;
            memcpy(&send_time, icmp_payload_ptr, sizeof send_time);
            const uint64_t mcrsec_delta = (current_time.tv_sec - send_time.tv_sec) * 100000 +
                    (current_time.tv_usec - send_time.tv_usec);
            sprintf(output + strlen(output), "time=%ld.%ld ms",
                    mcrsec_delta / 1000, mcrsec_delta % 1000 / 10);
        }

        if (ping_ctx.flags[PING_AUDIBLE]) {
            printf("%c", '\a');
            fflush(stdout);
        }
        response_count++;
        if (ping_ctx.flags[PING_RESPONSE_LIM] && response_count == ping_ctx.response_count_limit) {
            raise(SIGINT);
        }
        
        if (!ping_ctx.flags[PING_QUIET]) {
            printf("%s\n", output);
        }
    }
}



struct addrinfo* ping_lookup(const char *bin_name, const char *host)
{
    struct addrinfo hints, *res, *result;
    int errcode;
    char addrstr[100];
    void *ptr;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_INET;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo(host, NULL, &hints, &result);

    if (errcode != 0) {
        fprintf(stderr, "%s: %s: %s\n", bin_name, host, gai_strerror(errcode));
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
    }
    else {
        fprintf(stderr, "Unknown error\n");
        exit(EXIT_FAILURE);
    }

    return res;
}


int main(int argc, char **argv) {
    initialize_context(argc, argv);
    initialize_signals();

    // Detached pinger
    pthread_t thread;
    if (pthread_create(&thread, NULL, (void *(*)(void *))ping, NULL) != 0) {
        perror("cannot create thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_detach(thread) != 0) {
        perror("cannot detach thread");
        exit(EXIT_FAILURE);
    }

    // Response listener
    pong();
}

//void pr_iph(struct iphdr *ip)
//{
//    int hlen;
//    u_char *cp;
//
//    hlen = ip->ihl << 2;
//    cp = (u_char *)ip + 20;        /* point to options */
//
//    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
//    printf(" %1x  %1x  %02x %04x %04x",
//            ip->version, ip->ihl, ip->tos, ip->tot_len, ip->id);
//    printf("   %1x %04x", ((ip->frag_off) & 0xe000) >> 13,
//            (ip->frag_off) & 0x1fff);
//    printf("  %02x  %02x %04x", ip->ttl, ip->protocol, ip->check);
//    printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->saddr));
//    printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->daddr));
//    printf("\n");
//    pr_options(cp, hlen);
//}