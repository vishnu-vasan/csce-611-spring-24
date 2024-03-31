/* 
    Author: R. Bettati, Joshua Capehart
            Department of Computer Science
            Texas A&M University
			
	    A thread scheduler.

*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#ifndef NULL
#define NULL 0L
#endif

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"
#include "interrupts.H"

/*--------------------------------------------------------------------------*/
/* !!! IMPLEMENTATION HINT !!! */
/*--------------------------------------------------------------------------*/
/*
    One way to proceed is to implement the FIFO scheduling policy inside
    class 'Scheduler'. 

    If you plan to implement a Round-Robin Scheduler, derive it from class
    'Scheduler', say as class 'RRScheduler'. The class 'RRScheduler' is 
    really just a FIFO scheduler with THREE MODIFICATIONS:
    1. It manages a timer, which fires at the end-of-quantum (EOQ). 
    (For details on how to set up a timer and how to handle timer interrupts 
    see the 1-second timer in 'kernel.C'.)  The timer is set up in the
    constructor.
    2. It uses an additional function, the EOQ handler. This function gets
    called whenever an EOQ timer event fires. The EOQ handler forces the 
    current thread to call the scheduler's 'yield' function.
    3. The 'yield' function must be modified to account for unused quantum
    time. If a thread voluntarily yields, the EOQ timer must be reset in order
    to not penalize the next thread.
 
    (Note that this qualifies as programming at about the level of a baboon.
     Much better woudl be to have the abstract class 'Scheduler' implement 
     the basic scheduling MECHANISMS and provide abstract funtions to define
     the queue management POLICIES in derived classes, 
     such as 'FIFOScheduler'.)
    
 */

/*--------------------------------------------------------------------------*/
/* QUEUE DATA STRUCTURE */
/*--------------------------------------------------------------------------*/

class Queue
{
	private:
	
	Thread* thread;   // Pointer to thread at the top of queue
	Queue* next;		// Pointer to next thread in queue. 
		
	public:
	Queue(){  // Constructor for initial setup
		thread = NULL;
		next = NULL;
	}
	Queue(Thread* new_thread){ // Constructor for new threads to enter queue
		thread = new_thread;
		next = NULL;
	}
	void enqueue(Thread* new_thread){   // adding thread at the end of queue
		if (thread == NULL){   // first thread added to queue
			thread = new_thread;
		}
		else{	
			if(next != NULL){  // traversing the queue
				next->enqueue(new_thread);
			}
			else{ // Reached end of queue: adding new thread at the end of queue
				next =  new Queue(new_thread);
			}
		}
	}
	Thread* dequeue(){   // removing thread at head position and point to next thread in queue
		if(thread == NULL){ // empty queue
			return NULL; 
		}
		
		Thread *top = thread;   // Getting top of queue element
	
		if(next == NULL){
			thread = NULL;
		}
		else{
			thread = next->thread;  // updating head element of queue
			Queue * del_node = next;   // deleting current node
			next = next->next;     			
			delete del_node;
		}
		return top;
	}
};

/*--------------------------------------------------------------------------*/
/* SCHEDULER */
/*--------------------------------------------------------------------------*/

class Scheduler {

  /* The scheduler may need private members... */
  Queue ready_queue;
  int qsize;
  
public:

   Scheduler();
   /* Setup the scheduler. This sets up the ready queue, for example.
      If the scheduler implements some sort of round-robin scheme, then the 
      end_of_quantum handler is installed in the constructor as well. */

   /* NOTE: We are making all functions virtual. This may come in handy when
            you want to derive RRScheduler from this class. */
  
   virtual void yield();
   /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */

   virtual void resume(Thread * _thread);
   /* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */

   virtual void add(Thread * _thread);
   /* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */

   virtual void terminate(Thread * _thread);
   /* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
  
};
	
/*--------------------------------------------------------------------------*/
/* ROUND ROBIN SCHEDULER */
/*--------------------------------------------------------------------------*/

// Inherited Scheduler & Interrupt Handler classes
class RRScheduler: public Scheduler, public InterruptHandler
{
	Queue ready_rr_queue;			// Ready queue for Round-Robin scheduling
	int rr_qsize;						// Robin-Robin ready queue size 
	int ticks;							// No. of ticks since last update
	int hz;								// Frequency of update of ticks
	
	void set_frequency(int _hz);	// setting the interrupt frequency for the timer
	
public:
	RRScheduler();
	// Setting up the Round-Robin scheduler. This sets up the round robin ready queue. The end_of_quantum handler is registered.
	
	virtual void yield();
	/* Invoked by the running thread to yield the CPU. 
   The scheduler selects the next thread from the ready queue and triggers 
   the dispatcher for context switching. */
	
	virtual void resume(Thread * _thread);
	/* Adding the given thread to the ready queue of the RRScheduler. Called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */
	
	virtual void add(Thread * _thread);
	/* Making the given thread runnable by the scheduler. This is called
      after creation of the thread. */
	
	virtual void terminate(Thread * _thread);
	/* Removing the given thread from the scheduler in preparation for destruction
      of the thread. */
	
	virtual void handle_interrupt(REGS * _regs);
	/* The end of Quantum interrupt handler is called using this function. */
};



#endif
