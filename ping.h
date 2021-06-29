#ifndef FT_PING_PING_H
# define FT_PING_PING_H

# include <stdbool.h>

# define PING_AVL_FLAGS	"vhs:"
# define PING_VERBOSE	'v'
# define PING_HELP		'h'
# define PING_PACKET_SZ	's'

# define PING_MIN_PAYLOAD_SZ		0
# define PING_MAX_PAYLOAD_SZ		127992
# define PING_IPV4_DEFAULT_TTL_PATH "/proc/sys/net/ipv4/ip_default_ttl"

struct s_ping_context {
	bool 		flags[256];
	const char 	*dest;
	int 		payload_size;
	int 		ttl;
};

typedef struct s_ping_context ping_context_t;

#endif
