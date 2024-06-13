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
    int num_quantums;

public:
    /** Creates new Thread with state=READY, num_quantums=0 */
    explicit Thread(int tid) {
        this->tid = tid;
        this->state = READY;
        this->stack = new char[STACK_SIZE];
        num_quantums = 0;
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

    void addQuantum() {
        num_quantums++;
    }

    int getNumQuantums() const { return num_quantums; }

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
    int num_quantums;

public:
    UThreadManager() : threads(MAX_THREAD_NUM, nullptr) {
        // Preallocate MAX_THREADS_NUM spaces for threads
        num_threads = 0;
        num_quantums = 0;
    }

    /** @brief Initialize UThreads lib (no validity check) */
    void init(micro_sec quantum) {
        this->quantum = quantum;
        // Perhaps this->quantum is useless.
        // Perhaps can be used once in the lib function.

        threads[0] = new Thread(0);
        threads[0]->setState(RUNNING);
        threads[0]->addQuantum();
        running_thread_tid = 0;
        num_threads = 1;
        num_quantums = 1;
        sigsetjmp(threads[0]->getEnv(), 1);
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
        num_quantums++;
        reduceSleep();
        if (!thread_queue.empty()) {
            queueRunningThread();
        }
        else threads[running_thread_tid]->addQuantum();
    }

    /** @brief terminates thread with that tid. Should not be called with 0 (main thread)
     * @return SUCCESS or FAILURE
     */
    int terminateThread(int tid) {
        if (!isTidValid(tid, false)) return FAILURE;

        if (threads[tid]->getState() == READY)
            thread_queue.remove(tid);
        removeSleepingTid(tid);
        delete threads[tid];
        threads[tid] = nullptr;
        num_threads--;
        if (tid == running_thread_tid){
          num_quantums++;
          queueRunningThread();
        }
        return SUCCESS;
    }


    int blockThread(int tid, bool to_sleep=false) {
        if (!isTidValid(tid, false)) return FAILURE;

        if (threads[tid]->getState() == RUNNING) {
            num_quantums++;
            queueRunningThread(BLOCKED);
            // TODO: Reset timer...
        }
        if (threads[tid]->getState() == READY){
            threads[tid]->setState(BLOCKED);
            thread_queue.remove(tid);
        }
        if (threads[tid]->getState() == BLOCKED && !to_sleep){
            removeSleepingTid(tid);
        }
        return SUCCESS;
    }

    int resumeThread(int tid) {
        if (!isTidValid(tid, true)) return FAILURE;

        if (threads[tid]->getState() == BLOCKED) {
            threads[tid]->setState(READY);
            thread_queue.push_back(tid);
        }
        return SUCCESS;
    }

    int sleepThread(int num_quantums) {
        if (running_thread_tid == 0) return FAILURE;
        if (threads[running_thread_tid]->getState() == BLOCKED) return SUCCESS;
        sleeping_threads.emplace_back(running_thread_tid, num_quantums);
        blockThread(running_thread_tid, true);
        return SUCCESS;
    }

    int getRunningTid() const {
        return running_thread_tid;
    }

    int getNumQuantums() const {return num_quantums;}

    int getNumRunningQuantums(int tid) const {
        if (!isTidValid(tid, true)) return FAILURE;

        return threads[tid]->getNumQuantums();
    }

    ~UThreadManager() {
        for (auto& thread_ptr : threads) {
            delete thread_ptr;
        }
    }


private:
    /** @param tid the thread id to check
     * @param with0 is 0 valid
     * @return false if tid is not valid else true
     */
    bool isTidValid(int tid, bool with0) const {
        if (tid < 0 || tid >= MAX_THREAD_NUM) return false;
        if (!with0 & (tid == 0)) return false;
        if (threads[tid] == nullptr) return false;
        return true;
    }

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
            // Pairs of <Tid, Quantums Remaining>
            i->second--;
            int tid = i->first;
            if (!i->second) {
                threads[tid]->setState(READY);
                thread_queue.push_back(tid);
                sleeping_threads.erase(i--);
            }
        }
    }

    /**
     * @param new_state - new state for the previous running thread (READY/BLOCKED)
     */
    void queueRunningThread(ThreadState new_state=READY) {
        int ret_value;
        if (threads[running_thread_tid]) {
            threads[running_thread_tid]->setState(new_state);
            if (new_state == READY)
                thread_queue.push_back(running_thread_tid);
            ret_value = sigsetjmp(threads[running_thread_tid]->getEnv(), 1);
        }
        else ret_value = 0;
        if (ret_value == 0) {
            running_thread_tid = popReadyThread();
            threads[running_thread_tid]->addQuantum();
            threads[running_thread_tid]->setState(RUNNING);
            siglongjmp(threads[running_thread_tid]->getEnv(), 1);
        }
    }

    void removeSleepingTid(int tid) {
        sleeping_threads.remove_if([&tid](const std::pair<int, int>& pair) {return pair.first == tid;});
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

int uthread_get_tid() {
    return manager.getRunningTid();
}

int uthread_get_total_quantums() {
    return manager.getNumQuantums();
}

int uthread_get_quantums(int tid) {
    return manager.getNumRunningQuantums(tid);
}
