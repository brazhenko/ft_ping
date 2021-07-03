#include "ping.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

extern ping_context_t ping_ctx;

static void interrupt(int param) {
    printf("exit: %d\n", param);
    exit(EXIT_SUCCESS);
}

void    initialize_signals() {
    if (signal(SIGALRM, interrupt) == SIG_ERR) {
        perror("signal alarm error");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, interrupt) == SIG_ERR) {
        perror("signal interrupt error");
        exit(EXIT_FAILURE);
    }
    if (ping_ctx.flags[PING_LIFETIME_LIM]) {
        alarm(ping_ctx.seconds_to_work);
    }
}