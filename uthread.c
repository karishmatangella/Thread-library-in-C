#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <limits.h>

#include "context.h"
#include "preempt.h"
#include "queue.h"
#include "uthread.h"

/* STRUCT DEFINITION: Uthread_obj_t -------------------------------------------
 * 
 * The struct name containing all the information
 * needed for each thread.
*/
typedef struct uthread_obj
{
	uthread_t tid;
	uthread_ctx_t t_ctx;
	void* stack;
	uthread_func_t t_func;
	uthread_t thread_im_blocking;
	bool joined_thread;
	uthread_t thread_blocking_me;
	int func_retval;
	void* arg;
} uthread_obj_t;
// END STRUCT DEFINITION ------------------------------------------------------

// GLOBALS: -------------------------------------------------------------------
bool initialization_done = false; // denoting if initialization has been performed.
uthread_t global_tid = 0; // initial number for the thread identifier.

uthread_obj_t* running_thread; // global holding the running thread
queue_t ready_queue; // global holding the ready threads
queue_t blocked_queue; // global holding the blocked threads
queue_t zombie_queue; // global holding the zombie threads
// END GLOBALS -----------------------------------------------------------------

/* HELPER FUNCTIONS: ----------------------------------------------------------
 *
 * static int - Function type for queue_iterate
 * @data: data contained inside each node of the queue
 * @tid: tid that we are trying to find when iterating through the queue.
 * 
 * This function will compare the @tid with the current @data's tid.
 *
 * Return: Integer value (1 for found, 0 for not found)
 */
static int find_tid(void *data, void* tid)
{
	preempt_enable();
    uthread_obj_t* a = (uthread_obj_t*) data;
	uthread_t* b_tid = (uthread_t*) tid;
    if (a->tid == *b_tid)
        return 1;
    return 0;
}

/*
 * static int - Function type for queue_iterate
 * @data: data contained inside each node of the queue
 * @tid: tid that we are trying to find when iterating through the queue.
 * 
 * This function will compare the @tid with the current @data's tid and 
 * check whether the @data associated with that @tid has the
 * flag joined_thread as true, meaning that the thread has already been joined.
 *
 * Return: Integer value (1 for found, 0 for not found)
 */
static int find_joined_thread(void *data, void* tid)
{
	preempt_enable();
    uthread_obj_t* a = (uthread_obj_t*) data;
	uthread_t* b_tid = (uthread_t*) tid;
    if (a->tid == *b_tid)
	{
		if (a->joined_thread == true)
		{
			return 1;
		}
	}
    return 0;
}
//END HELPER FUNCTIONS --------------------------------------------------------


void uthread_yield(void)
{
	preempt_enable();
	uthread_obj_t* new_running_thread;
	preempt_disable();

	// In case the ready queue is empty, return:
	if(queue_dequeue(ready_queue, (void **)(&new_running_thread)) == -1)
	{
		preempt_enable();
		return;
	}

	queue_enqueue(ready_queue, (void*)(running_thread));
	uthread_obj_t* temp = running_thread;
	running_thread = new_running_thread;

	/* perform the context switch with the old running thread as temp
	 * which holds the previous running thread, and the new running
	 * thread.
	 */
	uthread_ctx_switch(&temp->t_ctx, &new_running_thread->t_ctx);
	preempt_enable();
}

uthread_t uthread_self(void)
{
	preempt_enable();
	return running_thread->tid;
}

int uthread_init()
{
	// Initialize all of our queues:
	ready_queue = queue_create();
	blocked_queue = queue_create();
	zombie_queue = queue_create();

	initialization_done = true; // mark initialization as DONE
	preempt_start(); // set up preemption
	return 0;
}
			
int uthread_create(uthread_func_t func, void *arg)
{
	if (initialization_done == false)
	{
		uthread_init(); // initializing the library
		preempt_enable();

		running_thread = (uthread_obj_t*)malloc(sizeof(uthread_obj_t));
		// Error: memory allocation failure:
		if(running_thread == NULL)
		{
			preempt_enable();
			return -1;
		}

		preempt_disable();

		// setting the properties of the main thread:
		running_thread->joined_thread = false;
		running_thread->thread_blocking_me = -1;
		running_thread->thread_im_blocking = -1;
		running_thread->tid = global_tid++;
	}

	// Error: TID Unsigned long Overflow
	if (global_tid > USHRT_MAX)
	{
		preempt_enable();
		return -1;
	}
	preempt_enable();

	uthread_obj_t* current_thread = (uthread_obj_t*)malloc(sizeof(uthread_obj_t));
	// Error: memory allocation failure:
	if(current_thread == NULL)
	{
		preempt_enable();
		return -1;
	}

	// setting all of the properties of the thread to be enqueued in ready queue 
	current_thread->t_func = func;
	current_thread->arg = arg;
	current_thread->joined_thread = false;
	current_thread->thread_blocking_me = -1;
	current_thread->thread_im_blocking = -1;
	preempt_disable();
	current_thread->tid = global_tid++;
	preempt_enable();

	current_thread->stack = uthread_ctx_alloc_stack();
	// Error: stack memory allocation failure:
	if (current_thread->stack == NULL)
	{
		preempt_disable();
		return -1;
	}

	preempt_disable();

	int err = uthread_ctx_init(&current_thread->t_ctx, (char*)(current_thread->stack), \
		current_thread->t_func, current_thread->arg);
	// Error: context init failure:
	if (err == -1)
	{
		preempt_enable();
		return -1;
	}

	queue_enqueue(ready_queue, (void*)(current_thread)); // enqueueing created thread
	preempt_enable();
	return current_thread->tid;
}

