#include "ping.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

// getuid

extern ping_context_t ping_ctx;

void print_iphdr(struct iphdr *ip)
{
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src          Dst\n");
    printf(" %1x  %1x  %02x %04x %04x",
            ip->version, ip->ihl, ip->tos, ip->tot_len, ip->id);
    printf("   %1x %04x", ((ip->frag_off) & 0xe000) >> 13,
            (ip->frag_off) & 0x1fff);
    printf("  %02x  %02x %04x", ip->ttl, ip->protocol, ip->check);
    printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->saddr));
    printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->daddr));
    printf("\n");
}

void ping() {
    while (true) {
        if (send_icmp_msg_v4(
                ping_ctx.icmp_sock,
                getpid(),
                ping_ctx.ttl,
                ICMP_ECHO,
                ping_ctx.messages_sent + 1,
                ping_ctx.payload_size,
                ((struct sockaddr_in *)ping_ctx.src_addr_info->ai_addr)->sin_addr.s_addr,
                ping_ctx.dest_addr
        ) != 0) {

            perror("cannot send echo");
            exit(EXIT_FAILURE);

        }

        ping_ctx.messages_sent++;
        sleep(ping_ctx.interval_between_echoes);
    }
}

void pong() {
    char            buffer[512];
    ssize_t         ret;
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
        ret = recvmsg(ping_ctx.icmp_sock, &msg, 0);

        if (ret < 0) {
            perror("Holy shit!");
            exit(EXIT_FAILURE);
        }
        else if (ret == 0) {
            perror("Connection closed");
        }
        if (ret < ip_hdr_size + icmp_hdr_size) {
            fprintf(stderr,
                    "Something impossible happened,"
                    "packet is too small, ignoring\n");
            continue;
        }

        // Get current time for different needs
        if (gettimeofday(&current_time, NULL) != 0) {
            perror("cannot get time");
            exit(EXIT_FAILURE);
        }

        // Clear output print buffer...
        memset(output, 0, sizeof output);

        if (ping_ctx.flags[PING_TIMESTAMP_PREF]) {
            sprintf(output + strlen(output), "[%zu.%06zu] ", current_time.tv_sec, current_time.tv_usec);
        }

        // Parse response msg
        struct ip* ip_hdr = (struct ip*)buffer;
        struct icmp *icmp_hdr = (struct icmp*)(buffer + ip_hdr_size);

        in_addr_t sender_ip = ip_hdr->ip_src.s_addr;

        if (icmp_hdr->icmp_type == ICMP_ECHOREPLY) {
            // Good echo-reply received
            sprintf(output + strlen(output), "%ld bytes ", ntohs(ip_hdr->ip_len) - sizeof (struct iphdr));
            char ip_buffer[64] = {0};
            inet_ntop(AF_INET, &ip_hdr->ip_dst.s_addr, ip_buffer, sizeof ip_buffer);
//        printf("dest computor: %s\n", ip_buffer);

            inet_ntop(AF_INET, &sender_ip, ip_buffer, sizeof ip_buffer);

            if (ping_ctx.flags[PING_NO_DNS_NAME]) {
                // Print out without DNS name
                sprintf(output + strlen(output), "from %s: ", ip_buffer);
            }
            else {
                struct addrinfo hints, *res, *result;
                int errcode;
                char addrstr[100];
                void *ptr;

                memset (&hints, 0, sizeof (hints));
                hints.ai_family = PF_INET;
                hints.ai_flags |= AI_CANONNAME;


                errcode = getaddrinfo(ip_buffer, NULL, &hints, &result);
                if (errcode != 0) {
                    fprintf(stderr, "%s: %s\n", ip_buffer, gai_strerror(errcode));
                    exit(EXIT_FAILURE);
                }


                sprintf(output + strlen(output), "from %s (%s): ", result->ai_canonname, ip_buffer);

                freeaddrinfo(result);
            }


            sprintf(output + strlen(output), "icmp_seq=%d ", ntohs(icmp_hdr->icmp_seq));
            sprintf(output + strlen(output), "ttl=%d ", ip_hdr->ip_ttl);

            if (ret - ip_hdr_size - icmp_hdr_size >= sizeof (struct timeval)) {
                // There is timestamp? Using it!
                char *icmp_payload_ptr = buffer + ip_hdr_size + icmp_hdr_size;
                memcpy(&send_time, icmp_payload_ptr, sizeof send_time);
                const uint64_t trip_time = (current_time.tv_sec - send_time.tv_sec) * 100000 +
                        (current_time.tv_usec - send_time.tv_usec);
                ping_ctx.min_ping_time = min(ping_ctx.min_ping_time, trip_time);
                ping_ctx.max_ping_time = max(ping_ctx.max_ping_time, trip_time);
                ping_ctx.acc_ping_time += trip_time;
                ping_ctx.acc_ping_time2 += (trip_time * trip_time);

                ping_ctx.stats_count++;
                sprintf(output + strlen(output), "time=%ld.%02ld ms",
                        trip_time / 1000, trip_time % 1000 / 10);
            }

            if (ping_ctx.flags[PING_AUDIBLE]) {
                printf("%c", '\a');
                fflush(stdout);
            }
            ping_ctx.message_received++;
            if (ping_ctx.flags[PING_RESPONSE_LIM] && ping_ctx.message_received == ping_ctx.response_count_limit) {
                raise(SIGINT);
            }
        }
        else {
            char ip_buffer[64] = {0};
            inet_ntop(AF_INET, &sender_ip, ip_buffer, sizeof ip_buffer);

            if (ping_ctx.flags[PING_NO_DNS_NAME]) {
                // Print out without DNS name
                sprintf(output + strlen(output), "From %s ", ip_buffer);
            }
            else {
                struct addrinfo hints, *res, *result;
                int errcode;
                char addrstr[100];
                void *ptr;

                memset (&hints, 0, sizeof (hints));
                hints.ai_family = PF_INET;
                hints.ai_flags |= AI_CANONNAME;


                errcode = getaddrinfo(ip_buffer, NULL, &hints, &result);
                if (errcode != 0) {
                    fprintf(stderr, "%s: %s\n", ip_buffer, gai_strerror(errcode));
                    exit(EXIT_FAILURE);
                }
                sprintf(output + strlen(output), "From %s (%s) ", result->ai_canonname, ip_buffer);
                freeaddrinfo(result);
            }

            sprintf(output + strlen(output), "icmp_seq=%d ",
                    ntohs(*(uint16_t*)&buffer[ret - 2]) /* on error icmp_seq number is in the last 2 bytes of payload */);
            if (icmp_hdr->icmp_type == ICMP_TIME_EXCEEDED) {
                sprintf(output + strlen(output), "Time to live exceeded");
            }
            else if (icmp_hdr->icmp_type == ICMP_DEST_UNREACH) {
                sprintf(output + strlen(output), "Destination Unreachable");
            }
            else {
                sprintf(output + strlen(output), "Unknown ICMP return code: %x", icmp_hdr->icmp_seq);
            }
        }
        if (!ping_ctx.flags[PING_QUIET]) {
            printf("%s\n", output);
        }
        if (ping_ctx.flags[PING_VERBOSE] && icmp_hdr->icmp_type != ICMP_ECHOREPLY) {
            print_iphdr((struct iphdr*)ip_hdr);
        }
    }
}

//struct addrinfo* ping_lookup(const char *bin_name, const char *host) {
//    struct addrinfo hints, *res, *result;
//    int errcode;
//    char addrstr[100];
//    void *ptr;
//
//    memset (&hints, 0, sizeof (hints));
//    hints.ai_family = PF_INET;
//    hints.ai_flags |= AI_CANONNAME;
//    errcode = getaddrinfo(host, NULL, &hints, &result);
//    if (errcode != 0) {
//        fprintf(stderr, "%s: %s: %s\n", bin_name, host, gai_strerror(errcode));
//        exit(EXIT_FAILURE);
//    }
//
//    res = result;
//
//    printf ("Host: %s\n", host);
//    if (res) {
//        switch (res->ai_family)
//        {
//            case AF_INET:
//                ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
//                break;
//            case AF_INET6:
//                ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
//                break;
//        }
//        inet_ntop(res->ai_family, ptr, addrstr, 100);
//        printf("IPv%d address: %s %s\n", res->ai_family == PF_INET6 ? 6 : 4, addrstr, res->ai_canonname);
//    }
//    else {
//        fprintf(stderr, "Unknown error\n");
//        exit(EXIT_FAILURE);
//    }
//
//    return res;
//}

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


