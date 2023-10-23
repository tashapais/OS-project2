#include <stdio.h>
#include <unistd.h>
#include <ucontext.h>
#include "thread-worker.h"

// Function that will be executed in one of the threads
void foo(void *arg) {
    while (1) {
        printf("foo\n");
        sleep(1);  // Sleep to avoid flooding the console
    }
}

// Function that will be executed in another thread
void bar(void *arg) {
    while (1) {
        printf("bar\n");
        sleep(1);  // Sleep to avoid flooding the console
    }
}

int main(int argc, char **argv) {
    worker_t thread1, thread2;

    // Initialize the scheduler
    //init_scheduler();

    // Create two threads
    worker_create(&thread1, NULL, (void* (*)(void*))foo, NULL);
    worker_create(&thread2, NULL, (void* (*)(void*))bar, NULL);

    // For this test, just loop indefinitely in main
    // In a real scenario, you might want to join the threads or perform other operations.
    while (1) sleep(1);

    return 0;
}
