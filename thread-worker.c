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

static void schedule();
//Define a signal handler function that will be called when the timer goes off. 
//This function will be responsible for invoking the scheduler
void timer_signal_handler(int signum) {
    if (current_thread) {
        // Switch to the scheduler's context
        swapcontext(&(current_thread->context), &scheduler_context);
    } else {
        setcontext(&scheduler_context);
    }
}
void init_timer() {
    // Register the signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = timer_signal_handler;
    sigaction(SIGPROF, &sa, NULL);

    // Set up the timer to go off after 1 second
    struct itimerval timer;
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_value.tv_sec = 1;
    setitimer(ITIMER_PROF, &timer, NULL);
}

int worker_create(worker_t *thread, pthread_attr_t *attr, 
                  void *(*function)(void*), void *arg) {
    static int firstCall = 1;
    if (firstCall) {
        firstCall = 0; //separate scheduler context initialized the first time worker_create() is called
        getcontext(&scheduler_context);
        scheduler_context.uc_stack.ss_sp = malloc(SIGSTKSZ);
        scheduler_context.uc_stack.ss_size = SIGSTKSZ;
        scheduler_context.uc_link = 0; // No next context after scheduler ends (though it shouldn't end)
        makecontext(&scheduler_context, schedule, 0); // Assuming you have a scheduler function
        init_timer();
    }

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
    current_thread->status = THREAD_READY;
    enqueue(current_thread);

    // Switch to the scheduler's context
    setcontext(&scheduler_context);

    return 0;
}

void worker_exit(void *value_ptr) {
    // Set the current thread's status to terminated
    current_thread->status = THREAD_TERMINATED;

    // Free the allocated stack and TCB of the current thread
    free(current_thread->stack);
    free(current_thread);

    // Switch to the scheduler's context to schedule another thread
    setcontext(&scheduler_context);
}

tcb* get_tcb_by_id(worker_t thread_id) {
    ThreadNode* temp = head;  // Assuming 'head' is the starting point of your global list of TCBs

    while (temp) {
        if (temp->thread->thread_id == thread_id) {
            return temp->thread;
        }
        temp = temp->next;
    }

    return NULL;  // Return NULL if no matching TCB is found
}

int is_thread_terminated(worker_t thread_id) {
    tcb* thread_tcb = get_tcb_by_id(thread_id);
    if (!thread_tcb) return 0;  // Return 0 if thread not found (or you can handle this case differently)
    return (thread_tcb->status == THREAD_TERMINATED);
}
void free_thread_resources(worker_t thread_id) {
    // Check if the list is empty
    if (!head) return;

    // Special case: if the thread to be removed is at the head of the list
    if (head->thread->thread_id == thread_id) {
        ThreadNode *temp = head;
        head = head->next;
        free(temp);
        return;
    }

    // Traverse the list to find the node to be removed
    ThreadNode *prev = head;
    ThreadNode *current = head->next;
    while (current != NULL && current->thread->thread_id != thread_id) {
        prev = current;
        current = current->next;
    }

    // If the thread is not found
    if (!current) return;

    // Adjust the pointers to unlink the node
    prev->next = current->next;

    // If the thread to be removed is at the tail of the list
    if (tail == current) {
        tail = prev;
    }

    // Free the node
    free(current);
}

int worker_join(worker_t thread, void **value_ptr) {
    // For simplicity, we will perform busy-waiting. In a real-world scenario,
    // you'd want to have a more efficient way of waiting, like blocking the current thread.
    while (1) {
        if (is_thread_terminated(thread)) {
            free_thread_resources(thread);
            break;
        }
    }
    return 0;
}


/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    // Initialize the mutex's fields
    mutex->is_locked = 0;    // Mutex is initially unlocked
    mutex->owner = NULL;    // No owner at the start
    return 0;
}

/* acquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {
    while (__sync_lock_test_and_set(&(mutex->is_locked), 1)) {
        // If the mutex is already locked, push the current thread into a block list
        // This assumes you have a function `block_current_thread` to block the current thread
        block_current_thread();
        
        // Context switch to the scheduler thread
        setcontext(&scheduler_context);
    }
    // Set the current thread as the owner of the mutex
    mutex->owner = current_thread;
    return 0;
}

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
    // Check if the current thread is the owner of the mutex
    if (mutex->owner != current_thread) {
        // The current thread is not the owner, so it cannot unlock the mutex
        return -1;  // Error code
    }
    // Release the mutex
    mutex->owner = NULL;
    __sync_lock_release(&(mutex->is_locked));
    
    // Put threads in block list to the run queue
    // This assumes you have a function `move_blocked_to_ready` to move blocked threads to the ready queue
    move_blocked_to_ready();
    return 0;
}

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
    // Check if the mutex is locked
    if (mutex->is_locked) {
        return -1;  // Cannot destroy a locked mutex
    }
    // De-allocate dynamic memory created in worker_mutex_init
    // In this example, we didn't allocate any dynamic memory, but you'd want to free any resources here if you did
    return 0;
}

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

// // - schedule policy
// #ifndef MLFQ
// 	// Choose PSJF
// #else 
// 	// Choose MLFQ
// #endif
	//simple round robin scheduler
    while (1) {
        current_thread = dequeue();
        if (current_thread) {
            setcontext(&(current_thread->context));
        } else {
            // If there are no more threads, exit the scheduler
            break;
        }
    }
}

// Initialize the scheduler's context. This should be called once.
void init_scheduler() {
    getcontext(&scheduler_context);
    scheduler_context.uc_stack.ss_sp = malloc(SIGSTKSZ);
    scheduler_context.uc_stack.ss_size = SIGSTKSZ;
    scheduler_context.uc_link = 0;
    makecontext(&scheduler_context, schedule, 0);
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

