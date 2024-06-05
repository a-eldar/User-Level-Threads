//
// Created by amare on 6/5/2024.
//

#include <set>
#include <queue>

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
private: std::set<Thread> thread_set;
private: std::queue<Thread> thread_queue;


};

UThreadManager manager; // Global variable responsible for controlling all of the threads.
// all library functions will use this global object operate on the threads.
// Note: uthread_init returns -1 upon receiving a negative value.
