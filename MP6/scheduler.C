/*
 File: scheduler.C
 
 Author: Vishnuvasan Raghuraman
 Date  : 03/30/2024
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "machine.H"

extern BlockingDisk * SYSTEM_DISK;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  qsize = 0;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  // Disable interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
    // checking if disk is ready and blocked threads exist in the queue
	// if hte disk is ready, immediately dispatch to the top thread in blocked queue
	if(SYSTEM_DISK->check_blocked_thread_in_queue()){
        // re enabling interrupts
		if(!Machine::interrupts_enabled()){
			Machine::enable_interrupts();
		}
		Thread::dispatch_to(SYSTEM_DISK->get_top_thread());
	}
    // if the disk is not ready or blocked threads do not exist in queue
	// checking the regular FIFO ready queue
    else{
        if(qsize == 0){
            Console::puts("Queue is empty. No threads available. \n");
        }
        else{
            // removing thread from queue for CPU time
            Thread * new_thread = ready_queue.dequeue();
            // decrementing queue size
            qsize = qsize - 1;
            // re enabling interrupts
            if(!Machine::interrupts_enabled()){
                Machine::enable_interrupts();
            }
            // context-switch and giving CPU time for new thread
            Thread::dispatch_to(new_thread);
        }
    }
}

void Scheduler::resume(Thread * _thread) {
  // disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	// adding thread to ready queue
	ready_queue.enqueue(_thread);
	// incrementing queue size
	qsize = qsize + 1;
	// re enabling interrupts
	if(!Machine::interrupts_enabled()){
		Machine::enable_interrupts();
	}
}

void Scheduler::add(Thread * _thread) {
  // disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	// adding thread to ready queue
	ready_queue.enqueue(_thread);
	// incrementing queue size
	qsize = qsize + 1;
	// re enabling interrupts
	if(!Machine::interrupts_enabled()){
		Machine::enable_interrupts();
	}
}

void Scheduler::terminate(Thread * _thread) {
  // disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	int index = 0;
	// iterating and dequeuing each thread
	// checking if thread ID matches with thread to be terminated
	// adding thread back to ready queue if thread id doesn't match
	for(index=0;index<qsize;index++){
		Thread * top = ready_queue.dequeue();
		if(top->ThreadId() != _thread->ThreadId()){
			ready_queue.enqueue(top);
		}
		else{
			qsize = qsize - 1;
		}
	}
	// re enabling interrupts
	if(!Machine::interrupts_enabled()){
		Machine::enable_interrupts();
	}
}