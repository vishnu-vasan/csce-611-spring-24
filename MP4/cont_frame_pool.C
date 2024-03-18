/*
 File: ContFramePool.C
 
 Author: Vishnuvasan Raghuraman
 Date  : 02/23/2024
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

ContFramePool* ContFramePool::head = NULL; 

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

// Getter here
ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no)
{
    unsigned int bitmap_index = _frame_no;
    unsigned char mask = 0x80;
    unsigned char mask_head = 0xC0;

    if((bitmap[bitmap_index] & mask) == 0)
        return FrameState::Used;
    else if((bitmap[bitmap_index] & mask_head) == 0)
        return FrameState::HoS;
    else
        return FrameState::Free;
}
// Setter here
void ContFramePool::set_state(unsigned long _frame_no, FrameState _state)
{
    unsigned int bitmap_index = _frame_no;
    unsigned char mask = 0x80;
    unsigned char mask_head = 0xC0;

    switch(_state){
        case FrameState::Used:
            bitmap[bitmap_index] ^= mask;
            break;
        case FrameState::Free:
            bitmap[bitmap_index] = 0xFF;
            break;
        case FrameState::HoS:
            bitmap[bitmap_index] ^= mask_head;
            break;
    }
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED! -> NOW IMPLEMENTED
    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    n_free_frames = _n_frames;
    info_frame_no = _info_frame_no;

    // If _info_frame_no is zero, management info is stored in the first frame; otherwise, the provided frame is used.
    if(info_frame_no == 0)
        bitmap = (unsigned char *)(base_frame_no * FRAME_SIZE);
    else
        bitmap = (unsigned char *)(info_frame_no * FRAME_SIZE);
    
    // The bitmap needs to occupy the entire frame
    assert((n_frames % 8) == 0);

    // setting all bits in the bitmap to represent a free state
    for(int f_no=0;f_no<_n_frames;f_no++)
        set_state(f_no,FrameState::Free);

    // sesignating the first frame in the kernel pool as used
    if(_info_frame_no == 0){
        set_state(0, FrameState::Used);
        n_free_frames -= 1;
    }

    // Singly LinkedList for the Pool
    if(head == NULL){
        head = this;
        head->next = NULL;
    }
    else{
        ContFramePool *p = NULL;
        for(p=head;p->next!=NULL;p=p->next);
        p->next = this;
        p = this;
        p->next = NULL;
    }
    Console::puts("Frame pool now initialized and ready.\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED! -> NOW IMPLEMENTED
    if(_n_frames > n_free_frames){
        Console::puts("Cannot allocate frames more than the available number.\n");
        assert(false);
    }
    unsigned int f_no = 0;
    unsigned int start_frame = base_frame_no;

    // checking which frames are in use
    p:
    while(get_state(f_no) == FrameState::Used)
        f_no += 1;

    start_frame = base_frame_no + f_no;
    // checking if frames are available after the free frame
    for(unsigned int i=f_no;i<(_n_frames + f_no);i++){
        if(!((start_frame + _n_frames) <= (base_frame_no + n_frames))){
            Console::puts("Continuous Free Frames are NOT AVAILABLE. \n");
            assert(false);
        }
        if(get_state(i) == FrameState::Used){
            // checking for maximum number of frames available
            if(i < base_frame_no + n_frames){
                f_no = i;
                // goto p and start the search for next free frame
                goto p;
            }
            else{
                Console::puts("Continuous Free Frames are NOT AVAILABLE. \n");
                assert(false);
            }
        }
    }
    // allocate frames continuously
    unsigned int k = _n_frames;
    // mark the frame currently used in bitmap as HoS or as used
    while(k != 0){
        if(k == _n_frames)
            set_state(f_no, FrameState::HoS);
        else
            set_state(f_no, FrameState::Used);
        n_free_frames -= 1;
        f_no += 1;
        k -= 1;
    }
    return start_frame;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED! -> NOW IMPLEMENTED
    for(int f_no=_base_frame_no; f_no<_base_frame_no+_n_frames;f_no++){
        unsigned int frame_range = (f_no - base_frame_no);
        set_state(frame_range, FrameState::HoS);
    }
    n_free_frames = n_free_frames - _n_frames;
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED! -> NOW IMPLEMENTED
    ContFramePool *p = NULL;
    p = head;
    while(p != NULL){
        if((_first_frame_no >= p->base_frame_no) && (_first_frame_no <= (p->base_frame_no + p->n_frames - 1))){
            // Provide the first frame number to release frames starting from that frame.
            p->release_frame_range(_first_frame_no);
            break;
        }
        p = p->next;
    }
}

// release frames till the base frame and the next HoS frame
void ContFramePool::release_frame_range(unsigned long _first_frame_no ) {
	unsigned long frame_range = (_first_frame_no - base_frame_no);

    if(!(bitmap[frame_range] == 0x3F)){
        Console::puts("Not a head frame. Cannot release.\n");
        assert(false);
    }
    for(;frame_range<n_frames;){
        set_state(frame_range, FrameState::Free);
        frame_range += 1;
        // increment the number of free frames that are now available
        n_free_frames += 1;
        //stop releasing frames upon encountering HoS frame
        if(get_state(frame_range) == FrameState::HoS)
            break;
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED! -> NOW IMPLEMENTED
    // frame size = 4 * 1024
    // memory -> 8 bits per frame
    return ((_n_frames * 8) / (4 * 1024 * 8)) + (((_n_frames * 8) % (4 * 1024 * 8)) > 0 ? 1:0);
}
