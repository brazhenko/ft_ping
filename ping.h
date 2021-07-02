#ifndef FT_PING_PING_H
# define FT_PING_PING_H

# include <stdbool.h>
# include <stddef.h>

# define PING_AVL_FLAGS	        "vhs:fac:Dw:Vi:nqt:"
# define PING_VERBOSE           'v'
# define PING_HELP              'h'
# define PING_PACKET_SZ         's'
# define PING_FLOOD             'f'
# define PING_AUDIBLE           'a'
# define PING_REPLIES_LIM       'c'
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
# define PING_IPV4_DEFAULT_TTL_PATH "/proc/sys/net/ipv4/ip_default_ttl"
# define PING_VERSION_STR           "ft_ping v0.0.1"

struct s_ping_context {
	bool        flags[256];
	const char  *dest;
    int         icmp_sock;
    struct addrinfo*    addr_info;
	int         payload_size;
	size_t      packet_replies_count;
	size_t      seconds_to_work;
	size_t      seconds_interval;
	int         ttl;
};

typedef struct s_ping_context ping_context_t;

void initialize_context(int argc, char **argv);


#endif
