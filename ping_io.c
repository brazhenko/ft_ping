#include "ping.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>

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
    printf("\n\n");
}

void sync_ping() {
    char ip_buffer[64] = {0};
    inet_ntop(AF_INET, &ping_ctx.dest_addr, ip_buffer, sizeof ip_buffer);
    printf("PING %s (%s) %d(%lu) bytes of data.\n", ping_ctx.canon_dest,
            ip_buffer,
            ping_ctx.payload_size,
            ping_ctx.payload_size + icmp_hdr_size + ip_hdr_size);

    while (true) {
        if (send_icmp_msg_v4(
                ping_ctx.icmp_sock,
                getpid(),
                ping_ctx.ttl,
                ICMP_ECHO,
                ping_ctx.messages_sent + 1,
                ping_ctx.payload_size,
                ping_ctx.src_addr,
                ping_ctx.dest_addr
        ) != 0) {
            perror("cannot send echo");
            exit(EXIT_FAILURE);
        }

        ping_ctx.messages_sent++;
        if (ping_ctx.interval_between_echoes == 0) {
            usleep(MINIMUM_WAIT_MICROSECONDS);
        }
        else {
            sleep(ping_ctx.interval_between_echoes);
        }
    }
}

void sync_pong() {
    char    output[1024], ip_buffer[64], host_name[NI_MAXHOST], buffer[2048];
    struct timeval  current_time, send_time;
    struct iovec   iov[1];
    struct msghdr  msg;
    ssize_t ret;

    // Init message struct
    memset(&msg, 0, sizeof(msg));
    memset(iov, 0, sizeof(iov));
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof buffer;
    msg.msg_iov     = iov;
    msg.msg_iovlen  = 1;

    while (true) {
        // Read input stream...
        ret = recvmsg(ping_ctx.icmp_sock, &msg, 0);

        // Handle errors
        if (ret < 0) {
            perror("Holy shit!");
            exit(EXIT_FAILURE);
        }
        else if (ret == 0) {
            perror("Connection closed");
        }
        if (ret < ip_hdr_size + icmp_hdr_size) {
            fprintf(stderr, "Something impossible happened,"
                    "packet is too small, ignoring\n");
            continue;
        }

        // Get current time for different needs
        if (gettimeofday(&current_time, NULL) != 0) {
            perror("cannot get time");
            exit(EXIT_FAILURE);
        }

        // Struct pointers
        struct ip* ip_hdr = (struct ip*)buffer;
        struct icmp *icmp_hdr = (struct icmp*)(buffer + ip_hdr_size);

        // Ignore messages which are not for us
        if (ip_hdr->ip_dst.s_addr != ping_ctx.src_addr) {
            continue;
        }

        // Clear output print buffer...
        memset(output, 0, sizeof output);

        // Timestamp if needed
        if (ping_ctx.flags[PING_TIMESTAMP_PREF]) {
            sprintf(output + strlen(output), "[%zu.%06zu] ", current_time.tv_sec, current_time.tv_usec);
        }

        // Parse response msg
        in_addr_t sender_ip = ip_hdr->ip_src.s_addr;

        if (icmp_hdr->icmp_type == ICMP_ECHOREPLY) {
            // Good echo-reply received
            ping_ctx.messages_received++;

            sprintf(output + strlen(output), "%ld bytes ",
                    ntohs(ip_hdr->ip_len) - sizeof (struct iphdr));
            inet_ntop(AF_INET, &sender_ip, ip_buffer, sizeof ip_buffer);

            if (ping_ctx.flags[PING_NO_DNS_NAME]) {
                // Print out without DNS name
                sprintf(output + strlen(output), "from %s: ", ip_buffer);
            }
            else {
                if (get_name_by_ipaddr(sender_ip, host_name, sizeof host_name) != 0) {
                    perror("cannot resolve ip");
                    exit(EXIT_FAILURE);
                }
                sprintf(output + strlen(output), "from %s (%s): ", host_name, ip_buffer);
            }

            sprintf(output + strlen(output), "icmp_seq=%d ", ntohs(icmp_hdr->icmp_seq));
            sprintf(output + strlen(output), "ttl=%d ", ip_hdr->ip_ttl);

            if (ret - ip_hdr_size - icmp_hdr_size >= sizeof (struct timeval)) {
                // There is timestamp? Using it!
                char *icmp_payload_ptr = buffer + ip_hdr_size + icmp_hdr_size;
                memcpy(&send_time, icmp_payload_ptr, sizeof send_time);
                const uint64_t trip_time = time_diff(&send_time, &current_time);
                ping_ctx.min_ping_time = min(ping_ctx.min_ping_time, trip_time);
                ping_ctx.max_ping_time = max(ping_ctx.max_ping_time, trip_time);
                ping_ctx.acc_ping_time += trip_time;
                ping_ctx.acc_ping_time2 += (trip_time * trip_time);

                ping_ctx.stats_count++;
                sprintf(output + strlen(output), "time=%ld.%02ld ms",
                        trip_time / 1000, trip_time % 1000 / 10);
            }

            // Put ASCII 0x07 when good packet received
            if (ping_ctx.flags[PING_AUDIBLE]) {
                printf("%c", '\a');
                fflush(stdout);
            }
        }
        else {
            // Bad echo reply received
            ping_ctx.error_messages_received++;

            inet_ntop(AF_INET, &sender_ip, ip_buffer, sizeof ip_buffer);

            if (ping_ctx.flags[PING_NO_DNS_NAME]) {
                // Print out without DNS name
                sprintf(output + strlen(output), "From %s ", ip_buffer);
            }
            else {
                if (get_name_by_ipaddr(sender_ip, host_name, sizeof host_name) != 0) {
                    perror("cannot resolve ip");
                    exit(EXIT_FAILURE);
                }
                sprintf(output + strlen(output), "From %s (%s) ", host_name, ip_buffer);
            }

            sprintf(output + strlen(output), "icmp_seq=%d ",
                    /* on error icmp_seq number is in the last 2 bytes of payload */
                    ntohs(*(uint16_t*)&buffer[ret - 2]));

            // Append error
            if (icmp_hdr->icmp_type == ICMP_TIME_EXCEEDED)
                sprintf(output + strlen(output), "Time to live exceeded");
            else if (icmp_hdr->icmp_type == ICMP_DEST_UNREACH)
                sprintf(output + strlen(output), "Destination Unreachable");
            else
                sprintf(output + strlen(output), "Unknown ICMP return code: %x", icmp_hdr->icmp_type);
        }
        if (!ping_ctx.flags[PING_QUIET]) {
            printf("%s\n", output);
        }
        if (ping_ctx.flags[PING_VERBOSE] && icmp_hdr->icmp_type != ICMP_ECHOREPLY) {
            print_iphdr((struct iphdr*)ip_hdr);
        }

        // End job if needed
        if (ping_ctx.flags[PING_RESPONSE_LIM]
            && ping_ctx.messages_received + ping_ctx.error_messages_received == ping_ctx.response_count_limit) {
            raise(SIGINT);
        }
    }
}


__suseconds_t time_diff(struct timeval* begin, struct timeval *end) {
    __suseconds_t ret;

    ret = end->tv_sec - begin->tv_sec;
    ret *= MICROSECONDS_IN_SECOND;
    ret += (end->tv_usec - begin->tv_usec);

    return ret;
}