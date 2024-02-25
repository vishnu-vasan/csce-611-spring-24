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
   
   // Initializing page table
   unsigned long * page_table = (unsigned long *)(kernel_mem_pool->get_frames(1) * PAGE_SIZE);
   
   // initialize PDE and mark first PDE as valid
   // supervisor level, Read and Write and Present bits are set
   page_directory[0] = ((unsigned long)page_table | 0b11);			
   
   // mark remaining PDEs as invalid
   // supervisor level, Read and Write - set and Present bit is not set
   for(ind=1;ind<num_shared_frames;ind++)
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
	unsigned long fault_addr = read_cr2();
	// get page directory address using CR3 register
	unsigned long * page_dir = (unsigned long *)read_cr3();
	// get page directory index: first 10 bits
	unsigned long page_dir_ind = (fault_addr >> 22);
	// get page table index using mask: next 10 bits
	// 0x3FF = 001111111111 - retain only last 10 bits
	unsigned long page_table_ind = ((fault_addr >> 12) & 0x3FF);		
	unsigned long * new_page_table = NULL;

	// page fault occurs if page is not present
	if ((err_code & 1) == 0)
	{
		// checking where page fault has occured
		if ((page_dir[page_dir_ind] & 1) == 0)
		{
			// Page fault occured in page directory - PDE is invalid; Allocate a frame from kernel pool for page directory and mark flags - supervisor, R/W, Present bits
			page_dir[page_dir_ind] = (unsigned long)(kernel_mem_pool->get_frames(1)*PAGE_SIZE | 0b11);		// PDE marked valid
			
	        // Clearing last 12 bits to erase all flags using mask
			new_page_table = (unsigned long *)(page_dir[page_dir_ind] & 0xFFFFF000);

			// Setting flags for each page - PTEs marked: invalid
			for(ind=0;ind<1024;ind++)
			{
				// Setting user level flag bit
				new_page_table[ind] = 0b100;
			}
		}

		else
		{
			// Page fault occured in page table page - PDE is present, but PTE is invalid			
			// Clearing last 12 bits to erase all flags using mask
			new_page_table = (unsigned long *)(page_dir[page_dir_ind] & 0xFFFFF000);
			
			// Allocating a frame from process pool and mark flags - supervisor, R/W, Present bits
			new_page_table[page_table_ind] =  PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 0b11;	// PTE marked valid
		}
	}

	Console::puts("handled page fault\n");
}

