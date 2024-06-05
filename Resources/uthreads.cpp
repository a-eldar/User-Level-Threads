//
// Created by amare on 6/5/2024.
//

#include <vector>
#include <queue>
#include "uthreads.h"

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

public: UThreadManager() {
    threads.reserve(MAX_THREAD_NUM); // Preallocate MAX_THREADS_NUM spaces for threads
    std::fill(threads.begin(), threads.end(), nullptr);
    num_threads = 0;
}

    /** @brief Initialize UThreads lib (no validity check) */
    void init(micro_sec quantum) {
        this->quantum = quantum;
        // Perhaps this->quantum is useless.
        // Perhaps can be used once in the lib function.

        threads[0] = new Thread(0);
        threads[0]->setState(RUNNING);
        num_threads++;
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
