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

void interrupt(int param) {
	printf("exit: %d\n", param);
	exit(EXIT_SUCCESS);
}

int sock_;
struct addrinfo* add_;


void ping() {
	printf("pinging...\n");

	char arr[84] = {0};
	arr[0] = 8;

	int             len = 0;
	struct icmp     *icmp = (struct icmp *)arr;

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 125;
	icmp->icmp_id = getpid();
	icmp->icmp_seq = 1;
//	memset (icmp->icmp_data, 0xa5, g_global->datalen);


	int ret = sendto(sock_, arr, sizeof arr, 0, add_->ai_addr, sizeof(*add_->ai_addr));


	if (ret < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}

	alarm(5);
}

void pong(int sock) {
	char			buffer[512];
	ssize_t			ret;

	struct iovec	io = {
		.iov_base = NULL,
		.iov_len = 0
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
		ret = recvmsg(sock, &msg, 0);

		if (ret < 0) {
			perror("Holy shit!");
			exit(EXIT_FAILURE);
		}
		else if (ret == 0) {
			printf("Nothing to read!\n");
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
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	errcode = getaddrinfo(host, NULL, &hints, &result);
	if (errcode != 0)
	{
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

		inet_ntop (res->ai_family, ptr, addrstr, 100);

		printf ("IPv%d address: %s %s\n", res->ai_family == PF_INET6 ? 6 : 4,
				addrstr, res->ai_canonname);

		res = res->ai_next;
	}
	else {
		printf("No host...\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);

	return res;
}


int main(int ac, char **av) {
	const char *url = "yandex.ru";

	if (signal(SIGALRM, ping) == SIG_ERR) {
		perror("alarm error");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, interrupt) == SIG_ERR) {
		perror("interruption error");
		exit(EXIT_FAILURE);
	}

	// filling message
	printf("header size: %zu\n", sizeof (struct iphdr));
	struct icmphdr icmp_header;

	icmp_header.type = ICMP_ECHO;
	icmp_header.code = 0;


	struct addrinfo* adr = lookup_host(url);

	int icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock < 0) {
		perror("");
		exit(0);
	}
	printf("socket: %d\n", icmp_sock);

	if (setsockopt(icmp_sock, IPPROTO_IP, IP_HDRINCL, (int[1]){1}, sizeof(int)) == -1) {
		perror("");
		exit(0);
	}
	sock_ = icmp_sock;
	add_ = adr;

	ping();
	pong(icmp_sock);
}
