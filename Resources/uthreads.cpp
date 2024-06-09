//
// Created by amare on 6/5/2024.
//

#include <iostream>
#include <vector>
#include <list>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <setjmp.h>

#define FAILURE (-1)
#define SUCCESS 0
#define USEC_IN_SEC 1000000


typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
}

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
    char* stack;
    sigjmp_buf env;

public:
    /** Creates new Thread with state=READY */
    explicit Thread(int tid) {
        this->tid = tid;
        this->state = READY;
        this->stack = new char[STACK_SIZE];
    }

    ThreadState getState() const {
        return state;
    }

    int getTid() const {
        return tid;
    }

    char* getStack() {
        return stack;
    }

    void setState(ThreadState new_state) {
        state = new_state;
    }

    sigjmp_buf& getEnv() { return env; }

    ~Thread() {
        delete[] stack;
    }
};





/**
 * @brief Class instance manages all UThreads operations
 *      and thread holding.
 */
class UThreadManager {
    std::vector<Thread*> threads;
    std::list<int> thread_queue;
    std::list<std::pair<int, int>> sleeping_threads;
    micro_sec quantum;
    int num_threads;
    int running_thread_tid;

public:
    UThreadManager() : threads(MAX_THREAD_NUM, nullptr) {
    // Preallocate MAX_THREADS_NUM spaces for threads
    num_threads = 0;
    }

    /** @brief Initialize UThreads lib (no validity check) */
    void init(micro_sec quantum) {
        this->quantum = quantum;
        // Perhaps this->quantum is useless.
        // Perhaps can be used once in the lib function.

        threads[0] = new Thread(0); // TODO: What is the main entry point?
        sigsetjmp(threads[0]->getEnv(), 1);
        threads[0]->setState(RUNNING);
        running_thread_tid = 0;
        num_threads = 1;
    }

    /**
     * @brief Adds a new thread
     * @return tid or FAILURE
     */
    int spawnThread(thread_entry_point entry_point) {
        int tid = findMinAvailableTid();
        if (tid == FAILURE) return FAILURE;
        threads[tid] = new Thread(tid);
        setup_thread(tid, threads[tid]->getStack(), entry_point);
        thread_queue.push_back(tid);
        num_threads++;
        return tid;
    }

    void expireQuantum() {
        reduceSleep();
        if (thread_queue.empty()) return;
        queueRunningThread();
    }

    /** @brief terminates thread with that tid. Should not be called with 0 (main thread)
     * @return SUCCESS or FAILURE
     */
    int terminateThread(int tid) {
        if (tid <= 0 || tid >= MAX_THREAD_NUM) return FAILURE;
        if (threads[tid] == nullptr) return FAILURE;
        if (threads[tid]->getState() == READY)
            thread_queue.remove(tid);
        delete threads[tid];
        num_threads--;
        return SUCCESS;
    }

    int blockThread(int tid) {
        if (tid <= 0 || tid >= MAX_THREAD_NUM) return FAILURE;
        if (threads[tid] == nullptr) return FAILURE;

        if (threads[tid]->getState() == RUNNING)
            expireQuantum();
            // TODO: Reset timer...
        if (threads[tid]->getState() == READY)
            thread_queue.remove(tid);
        threads[tid]->setState(BLOCKED);
        return SUCCESS;
    }

    int resumeThread(int tid) {
        if (tid < 0 || tid >= MAX_THREAD_NUM) return FAILURE;
        if (threads[tid] == nullptr) return FAILURE;

        if (threads[tid]->getState() == BLOCKED) {
            threads[tid]->setState(READY);
            thread_queue.push_back(tid);
        }
        return SUCCESS;
    }

    int sleepThread(int num_quantums) {
        if (running_thread_tid == 0) return FAILURE;
        sleeping_threads.emplace_back(running_thread_tid, num_quantums);
        threads[running_thread_tid]->setState(BLOCKED);
        running_thread_tid = popReadyThread();
        return SUCCESS;
    }

    ~UThreadManager() {
        for (const auto& thread_ptr : threads) {
            delete thread_ptr;
        }
    }


private:
    int popReadyThread() {
        if (thread_queue.empty()) return FAILURE;
        int tid = thread_queue.front();
        thread_queue.pop_front();
        return tid;
    }

    int findMinAvailableTid() {
        for (int i = 0; i < MAX_THREAD_NUM; i++)
            if (threads[i] == nullptr)
                return i;

        return FAILURE;
    }

    void setup_thread(int tid, char *stack, thread_entry_point entry_point)
    {
        // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
        // siglongjmp to jump into the thread.
        address_t sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
        address_t pc = (address_t) entry_point;
        sigjmp_buf& env = threads[tid]->getEnv();
        sigsetjmp(threads[tid]->getEnv(), 1);
        (env->__jmpbuf)[JB_SP] = translate_address(sp);
        (env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&env->__saved_mask);
    }

    void reduceSleep() {
        for (auto i = sleeping_threads.begin(); i != sleeping_threads.end(); i++) {
            i->second--;
            int tid = i->first;
            int num_quantums = i->second;
            if (!num_quantums) {
                threads[tid]->setState(READY);
                thread_queue.push_back(tid);
                sleeping_threads.erase(i--);
            }
        }
    }

    void queueRunningThread() {
        threads[running_thread_tid]->setState(READY);
        thread_queue.push_back(running_thread_tid);
        running_thread_tid = popReadyThread();
        threads[running_thread_tid]->setState(RUNNING);
    }
};






UThreadManager manager; // Global variable responsible for controlling all of the threads.
// all library functions will use this global object operate on the threads.
// Note: uthread_init returns -1 upon receiving a negative value.


/* API functions implementation: */
//

void timer_handler(int sig) {
    manager.expireQuantum();
}

int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) return FAILURE;
    manager.init(quantum_usecs);


    struct sigaction sa = {nullptr};
    struct itimerval timer;

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        return FAILURE;
    }

    timer.it_value.tv_sec = quantum_usecs / USEC_IN_SEC;        // first time interval, seconds part
    timer.it_value.tv_usec = quantum_usecs % USEC_IN_SEC;        // first time interval, microseconds part

    timer.it_interval.tv_sec = quantum_usecs / USEC_IN_SEC;    // following time intervals, seconds part
    timer.it_interval.tv_usec = quantum_usecs % USEC_IN_SEC;    // following time intervals, microseconds part

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        return FAILURE;
    }

    return SUCCESS;
}

int uthread_spawn(thread_entry_point entry_point) {
    return manager.spawnThread(entry_point);
}

int uthread_terminate(int tid) {
    if (tid == 0) {
        manager.~UThreadManager();
        exit(SUCCESS);
    }
    return manager.terminateThread(tid);
}

int uthread_block(int tid) {
    return manager.blockThread(tid);
}

int uthread_resume(int tid) {
    return manager.resumeThread(tid);
}

int uthread_sleep(int num_quantums) {
    return manager.sleepThread(num_quantums); // TODO: how?
}
