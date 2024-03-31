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

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   R R S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

RRScheduler::RRScheduler(){
	rr_qsize = 0;
	ticks = 0;
	hz = 5;		// Frequency of update of ticks = 50 ms
	// instaling an interrupt handler for interrupt code 0
	InterruptHandler::register_handler(0, this);	
	// setting interrupt frequency for timer
	set_frequency(hz);
}

void RRScheduler::set_frequency(int _hz){
	hz = _hz;
	int divisor = 1193180 / _hz;				// The input clock runs at 1.19MHz
	Machine::outportb(0x43, 0x34);				// setting command byte to be 0x36
	Machine::outportb(0x40, divisor & 0xFF);	// setting low byte of divisor
	Machine::outportb(0x40, divisor >> 8);		// setting high byte of divisor
}


void RRScheduler::yield(){
	// sending an EOI message to the master interrupt controller
	Machine::outportb(0x20, 0x20);
	// disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	if(rr_qsize == 0){
		// Console::puts("Queue is empty. No threads available. \n");
	}
	else{
		// removing thread from RR queue for CPU time
		Thread * new_thread = ready_rr_queue.dequeue();
		// resseting tick count
		ticks = 0;
		// decrementing RR queue size
		rr_qsize = rr_qsize - 1;
		// re enabling interrupts
		if(!Machine::interrupts_enabled()){
			Machine::enable_interrupts();
		}
		// context-switching and giving CPU time for new thread
		Thread::dispatch_to(new_thread);
	}
}

void RRScheduler::resume(Thread * _thread){
	// disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	// adding thread to ready queue
	ready_rr_queue.enqueue(_thread);
	// incrementing RR queue size
	rr_qsize = rr_qsize + 1;
	// re enabling interrupts
	if(!Machine::interrupts_enabled()){
		Machine::enable_interrupts();
	}
}

void RRScheduler::add(Thread * _thread){
	// disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	// adding thread to ready queue
	ready_rr_queue.enqueue(_thread);
	// incrementing RR queue size
	rr_qsize = rr_qsize + 1;
	// re enabling interrupts
	if(!Machine::interrupts_enabled()){
		Machine::enable_interrupts();
	}
}

void RRScheduler::terminate(Thread * _thread){
	// disabling interrupts when performing any operations on ready queue
	if(Machine::interrupts_enabled()){
		Machine::disable_interrupts();
	}
	int index = 0;
	// iterating and dequeuing each thread
	// checking if thread id matches with thread to be terminated
	// If thread id does not match, add thread back to ready queue
	for(index=0;index<rr_qsize;index++){
		Thread * top = ready_rr_queue.dequeue();
		if(top->ThreadId() != _thread->ThreadId()){
			ready_rr_queue.enqueue(top);
		}
		else{
			rr_qsize = rr_qsize - 1;
		}
	}
	// re enabling interrupts
	if(!Machine::interrupts_enabled()){
		Machine::enable_interrupts();
	}
}

void RRScheduler::handle_interrupt(REGS * _regs){
	// incrementing ticks count
    ticks = ticks + 1;
	// time quanta is completed
	// preempting current thread and run next thread
    if (ticks >= hz){
        // resetting tick count
		ticks = 0;
        Console::puts("Time Quanta (50 ms) has passed \n");
		resume(Thread::CurrentThread()); 
		yield();
    }
}