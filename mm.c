/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

///////////////////////////////////////////////////////////
// MACROS in textbook
#define WSIZE 4     // word and header/footer size(bytes)
#define DSIZE 8     // doubld word size(bytes)
#define CHUNKSIZE (1<<12)

#define MAX(x, y)           ((x) > (y)? (x): (y))          // ??
#define PACK(size, alloc)   ((size) | (alloc))             //
#define GET(p)              (* (unsigned int *)(p))        // read a word
#define PUT(p, val)         (*(unsigned int *)(p) = (val)) // write a word
#define GET_SIZE(p)         (GET(p) & ~0x7)                // GET(p) & 0b11...11000
#define GET_ALLOC(p)        (GET(p) & 0x01)                // GET(p) & 0b00...00001

// bp: block pointer
#define HDRP(bp)        ((char *)(bp) - WSIZE)         // header pointer of block
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)      // footer pointer of block
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)) - WSIZE)      // next block pointer   
// 괄호 문제 생기면 책 보고 다시 괄호 넣기 
#define PREV_BLKP(bp)   ( (char *)(bp) - GET_SIZE( (char *)(bp) - DSIZE ) ) // prev block pointer 

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */

static heap_listp = NULL;

static void *extend_heap(size_t words){
    char *bp;       // block pointer
    size_t size;

    // allocate an even number of words to maintain alignment
    // words(int), size(bytes)
    size = (words % 2) ?  (words + 1)  * WSIZE: words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == 1) return NULL;

    // initialize free block header/ footer and the epiloque header
    PUT(HDPR(bp), PACK(size, 0));    // free block header
    PUT(FTRP(bp), PACK(size, 0));    // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epliogue header

    // coalese if th eprevious block was free
    return coalece(bp);

}

int mm_init(void)
{
    // create the initial empty heap
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1;

    PUT(heap_listp, 0);     // alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));  // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));  // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));      // epilogue header
    heap_listp += (2 * WSIZE);      // 왜 하필 여기인지..?

    // extend the empty heap with a free block of CHUNKSIZE bytes
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














