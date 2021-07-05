#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

extern ping_context_t ping_ctx;

static void interrupt(int a) {
    printf("\n--- %s ping statistics ---\n", ping_ctx.dest);

    if (ping_ctx.stats_count != 0) {
        struct timeval end_time;
        if (gettimeofday(&end_time, NULL) != 0) {
            perror("cannot get end time");
            exit(EXIT_FAILURE);
        }

        const uint64_t avg = ping_ctx.acc_ping_time / ping_ctx.stats_count;
        const uint64_t avg2 = ping_ctx.acc_ping_time2 / ping_ctx.stats_count;
        const uint64_t mdev = sqrt(avg2 - avg * avg);
        const uint64_t working_time =
                ((end_time.tv_sec - ping_ctx.time_program_started.tv_sec) * 1000000
                + (end_time.tv_usec - ping_ctx.time_program_started.tv_usec)) / 1000;

        const float loss_percentage = 1 - (float)ping_ctx.message_received / (float)ping_ctx.messages_sent;
        printf("%zu packets transmitted, %zu received, %.2f%% packet loss, time %ldms\n"
               "rtt min/avg/max/mdev = %ld.%03ld/%ld.%03ld/%ld.%03ld/%ld.%03ld ms\n",
                ping_ctx.messages_sent, ping_ctx.message_received,
                loss_percentage,
                working_time,
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