void uthread_exit(int retval)
{
	preempt_enable();
	uthread_obj_t* blocked_thread = NULL;
	preempt_disable();

	// Find the thread that the thread that has exited is blocking, if any:
	queue_iterate(blocked_queue, find_tid, \
		(void*) (uthread_t*)(&(running_thread->thread_im_blocking)), \
			(void**)&blocked_thread);
	if (blocked_thread != NULL)
	{
		blocked_thread->func_retval = retval; // parent collecting child's retval
		queue_delete(blocked_queue, (void*)(blocked_thread));
		blocked_thread->thread_blocking_me = -1; // thread is no longer blocking any threads
		queue_enqueue(ready_queue, (void*)(blocked_thread)); // enqueue ex-blocked 
															 // thread into ready queue
	}
	running_thread->func_retval = retval;
	uthread_obj_t* new_running_thread = NULL;
	queue_dequeue(ready_queue, (void**)(&new_running_thread));
	running_thread->thread_im_blocking = -1;
	queue_enqueue(zombie_queue, (void*)(running_thread));
	uthread_obj_t* temp = running_thread;
	running_thread = new_running_thread;

	//context switch between old running thread and new running thread
	uthread_ctx_switch(&temp->t_ctx, &running_thread->t_ctx);
	preempt_enable();
}

int uthread_join(uthread_t tid, int *retval)
{
	preempt_enable();

	// ERROR CHECKING: 
	if (tid >= global_tid || tid == 0 || tid == running_thread-> tid)
	{
		preempt_enable();
		return -1;
	}

	void* ready_found = NULL;
	void* blocked_found = NULL;
	void* zombie_found = NULL;
	preempt_disable();

	// Iterate through all of the queues to see if the thread associated with
	// the @tid has been joined by another thread before. Return -1 if so.
	queue_iterate(ready_queue, find_joined_thread,  &tid, (void**)&ready_found);
	queue_iterate(blocked_queue, find_joined_thread, (void*) &tid, (void**)&blocked_found);
	queue_iterate(zombie_queue, find_joined_thread, (void*) &tid, (void**)&zombie_found);
	if(ready_found != NULL || blocked_found != NULL || zombie_found != NULL)
	{
		preempt_enable();
		return -1;
	}

	// JOINING THREAD:
	queue_iterate(zombie_queue, find_tid, (void*) (uthread_t*)&tid, (void**)&zombie_found);

	// CASE 1: thread associated with @tid is already finished.
	// Here parent collects the retval of the thread and the memory associated
	// with the thread gets freed.
	if (zombie_found != NULL)
	{
		uthread_obj_t* zombie = (uthread_obj_t*)zombie_found;
		*retval = (int)(uthread_t)zombie->func_retval;
		uthread_ctx_destroy_stack(zombie->stack);
		free(zombie);
		preempt_enable();
		return 0;
	}

	// CASE 2: thread associated with @tid is NOT finished.
	// Here the thread associated with @tid becomes the new running thread
	// and the current running thread is enqueued in the blocked queue
	// indicating that it is blocked.
	else
	{
		uthread_obj_t* thread;
		uthread_obj_t* temp;
		queue_iterate(ready_queue, find_tid, (void*) (uthread_t*)&tid, (void**)&ready_found);
		if (ready_found != NULL)
		{
			uthread_obj_t* ready = (uthread_obj_t*)ready_found;
			queue_enqueue(blocked_queue, (void*)running_thread);
			while(1)
			{	
				queue_dequeue(ready_queue, (void**)(&thread));
				if(thread == ready)
				{
					ready->thread_im_blocking = running_thread->tid;
					ready->joined_thread = true;
					running_thread->thread_blocking_me = ready->tid;
					temp = running_thread;
					running_thread = ready;
					uthread_ctx_switch(&temp->t_ctx, &running_thread->t_ctx);
					*retval = running_thread->func_retval;
					break;
				}
				else
				{
					queue_enqueue(ready_queue, (void*)(thread));
				}
			}
		}

		// CASE 3: ERROR: @tid cannot be found in ready queue, and if it is in
		// blocked queue then that would cause a thread deadlock, so STOP.
		else 
		{
			preempt_enable();
			return -1;
		}
	}
	preempt_enable();
	return 0;
}