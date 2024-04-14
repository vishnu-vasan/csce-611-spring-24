/*
 File: queue.H
 
 Author: Vishnuvasan Raghuraman
 Date  : 04/12/2024
 
 */
#ifndef QUEUE_H
#define QUEUE_H

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"

/*--------------------------------------------------------------------------*/
/* QUEUE DATA STRUCTURE */
/*--------------------------------------------------------------------------*/

class Queue
{
	private:
	Thread* thread;					// Pointer to thread at the top of queue
	Queue* next;					// Pointer to next thread in queue.
	public:
	Queue(){ // Constructor for initial setup
		thread = nullptr;
		next   = nullptr;
	}
	
	Queue(Thread* new_thread){  // Constructor for new threads to enter queue
		thread = new_thread;
		next = nullptr;
	}
	
	// adding thread at the end of queue
	void enqueue(Thread* new_thread){
		// first thread added to queue
		if ( thread == nullptr ){
			thread = new_thread;
		}
		else{
			// traversing the queue
			if( next != nullptr ){
				next->enqueue(new_thread);
			}
			else{
				// reached end of queue - adding new thread at the end of queue
				next =  new Queue(new_thread);
			}
		}
	}
	
	// removing the thread at head position and point to next thread in queue
	Thread* dequeue(){
		// empty queeu
		if( thread == nullptr ){
			return nullptr; 
		}
		// retrieving top of queue element
		Thread *top = thread;
		// when only one queue element present
		if( next == nullptr ){
			thread = nullptr;
		}
		else{
			// updating head element of queue
			thread = next->thread;
			// deleting current node
			Queue * del_node = next;
			// updating next pointer
			next = next->next;
			delete del_node;
		}
		return top;
	}
};

#endif
