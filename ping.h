#ifndef FT_PING_PING_H
# define FT_PING_PING_H

# include <stdbool.h>
# include <stddef.h>
# include <limits.h>
# include <netinet/ip_icmp.h>
#include <netdb.h>

# define PING_AVL_FLAGS         "vhs:ac:Dw:Vi:nqt:"
# define PING_VERBOSE           'v'
# define PING_HELP              'h'
# define PING_PACKET_SZ         's'
# define PING_AUDIBLE           'a'
# define PING_RESPONSE_LIM      'c'
# define PING_TIMESTAMP_PREF    'D'
# define PING_LIFETIME_LIM      'w'
# define PING_VERSION           'V'
# define PING_INTERVAL          'i'
# define PING_NO_DNS_NAME       'n'
# define PING_QUIET             'q'
# define PING_TTL               't'

# define PING_MIN_PAYLOAD_SZ        0
# define PING_MAX_PAYLOAD_SZ        127992
# define PING_TTL_MIN               0
# define PING_TTL_MAX               255
# define PING_INTERVAL_MIN          0
# define PING_INTERVAL_MAX          10000
# define PING_RESPONSE_COUNT_MIN    1
# define PING_RESPONSE_COUNT_MAX    INT_MAX
# define PING_LIFETIME_SEC_MIN      1
# define PING_LEFETIME_SEC_MAX      1000000
# define PING_IPV4_DEFAULT_TTL_PATH "/proc/sys/net/ipv4/ip_default_ttl"
# define PING_VERSION_STR           "ft_ping v0.0.1"
# define MICROSECONDS_IN_SECOND     1000000
# define MICROSECONDS_IN_MILLISECOND    1000
# define MINIMUM_WAIT_MICROSECONDS  10000

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

static const size_t         ip_hdr_size = sizeof (struct iphdr); // Supposed to be 20
static const size_t         icmp_hdr_size = sizeof (struct icmphdr); // Supposed to be 8

struct s_ping_context {
    bool        flags[256];
    const char  *dest;
    char    canon_dest[NI_MAXHOST];
    int     icmp_sock;
    in_addr_t   dest_addr;
    in_addr_t   src_addr;
    int payload_size;
    size_t  response_count_limit;
    size_t  seconds_to_work;
    int interval_between_echoes;
    int ttl;
    size_t      messages_sent;
    size_t      messages_received;
    size_t      error_messages_received;

    struct timeval time_program_started;
    size_t      stats_count;

    // In microseconds
    uint64_t    min_ping_time;
    uint64_t    max_ping_time;
    uint64_t    acc_ping_time;
    uint64_t    acc_ping_time2;
};


typedef struct s_ping_context ping_context_t;

void    initialize_context(int argc, char **argv);
void    initialize_signals();
int send_icmp_msg_v4(
        int sock,
        uint16_t id,
        uint8_t ttl,
        uint8_t icmp_type,
        uint16_t icmp_seq_num,
        size_t payload_size,
        in_addr_t source_ip,
        in_addr_t dest_ip);
int get_ipaddr_by_name(const char *name, in_addr_t *out,
        char *canon_name, size_t canon_name_size);
int get_name_by_ipaddr(in_addr_t ip, char *host, size_t host_len);
__suseconds_t time_diff(struct timeval* begin, struct timeval *end);

void sync_ping();
void sync_pong();

#endif
