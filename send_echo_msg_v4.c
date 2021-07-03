#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ping.h"
#include <netdb.h>

static uint16_t ipv4_icmp_checksum(const uint16_t *words, size_t word_count) {
    uint32_t acc = 0;

    for (int i = 0; i < word_count; i++) {
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
        uint16_t icmp_seq_num,
        size_t payload_size,
        in_addr_t source_ip,
        in_addr_t dest_ip) {
    const size_t
            entire_msg_size = ip_hdr_size
            + icmp_hdr_size
            + payload_size;

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
    ip_header->ip_dst.s_addr = source_ip;
    ip_header->ip_src.s_addr = dest_ip;// ((struct sockaddr_in *)ping_ctx.src_addr_info->ai_addr)->sin_addr
    ip_header->ip_sum = ipv4_icmp_checksum((uint16_t *)ip_header,
            icmp_hdr_size / 2);

    // Filling the ICMP header
    struct icmp *icmp_header = (struct icmp *)(message + ip_hdr_size);
    icmp_header->icmp_type = ICMP_ECHO;
    icmp_header->icmp_code = 0;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_id = htons(id);
    icmp_header->icmp_seq = htons(icmp_seq_num);

    // Filling ICMP payload with random data
    char *payload_ptr = (char *)(message + ip_hdr_size + icmp_hdr_size);
    memset(payload_ptr, 0x42, payload_size);

    // Pushing send timestamp at the beginning of the payload
    if (payload_size >= sizeof (struct  timeval)) {
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
            (icmp_hdr_size + payload_size) / 2
                    + ((icmp_hdr_size + payload_size) & 1));


    // Actually send our message
    struct addrinfo dest;
    memset(&dest, 0, sizeof dest);
    ((struct sockaddr_in *)&dest.ai_addr)->sin_family = AF_INET;
    ((struct sockaddr_in *)&dest.ai_addr)->sin_addr.s_addr = dest_ip;

    ssize_t ret = sendto(
            sock,
            message,
            entire_msg_size,
            0,
            (struct sockaddr *)&dest,
            sizeof(struct sockaddr_in));

    if (ret < 0) {
        perror("send");
        return 1;
    }

    return 0;
}