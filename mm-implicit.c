// This file gives you a starting point to implement malloc using implicit list
// Each chunk has a header (of type header_t) and does *not* include a footer
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "mm-common.h"
#include "mm-implicit.h"
#include "memlib.h"

// turn "debug" on while you are debugging correctness. 
// Turn it off when you want to measure performance
static bool debug = false;


size_t hdr_size = sizeof(header_t);

void 
init_chunk(header_t *p, size_t csz, bool allocated)
{
	p->size = csz;
	p->allocated = allocated;
}


// Helper function next_chunk returns a pointer to the header of 
// next chunk after the current chunk h.
// It returns NULL if h is the last chunk on the heap.
// If h is NULL, next_chunk returns the first chunk if heap is non-empty, and NULL otherwise.
header_t *
next_chunk(header_t *h)
{
    header_t *n = NULL;
    // Check if h is NULL and heap is non-empty
    if (!h){
        if (mem_heapsize() != 0){
            n = (header_t *)mem_heap_lo();
        }
    }
    else{
	// Increase the pointer to the next header's address
        n = (header_t *)((char *)h + h -> size);

        if ((void *)n > mem_heap_hi()){
            n = NULL;
        }
    }
    
    return n;
}


/* 
 * mm_init initializes the malloc package.
 */
int mm_init(void)
{
	//double check that hdr_size should be 16-byte aligned
	assert(hdr_size == align(hdr_size));
	// start with an empty heap. 
	// no additional initialization code is necessary for implicit list.
	return 0;
}


// helper function first_fit traverses the entire heap chunk by chunk from the begining. 
// It returns the first free chunk encountered whose size is bigger or equal to "csz".  
// It returns NULL if no large enough free chunk is found.
// Please use helper function next_chunk when traversing the heap
header_t *
first_fit(size_t csz)
{
    header_t *cur = (header_t *)mem_heap_lo();
    // Traverse through entire heap to look for chunk free and large enough
    while(cur && (char *)cur < (char *)mem_heap_hi()){
	if (cur -> allocated == 0) {
		coalesce(cur);
	}
        if (cur -> size >= csz && cur -> allocated == 0){
            return cur;
        }
        
        cur = next_chunk(cur);
    }
    
    return NULL;
}

// helper function split cuts the chunk into two chunks. The first chunk is of size "csz", 
// the second chunk contains the remaining bytes. 
// You must check that the size of the original chunk is big enough to enable such a cut.
void
split(header_t *original, size_t csz)
{
    // Check if original chunk is big enough
    if (original -> size >= csz + 32){
        // Initialize second chunk
        init_chunk((header_t *)((char *)original + csz), original -> size - csz, false);
        // Initialize first chunk
        original -> size = csz;
    }
}

// helper function ask_os_for_chunk invokes the mem_sbrk function to ask for a chunk of 
// memory (of size csz) from the "operating system". It initializes the new chunk 
// using helper function init_chunk and returns the initialized chunk.
header_t *
ask_os_for_chunk(size_t csz)
{
    // Set n to the address where os gives the memory
    csz = align(csz);
    header_t *n = (header_t *)(mem_sbrk(csz));
    init_chunk(n, csz, false);
    return n;
}

/* 
 * mm_malloc allocates a memory block of size bytes
 */
void *
mm_malloc(size_t size)
{
	//make requested payload size aligned
	size = align(size);
	//chunk size is aligned because both payload and header sizes
	//are aligned
	size_t csz = hdr_size + align(size);

	header_t *p = NULL;

    // Check if there is available free chunk
    p = first_fit(csz);
	if (!p) {
        // Ask os for new memory
        	
        	p = ask_os_for_chunk(csz);

	}
    
    split(p, csz);
    p -> allocated = 1;
    return p + 1;
	//to obtain a free chunk p to satisfy this request.
	//
	//The code logic should be:
	//Try to find a free chunk using helper function first_fit
	//    If found, split the chunk (using helper function split).
	//    If not found, ask OS for new memory using helper ask_os_for_chunk
	//Set the chunk's status to be allocated


	//After finishing obtaining free chunk p, 
	//check heap correctness to catch bugs
	if (debug) {
		mm_checkheap(true);
	}
	return (void *)p;
}

