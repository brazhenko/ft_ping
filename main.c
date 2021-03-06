#include "ping.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

static void async_ping() {
    pthread_t thread;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type" // supress func ptr cast error
    if (pthread_create(&thread, NULL, (void *(*)(void *))sync_ping, NULL) != 0) {
        perror("cannot create thread");
        exit(EXIT_FAILURE);
    }
#pragma GCC diagnostic pop

    if (pthread_detach(thread) != 0) {
        perror("cannot detach thread");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    // Init all
    initialize_context(argc, argv);
    initialize_signals();

    // Do all
    async_ping();
    sync_pong();
}
