//
// Created by amare on 6/5/2024.
//

#include <iostream>
#include <vector>
#include <queue>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#define FAILURE (-1)
#define SUCCESS 0

typedef int micro_sec;

enum ThreadState {
    READY, RUNNING, BLOCKED
};




/**
 * @brief Represents a single thread.
 */
class Thread {
    ThreadState state;
    int tid;

public: explicit Thread(int tid) {
        this->tid = tid;
        this->state = READY;
    }

    ThreadState getState() const {
        return state;
    }

    int getTid() const {
        return tid;
    }

    void setState(ThreadState new_state) {
        state = new_state;
    }
};





/**
 * @brief Class instance manages all UThreads operations
 *      and thread holding.
 */
class UThreadManager {
    std::vector<Thread*> threads;
    std::queue<Thread*> thread_queue;
    micro_sec quantum;
    int num_threads;

public: UThreadManager() : threads(MAX_THREAD_NUM, nullptr) {
    // Preallocate MAX_THREADS_NUM spaces for threads
    num_threads = 0;
    }

    /** @brief Initialize UThreads lib (no validity check) */
    void init(micro_sec quantum) {
        this->quantum = quantum;
        // Perhaps this->quantum is useless.
        // Perhaps can be used once in the lib function.

        threads[0] = new Thread(0);
        threads[0]->setState(RUNNING);
        num_threads = 1;
    }

    /**
     * @brief Adds a new thread.
     * @return tid
     */
    int spawnThread() {
        num_threads++;
        // TODO: Implement

        // TODO: If we need to use thread_queue, ensure to add thread to queue \
            every time we set its state to READY!
    }

    ~UThreadManager() {
        for (const auto& thread_ptr : threads) {
            delete thread_ptr;
        }
    }
};






UThreadManager manager; // Global variable responsible for controlling all of the threads.
// all library functions will use this global object operate on the threads.
// Note: uthread_init returns -1 upon receiving a negative value.


/* API functions implementation: */
//

int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) return FAILURE;
    manager.init(quantum_usecs);

    struct sigaction sa = {0};
    struct itimerval timer;

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &manager.;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        return FAILURE;
    }

    // Configure the timer to expire after 1 sec... */
    timer.it_value.tv_sec = 1;        // first time interval, seconds part
    timer.it_value.tv_usec = 0;        // first time interval, microseconds part

    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = 3;    // following time intervals, seconds part
    timer.it_interval.tv_usec = 0;    // following time intervals, microseconds part
}
