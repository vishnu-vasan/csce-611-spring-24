/*
 File: scheduler.C
 
 Author:
 Date  :
 
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
  assert(false);
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  // Disable interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	if(qsize == 0){
		// Console::puts("Queue is empty. No threads available. \n");
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

void Scheduler::resume(Thread * _thread) {
  
}

void Scheduler::add(Thread * _thread) {
  assert(false);
}

void Scheduler::terminate(Thread * _thread) {
  assert(false);
}
