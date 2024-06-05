//
// Created by amare on 6/5/2024.
//

#include <vector>
#include <queue>
#include "uthreads.h"

enum ThreadState {
    READY, RUNNING, BLOCKED
};




/**
 * @brief Represents a single thread.
 */
class Thread {
private: ThreadState state;
private: int tid;

public: explicit Thread(int tid) {
        this->tid = tid;
        this->state = READY;
    }
};




class UThreadManager {
private: std::vector<Thread> threads;
private: std::queue<Thread> thread_queue;

public: UThreadManager() {
    // Preallocate MAX_THREADS_NUM spaces for threads
    threads.reserve(MAX_THREAD_NUM);
}

/** @brief Adds a new thread.
 * @return tid */
public: int SpawnThread() {
    // Implement
}
};






UThreadManager manager; // Global variable responsible for controlling all of the threads.
// all library functions will use this global object operate on the threads.
// Note: uthread_init returns -1 upon receiving a negative value.


/* API functions implementation: */
