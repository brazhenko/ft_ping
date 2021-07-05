#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

static uint16_t ipv4_icmp_checksum(const uint16_t *words, size_t word_count);

/*
 * Function: send_icmp_msg_v4
 * --------------------------
 *      Makes up an icmp_v4 echo message
 *  (https://datatracker.ietf.org/doc/html/rfc792)
 *  and sends to a particular IP-address,
 *
 *  sock - system socket
 *
 *  id - id to put in IPv4 header
 *
 *  ttl - tll for packet
 *
 *  icmp_seq_num - icmp sequence number
 *
 *  payload_size - size of ICMP payload (trailing bytes
 *  of icmp message). Is payload_size >= sizeof (struct timeval)
 *  the struct timeval will be stored in first sizeof (struct timeval)
 *  of payload time of message was sent
 *
 *  returns:    0 - success
 *              1 - error, errno will be set in a particular errcode
 */

int send_icmp_msg_v4(
        int sock,
        uint16_t id,
        uint8_t ttl,
        uint8_t icmp_type,
        uint16_t icmp_seq_num,
        size_t payload_size,
        in_addr_t source_ip,
        in_addr_t dest_ip
        ) {
    static const size_t ip_hdr_size = sizeof (struct iphdr) /* =20 */
        , icmp_hdr_size = sizeof (struct icmphdr); /* =8 */

    const size_t entire_msg_size
        = ip_hdr_size + icmp_hdr_size + payload_size;

    char message[entire_msg_size + 1]; /* God sorry for VLA for message... */
    memset(message, 0, entire_msg_size + 1);

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
    ip_header->ip_dst.s_addr = dest_ip;
    ip_header->ip_src.s_addr = source_ip;
    ip_header->ip_sum = ipv4_icmp_checksum((uint16_t *)ip_header,
            icmp_hdr_size / 2);

    // Filling the ICMP header
    struct icmp *icmp_header = (struct icmp *)(message + ip_hdr_size);
    icmp_header->icmp_type = icmp_type;
    icmp_header->icmp_code = 0;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_id = htons(id);
    icmp_header->icmp_seq = htons(icmp_seq_num);

    // Filling ICMP payload with random data
    char *payload_ptr = (char *)(message + ip_hdr_size + icmp_hdr_size);
    memset(payload_ptr, 0x42, payload_size);

    // Pushing timestamp at the beginning of the payload
    if (payload_size >= sizeof (struct  timeval)) {
        // Only if enough room in buffer
        struct timeval  current_time;
        if (gettimeofday(&current_time, NULL) != 0) {
            return 1;
        }
        memcpy(payload_ptr, &current_time, sizeof current_time);
    }

    // Calculating ICMP checksum after filling payload
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_cksum = ipv4_icmp_checksum((uint16_t *)icmp_header,
            (icmp_hdr_size + payload_size) / 2
                    + ((icmp_hdr_size + payload_size) & 1));

    // Actually send our message
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof dest);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_ip;

    ssize_t ret = sendto(
            sock,
            message,
            entire_msg_size,
            0,
            (struct sockaddr *)&dest,
            sizeof(struct sockaddr_in));

    if (ret < 0) return 1;

    return 0;
}

static uint16_t ipv4_icmp_checksum(const uint16_t *words, size_t word_count) {
    uint32_t acc = 0;

    for (int i = 0; i < word_count; i++) {
        acc += words[i];
        acc += (acc >> 16);
        acc &= UINT16_MAX;
    }

    return (acc ^ UINT16_MAX);
}
