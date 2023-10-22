// File:	thread-worker.c

// List all group member's name: Tasha Pais
// username of iLab:tdp74
// iLab Server: rlab2

#include "thread-worker.h"

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;


// Define a node structure for the ready queue
typedef struct ThreadNode {
    tcb *thread;
    struct ThreadNode *next;
} ThreadNode;

// Global variables
ThreadNode *head = NULL;           // Head of the ready queue
ThreadNode *tail = NULL;           // Tail of the ready queue
ucontext_t scheduler_context;      // Scheduler's context
tcb *current_thread = NULL;        // Pointer to the currently executing thread

// Function to add a thread to the ready queue
void enqueue(tcb *thread) {
    ThreadNode *newNode = malloc(sizeof(ThreadNode));
    newNode->thread = thread;
    newNode->next = NULL;

    if (!head) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
}

// Function to remove a thread from the ready queue
tcb* dequeue() {
    if (!head) return NULL;

    ThreadNode *temp = head;
    tcb *thread = temp->thread;

    head = head->next;
    free(temp);

    return thread;
}

int worker_create(worker_t *thread, pthread_attr_t *attr, 
                  void *(*function)(void*), void *arg) {
    tcb *newThread = malloc(sizeof(tcb));

    // Assign a unique thread ID. For simplicity, we'll use a static counter.
    static worker_t last_thread_id = 0;
    newThread->thread_id = ++last_thread_id;

    // Initialize the thread's context
    getcontext(&(newThread->context));

    // Allocate space for the thread's stack
    newThread->stack = malloc(SIGSTKSZ);
    newThread->context.uc_stack.ss_sp = newThread->stack;
    newThread->context.uc_stack.ss_size = SIGSTKSZ;
    newThread->context.uc_link = &scheduler_context;

    // Set up the thread's context to execute the given function
    makecontext(&(newThread->context), (void (*)())function, 1, arg);

    // Add the thread to the ready queue
    enqueue(newThread);

    return 0;
}

int worker_yield() {
    // Save the current thread's context
    getcontext(&(current_thread->context));

    // Change the thread's state to ready and add it to the ready queue
    enqueue(current_thread);

    // Switch to the scheduler's context
    setcontext(&scheduler_context);

    return 0;
}

void worker_exit(void *value_ptr) {
    // Free the allocated stack and TCB of the current thread
    free(current_thread->stack);
    free(current_thread);

    // Switch to the scheduler's context to schedule another thread
    setcontext(&scheduler_context);
}


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// - schedule policy
#ifndef MLFQ
	// Choose PSJF
#else 
	// Choose MLFQ
#endif

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

