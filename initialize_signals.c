#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

extern ping_context_t ping_ctx;

static void interrupt(int a) {
    struct timeval end_time;
    if (gettimeofday(&end_time, NULL) != 0) {
        perror("cannot get end time");
        exit(EXIT_FAILURE);
    }

    printf("\n--- %s ping statistics ---\n", ping_ctx.canon_dest);

    const float loss_percentage = (1 - (float)ping_ctx.messages_received / (float)ping_ctx.messages_sent) * 100;
    printf("%zu packets transmitted, %zu received, ",
            ping_ctx.messages_sent, ping_ctx.messages_received);
    if (ping_ctx.error_messages_received != 0) {
        printf("+%zu errors, ", ping_ctx.error_messages_received);
    }
    printf("%.2f%% packet loss, time %ldms\n",
            loss_percentage,
            time_diff(&ping_ctx.time_program_started, &end_time) / MICROSECONDS_IN_MILLISECOND);

    if (ping_ctx.stats_count != 0) {
        const uint64_t avg = ping_ctx.acc_ping_time / ping_ctx.stats_count;
        const uint64_t avg2 = ping_ctx.acc_ping_time2 / ping_ctx.stats_count;
        const uint64_t mdev = sqrt(avg2 - avg * avg);

        printf("rtt min/avg/max/mdev = %ld.%03ld/%ld.%03ld/%ld.%03ld/%ld.%03ld ms\n",
                ping_ctx.min_ping_time / 1000, ping_ctx.min_ping_time % 1000,
                avg / 1000, avg % 1000,
                ping_ctx.max_ping_time / 1000, ping_ctx.max_ping_time % 1000,
                mdev / 1000, mdev % 1000
        );
    }

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
