#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
VMPool * PageTable::vm_pool_head = NULL;


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;
   
   Console::puts("Initialized Paging System\n");
}


PageTable::PageTable()
{   
   unsigned int ind = 0;
   unsigned long addr = 0;
   
   // disable paging in the beginning
   paging_enabled = 0;
   
   // calculating no of frames shared: 4MB/4KB = 1024
   unsigned long num_shared_frames = (PageTable::shared_size / PAGE_SIZE);
   
   // initializing page directory
   page_directory = (unsigned long *)(kernel_mem_pool->get_frames(1) * PAGE_SIZE);
   page_directory[num_shared_frames-1] = ( (unsigned long)page_directory | 0b11 );
   
   // Initializing page table
   unsigned long * page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
   
   // initialize PDE and mark first PDE as valid
   // supervisor level, Read and Write and Present bits are set
   page_directory[0] = ((unsigned long)page_table | 0b11);			
   
   // mark remaining PDEs as invalid
   // supervisor level, Read and Write - set and Present bit is not set
   for(ind=1;ind<num_shared_frames-1;ind++)
   {
	   page_directory[ind] = (page_directory[ind] | 0b10);		
   }
   
    // map first 4MB of memory for page table - All pages: valid
	for(ind=0;ind<num_shared_frames;ind++)
   {
	   // Supervisor level, R/W and Present bits are set
	   page_table[ind] = (addr | 0b11);							   
	   addr += PAGE_SIZE;
   }
   
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   
   // Writing page directory address into CR3 register
   write_cr3((unsigned long)(current_page_table->page_directory));
   
   Console::puts("Loaded page table\n");
}


void PageTable::enable_paging()
{
   // set paging bit 
   write_cr0(read_cr0() | 0x80000000);	
   // set paging enabled var
   paging_enabled = 1;		
   
   Console::puts("Enabled paging\n");
}


void PageTable::handle_fault(REGS * _r)
{
	unsigned int ind = 0;
	unsigned long err_code = _r->err_code;
	// get page fault address using CR2 register
	

	// page fault occurs if page is not present
	if ((err_code & 1) == 0)
	{
		unsigned long fault_addr = read_cr2();
		// get page directory address using CR3 register
		unsigned long * page_dir = (unsigned long *)read_cr3();
		// get page directory index: first 10 bits
		unsigned long page_dir_ind = (fault_addr >> 22);
		// get page table index using mask: next 10 bits
		// 0x3FF = 001111111111 - retain only last 10 bits
		unsigned long page_table_ind = ((fault_addr & (0x3FF << 12)) >> 12);		
		unsigned long * new_page_table = NULL;
		unsigned long *new_pde = NULL;

		// checking if logical address is valid and proper
		unsigned int present_flag = 0;

		// Iterating through the VM Pool regions
		VMPool * temp = PageTable::vm_pool_head;

		for(;temp!=NULL;temp=temp->vm_pool_next){
			if(temp->is_legitimate(fault_addr) == true){
				present_flag = 1;
				break;
			}
		}

		if((temp != NULL) && (present_flag == 0)){
			Console::puts("Not a legit address!! \n");
			assert(false);
		}

		// checking where page fault has occured
		if ((page_dir[page_dir_ind] & 1) == 0)
		{
			int ind = 0;
			new_page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
			// PDE = Page Directory Entry
			// PTE = Page Table Entry
			// PDE address = 1023 | 1023 | Offset
			unsigned long * new_pde = (unsigned long *)( 0xFFFFF << 12 );               
			new_pde[page_dir_ind] = ( (unsigned long)(new_page_table)| 0b11 );
			
			// setting flags for each page - PTEs marked as invalid
			for(ind=0;ind<1024;ind++){
				// Set user level flag bit
				new_page_table[ind] = 0b100;
			}
			// Handling invalid PTE scenario to avoid raising another page fault
			new_pde = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);
			// PTE address = 1023 | PDE | Offset
			unsigned long * page_entry = (unsigned long *)( (0x3FF << 22) | (page_dir_ind << 12) );
			
			// marking PTE valid
			page_entry[page_table_ind] = ( (unsigned long)(new_pde) | 0b11 );
		}
		else{
			// Page fault occured in page table page - PDE is present, but PTE is invalid			
			new_pde = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);
			// PTE address = 1023 | PDE | Offset
			unsigned long * page_entry = (unsigned long *)( (0x3FF << 22)| (page_dir_ind << 12) );
			page_entry[page_table_ind] = ( (unsigned long)(new_pde) | 0b11 );
		}
	}

	Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{	
	// registering the initial VM pool
	if(PageTable::vm_pool_head == NULL){
		PageTable::vm_pool_head = _vm_pool;
	}
	// registering subsequent VM pools
	else{
		VMPool * temp = PageTable::vm_pool_head;
		for(;temp->vm_pool_next!=NULL;temp=temp->vm_pool_next);		
		// adding pool to tje end of the linked list
		temp->vm_pool_next = _vm_pool;
	}
    Console::puts("Successfully Registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no)
{
	// extracting page directory index - First 10 bits
	unsigned long page_dir_ind = ( _page_no & 0xFFC00000) >> 22;
	// extracting page table index using mask - next 10 bits
	unsigned long page_table_ind = (_page_no & 0x003FF000 ) >> 12;
	// PTE Address = 1023 | PDE | Offset
	unsigned long * page_table = (unsigned long *) ( (0x000003FF << 22) | (page_dir_ind << 12) );
	// obtaining frame number for releasing
	unsigned long frame_no = ( (page_table[page_table_ind] & 0xFFFFF000) / PAGE_SIZE );
	// releasing frame from process pool
	process_mem_pool->release_frames(frame_no);
	// marking PTE as invalid
	page_table[page_table_ind] = page_table[page_table_ind] | 0b10;
	// flushing TLB by reloading page table
	load();
	Console::puts("Successfully Freed page!\n");
}
