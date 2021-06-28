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

void ping() {
	printf("pinging...\n");

	alarm(2);
}

void pong() {
	while (true) {
		// reading answer...
		;
	}
}

int lookup_host (const char *host)
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
		return -1;
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

		res = res->ai_next;
	}
	else {
		printf("No host...\n");
		return -1;
	}

	freeaddrinfo(result);

	return 0;
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


	lookup_host("yandex.ru");
	struct in_addr	adr;
	inet_pton(AF_INET, "192.168.0.1", &adr);

	ping();
	pong();
}
