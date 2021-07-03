#ifndef FT_PING_PING_H
# define FT_PING_PING_H

# include <stdbool.h>
# include <stddef.h>
# include <limits.h>

#include <unistd.h>
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

static const size_t         ip_hdr_size = sizeof (struct iphdr); // Supposed to be 20
static const size_t         icmp_hdr_size = sizeof (struct icmphdr); // Supposed to be 8

struct s_ping_context {
    bool        flags[256];
    const char  *dest;
    int         icmp_sock;
    struct addrinfo*    dest_addr_info;
    struct addrinfo*    src_addr_info;
    int         payload_size;
    size_t      response_count_limit;
    size_t      seconds_to_work;
    size_t      interval_between_echoes;
    int         ttl;
};

typedef struct s_ping_context ping_context_t;

void    initialize_context(int argc, char **argv);
void    initialize_signals();

#endif
