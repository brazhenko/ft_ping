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

static uint16_t ipv4_icmp_checksum(const uint16_t *words, size_t wordcount) {
	uint32_t tmp = 0;

	for (int i = 0; i < wordcount; i++) {
		int a = (words[i]);
		tmp += a;
		tmp += (tmp >> 16);
		tmp &= UINT16_MAX;
	}

	return (tmp ^ UINT16_MAX);
}

int send_echo_msg(int sock, uint16_t id, uint8_t ttl, uint16_t icmp_seq_num) {
    const size_t
            entire_msg_size = sizeof (struct ip)
            + sizeof (struct icmphdr)
            + ping_ctx.payload_size,
            iphdr_size = sizeof (struct iphdr),
            icmphdr_size = sizeof (struct icmphdr);

    printf("ID: %d\n", id);

    char message[entire_msg_size]; // God sorry for VLA...
    memset(message, 0, entire_msg_size); // Clear buffer

    // Filling the IP header
    struct ip *ip_header = (struct ip *)message;
    ip_header->ip_v = 4;
    ip_header->ip_hl = 5;
    ip_header->ip_tos = 0;
    ip_header->ip_len = entire_msg_size;
    ip_header->ip_id = id;
    ip_header->ip_off = 0; //??
    ip_header->ip_ttl = ttl;
    ip_header->ip_p = IPPROTO_ICMP;
    ip_header->ip_dst = ((struct sockaddr_in *)ping_ctx.dest_addr_info->ai_addr)->sin_addr;
    ip_header->ip_src = ((struct sockaddr_in *)ping_ctx.src_addr_info->ai_addr)->sin_addr;
    ip_header->ip_sum = ipv4_icmp_checksum((uint16_t *)ip_header,
            icmphdr_size / 2);

    // Filling the ICMP header
    struct icmp *icmp_header = (struct icmp *)(message + iphdr_size);
    icmp_header->icmp_type = ICMP_ECHO;
    icmp_header->icmp_code = 0;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_id = __bswap_16(id);
    icmp_header->icmp_seq = __bswap_16(icmp_seq_num);

    // Filling ICMP payload
    char *payload_ptr = (char *)(message + iphdr_size + icmphdr_size);
    memset(payload_ptr, 0x42, ping_ctx.payload_size);

    // Calculating ICMP checksum after filling payload
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_cksum = ipv4_icmp_checksum((uint16_t *)icmp_header,
            (icmphdr_size + ping_ctx.payload_size) / 2);

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
    if (send_echo_msg(ping_ctx.icmp_sock, getpid(), ping_ctx.ttl, 1) != 0) {
        exit(EXIT_FAILURE);
    }

	alarm(10);
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
