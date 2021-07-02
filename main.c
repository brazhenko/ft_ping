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


extern ping_context_t ping_ctx;

void interrupt(int param) {
	printf("exit: %d\n", param);
	exit(EXIT_SUCCESS);
}

static uint16_t ipv4_icmp_checksum(const uint16_t *words, int wordcount) {
	uint32_t tmp = 0;

	for (int i = 0; i < wordcount; i++) {
		int a = (words[i]);
		tmp += a;
		tmp += (tmp >> 16);
		tmp &= UINT16_MAX;
	}

	return (tmp ^ UINT16_MAX);
}

void ping() {
	printf("pinging...\n");

	size_t sz = sizeof (struct ip) + sizeof (struct icmphdr) + ping_ctx.payload_size;

    printf("ip: %zu, icmp: %zu\n", sizeof (struct ip), sizeof (struct icmphdr));

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

	ip_header->ip_sum = ipv4_icmp_checksum((uint16_t *)ip_header,
            sizeof(*ip_header) / 2);

	printf("%d\n", ip_header->ip_sum);

	struct icmp *icmp_header = (struct icmp *)(arr + sizeof (struct ip));
	icmp_header->icmp_type = ICMP_ECHO;
	icmp_header->icmp_code = 0;
	icmp_header->icmp_cksum = 0;
	icmp_header->icmp_seq = htons(1);


	struct sockaddr_in dest;
	memset(&dest, 0, sizeof dest);

	dest.sin_addr.s_addr = ((struct sockaddr_in *)ping_ctx.addr_info->ai_addr)->sin_addr.s_addr;
	dest.sin_family = AF_INET;

	char *payload_ptr = (char *)(arr + sizeof (struct iphdr) + sizeof (struct icmphdr));
	memset(payload_ptr, 0x42, ping_ctx.payload_size);

	printf("size: %ld\n", sz);


    //  0x0000:  45 00 00 54    00 01 00 00 40 01 74 e2  c3 85 ef 53
    //  0x0010:  57 fa fa f2    08 00 00 00 00 00 00 01  42 42 42 42
    //  0x0020:  42 42 42 42    42 42 42 42 42 42 42 42  42 42 42 42
    //  0x0030:  42 42 42 42    42 42 42 42 42 42 42 42  42 42 42 42
    //  0x0040:  42 42 42 42    42 42 42 42 42 42 42 42  42 42 42 42
    //  0x0050:  42 42 42 42

	// Original
	//  0x0000: [45 00 00 54    32 f2 40 00 40 01 08 b7  ac 11 00 02
    //	0x0010:  57 fa fa f2]  [08 00 de a6 0e 28 00 01] dd 74 db 60
    //	0x0020:  00 00 00 00    8c 87 07 00 00 00 00 00  10 11 12 13
    //	0x0030:  14 15 16 17    18 19 1a 1b 1c 1d 1e 1f  20 21 22 23
    //	0x0040:  24 25 26 27    28 29 2a 2b 2c 2d 2e 2f  30 31 32 33
    //	0x0050:  34 35 36 37


	char tmpb[] = "\x45\x00\x00\x54\x32\xf2\x40\x00\x40\x01\x08\xb7\xac\x11\x00\x02"
				"\x57\xfa\xfa\xf2\x08\x00\xde\xa6\x0e\x28\x00\x01\x42\x42\x42\x42"
                "\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42"
                "\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42"
                "\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42\x42"
                "\x42\x42\x42\x42"
//                "\x57\xfa\xfa\xf2\x08\x00\xde\xa6\x0e\x28\x00\x01\xdd\x74\xdb\x60"
//				"\x00\x00\x00\x00\x8c\x87\x07\x00\x00\x00\x00\x00\x10\x11\x12\x13"
//				"\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23"
//				"\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33"
//				"\x34\x35\x36\x37"
				;

    char tmp28[] = "\x45\x00\x00\x1c\x51\x92\x40\x00\x40\x01"
                   "\xea\x4e" // чексумма

                   "\xac\x11\x00\x02\x57\xfa\xfa\xf2"


                   "\x08\x00\xf4\x8b\x03\x6f\x00\x05";

    char tmp30[] = "\x45\x00\x00\x1e\x6f\x9b\x40\x00\x40\x01\xcc\x43\xac\x11\x00\x02\x57\xfa\xfa\xf2"

                   "\x08\x00"
                   "\xed\xfc" // чексумма
                   "\x0a\x00\x00\x02"
                   "\x00\x01";


    memcpy(arr, tmpb, (sizeof (struct iphdr) + sizeof (struct icmphdr) + ping_ctx.payload_size));

    printf("ip checksum: %u\n", ip_header->ip_sum);
    ip_header->ip_sum = 0;

    ip_header->ip_sum = ipv4_icmp_checksum((uint16_t *)ip_header,
            sizeof *ip_header / 2);
    printf("ip checksum: %u\n", ip_header->ip_sum);

    printf("icmp checksum: %u\n", icmp_header->icmp_cksum);
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_cksum = ipv4_icmp_checksum((uint16_t *)icmp_header,
            ((sizeof(struct icmphdr)) + ping_ctx.payload_size) / 2);
    printf("icmp checksum: %u\n", icmp_header->icmp_cksum);
    printf("icmp size: %zu\n", sizeof (struct icmphdr));

    size_t ret = sendto(ping_ctx.icmp_sock, arr, (sizeof (struct iphdr) + sizeof (struct icmphdr) + ping_ctx.payload_size), 0, (struct sockaddr *)&dest, sizeof(dest));
//	size_t ret = sendto(sock_, tmpb, sz, 0, (struct sockaddr *)&dest, sizeof(dest));


	if (ret < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}

	printf("ret: %zu\n", ret);

	alarm(5);
}

void pong() {
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
		ret = recvmsg(ping_ctx.icmp_sock, &msg, 0);

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
	hints.ai_flags |= AI_CANONNAME;

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

int main(int argc, char **argv) {
    initialize_context(argc, argv);

	if (signal(SIGALRM, ping) == SIG_ERR) {
		perror("signal alarm error");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, interrupt) == SIG_ERR) {
		perror("signal interrupt error");
		exit(EXIT_FAILURE);
	}

	ping();
	pong();
}
