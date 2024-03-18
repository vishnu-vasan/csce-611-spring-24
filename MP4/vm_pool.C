/*
 File: vm_pool.C
 
 Author: Vishnuvasan Raghuraman
 Date  : 03/17/2024
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    vm_pool_next = NULL;
    num_regions = 0;

    page_table->register_pool(this);   // registering the vm pool

    // first entry storing base address and page size
    alloc_region_info * region = (alloc_region_info*)base_address;
    region[0].base_address = base_address;
    region[0].length = PageTable::PAGE_SIZE;
    vm_regions = region;

    num_regions += 1;

    available_memory -= PageTable::PAGE_SIZE; // calculating available vm

    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned long pages_ct = 0;
    if( _size > available_memory ){
		Console::puts("VMPool::allocate - Not enough vm space available.\n");
		assert(false);
	}
	pages_ct = (_size / PageTable::PAGE_SIZE) + ((_size % PageTable::PAGE_SIZE) > 0 ? 1 : 0); 	// calculating number of pages to be allocated
	// storing details of new vm region
	vm_regions[num_regions].base_address = vm_regions[num_regions-1].base_address +  vm_regions[num_regions-1].length;
	vm_regions[num_regions].length = pages_ct * PageTable::PAGE_SIZE;
	// calculating available memory
	available_memory -= pages_ct * PageTable::PAGE_SIZE;
	num_regions += 1;
    Console::puts("Successfully allocated region of memory.\n");
	
	// returning the allocated base_address
	return vm_regions[num_regions-1].base_address; 
}

void VMPool::release(unsigned long _start_address) {
    int ind = 0;
	int region_no = -1;
	unsigned long page_ct = 0;
	
	// iterating and finding the region the start address belongs to
	for(ind=1;ind<num_regions;ind++){
		if(vm_regions[ind].base_address==_start_address){
			region_no = ind;
		}
	}
	// calculating the number of pages to free
	page_ct = vm_regions[region_no].length / PageTable::PAGE_SIZE;
	while(page_ct > 0){
		// freeing the page
		page_table->free_page(_start_address);
		// incrementing the start address by page size
		_start_address = _start_address + PageTable::PAGE_SIZE;
		page_ct = page_ct - 1;
	}
	// deleting the virtual memory region information
	for(ind=region_no;ind<num_regions;ind++){
		vm_regions[ind] = vm_regions[ind+1];
	}
	available_memory += vm_regions[region_no].length;  // calculating the available memory
	num_regions -= 1;   // decrementing the number of regions
    
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    Console::puts("Checked whether address is part of an allocated region.\n");
    if((_address < base_address) || (_address > (base_address + size))){
        return false;
    }
    return true;
}