// Helper function payload_to_header returns a pointer to the 
// chunk header given a pointer to the payload of the chunk 
header_t *
payload2header(void *p)
{
    // Substract header size from payload pointer
    header_t *h = (header_t *)(p - hdr_size);
    return h;
}

// Helper function coalesce merges free chunk h with subsequent 
// consecutive free chunks to become one large free chunk.
// You should use next_chunk when implementing this function
void
coalesce(header_t *h)
{
    // Check if next chunk exists and is free
    while (next_chunk(h)){
        if (next_chunk(h) -> allocated == 0){
            h -> size += next_chunk(h) -> size;
        }
        else{
            return;
        }
    } 
      
}

/*
 * mm_free frees the previously allocated memory block
 */
void 
mm_free(void *p)
{
    header_t *h = payload2header(p);
    h -> allocated = 0;
    coalesce(h);
	// 
	// The code logic should be:
	// Obtain pointer to current chunk using helper payload_to_header 
	// Set current chunk status to "free"
	// Call coalesce() to merge current chunk with subsequent free chunks
	  
	  
	// After freeing the chunk, check heap correctness to catch bugs
	if (debug) {
		mm_checkheap(true);
	}
}	

/*
 * mm_realloc changes the size of the memory block pointed to by ptr to size bytes.  
 * The contents will be unchanged in the range from the start of the region up to the minimum of   
 * the  old  and  new sizes.  If the new size is larger than the old size, the added memory will   
 * not be initialized.  If ptr is NULL, then the call is equivalent  to  malloc(size).
 * if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
 */
void *
mm_realloc(void *ptr, size_t size)
{
	size = align(size);
	if (!ptr){
        	mm_malloc(size);
    	}
    else if (size == 0){
        mm_free(ptr);
    }
    else {
        header_t *r = payload2header(ptr);
	size_t originalsize = r -> size;
	// Coalesce with the next free chunks until r is large enough        
	coalesce(r);
	
	if (r -> size < size + hdr_size){
                // Fail to find enough empty space, malloc new memory
                
                header_t *n = (header_t *)mm_malloc(size) - 1;
                // Copy content from old payload to new
                char *nptr = (char *)(n + 1);
		memcpy(nptr, ptr, originalsize - 16);
                
                mm_free(ptr);
                return (n + 1);
	} else {
		// Split r for a memory size exactly as required
		split(r, size + hdr_size);
		return (header_t *)ptr;
	}

    }
	  
	// Check heap correctness after realloc to catch bugs
	if (debug) {
		mm_checkheap(true);
	}
	return NULL;
}


/*
 * mm_checkheap checks the integrity of the heap and returns a struct containing 
 * basic statistics about the heap. Please use helper function next_chunk when 
 * traversing the heap
 */
heap_info_t 
mm_checkheap(bool verbose) 
{
    // Initialize info
    heap_info_t info;
    info.num_allocated_chunks = 0;
    info.num_free_chunks = 0;
    info.allocated_size = 0;
    info.free_size = 0;
    header_t *cur = (header_t *)mem_heap_lo();
    // Add number and size to info according to if cur is allocated
    while (cur){
        if (cur -> allocated == 0){
            info.num_free_chunks += 1;
            info.free_size += cur -> size;
        }
        else{
            info.num_allocated_chunks += 1;
            info.allocated_size += cur -> size;
        }
        
        cur = next_chunk(cur); 
        }
	// traverse the heap to fill in the fields of info
	
	// correctness of implicit heap amounts to the following assertion.
	assert(mem_heapsize() == (info.allocated_size + info.free_size));
	return info;
}
