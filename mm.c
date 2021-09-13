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


///////////////////////////////////////////////////////////
// MACROS in textbook
#define WSIZE 4     // word and header/footer size(bytes)
#define DSIZE 8     // doubld word size(bytes)
#define CHUNKSIZE (1<<12)

#define MAX(x, y)           ((x) > (y)? (x): (y))          // 
#define PACK(size, alloc)   ((size) | (alloc))             //
#define GET(p)              (*(unsigned int *)(p))        // read a word
#define PUT(p, val)         (*(unsigned int *)(p) = (val)) // write a word
#define GET_SIZE(p)         (GET(p) & ~0x7)                // GET(p) & 0b11...11000
#define GET_ALLOC(p)        (GET(p) & 0x1)                // GET(p) & 0b00...00001

// bp: block pointer
#define HDRP(bp)        ((char *)(bp) - WSIZE)         // header pointer of block
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)      // footer pointer of block

#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))      // next block pointer   
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // prev block pointer 

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "chimera",
    /* First member's full name */
    "Genghis Khan",
    /* First member's email address */
    "mongol@empire.com",
    /* Second member's full name (leave blank if none) */
    "jinh",
    /* Second member's email address (leave blank if none) */
    "jinh@test.test"
};

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
// SIZE_TSIZE == 8
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */

// 위치 맞나??
static void *heap_listp = NULL;
static void *next_fit_ptr = NULL;


static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t newsize);
static void place(void *bp, size_t newsize);

int mm_init(void)
{
    // create the initial empty heap
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1;

    PUT(heap_listp, 0);     // alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));  // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));  // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));      // epilogue header
    heap_listp += (2 * WSIZE);                      // bp of prologue block
    // next-fit
    next_fit_ptr = heap_listp;

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
    // // solution in book
    // size_t asize;
    // size_t extendsize;
    // char *bp;

    // if (size == 0) return NULL;
    // if (size <= DSIZE) asize = 2 * DSIZE;
    // else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // // search the free list for a fit
    // if ((bp = find_fit(asize)) != NULL){
    //     place(bp, asize);
    //     return bp;
    // }
    // //no fit
    // extendsize = MAX(asize, CHUNKSIZE);
    // if ((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;

    // place(bp, asize);
    // return bp;
    // ////////////  end of solution in book  /////////////////////////////


    // version1

    // ignore spurious requests
    if (size == 0) return NULL;
    
    // adjust block size to include overhead and alignment reqs.
    // newsize = (size + ALIGNMENT + (ALIGNMENT-1)) & ~0x7
    int newsize = ALIGN(size + SIZE_T_SIZE);

    // search the free list for a fit
    char *bp;
    if ((bp = find_fit(newsize)) != NULL){
        place(bp, newsize);
        // next-fit
        next_fit_ptr = bp;
        return bp;
    }

    // no fit found. Get more memory and place the block
    size_t extendsize = MAX(newsize, CHUNKSIZE);
    // heap extension fails
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    // heap extension success
    place(bp, newsize);

    // next-fit
    next_fit_ptr = bp;
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
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
    // newly allocation fails
    if (newptr == NULL) return NULL;

    // 원래 있던 코드: 에러 유발
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);

    // copy data from original block to new block
    copySize = GET_SIZE(HDRP(oldptr));      // copySize: size of data to be copied
    if (size < copySize) copySize = size;   
    memcpy(newptr, oldptr, copySize);

    mm_free(oldptr);
    return newptr;
}

static void *extend_heap(size_t words){
    // extends the heap with new free block

    char *bp;       // block pointer
    size_t size;

    // allocate an even number of words to maintain alignment(multible of 8bytes)
    // words(int), size(bytes)
    size = (words % 2) ?  (words + 1)  * WSIZE: words * WSIZE;
    // heap extension fails
    if ((long)(bp = mem_sbrk(size)) == 1) return NULL;

    // initialize free block header/ footer and the epiloque header
    PUT(HDRP(bp), PACK(size, 0));         // free block header
    PUT(FTRP(bp), PACK(size, 0));         // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epliogue header

    // coalese if the previous block(prev & next block) was free
    return coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case1: prev(allocated) & next(allocated)
    if (prev_alloc && next_alloc) return bp;

    // case2: prev(allocated) & next(free)
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    // case3: prev(free) & next(allocated)
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // case4: prev(free) & next(free)
    else{
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // next-fit
    next_fit_ptr = bp;
    return bp;
}

// next-fit
static void *find_fit(size_t newsize){
    // my code
    // next-fit version 2

    void *bp = next_fit_ptr;
    char *end = mem_heap_hi();                    // last byte of current heap

    while ((bp < end) && 
            (GET_ALLOC(HDRP(bp)) || newsize > GET_SIZE(HDRP(bp)))){
            bp = NEXT_BLKP(bp);
            }
    if (bp < end) return bp;

    // re-find from first(heap_listp)
    bp = heap_listp;
    while ((bp < next_fit_ptr) && 
            (GET_ALLOC(HDRP(bp)) || newsize > GET_SIZE(HDRP(bp)))){
            bp = NEXT_BLKP(bp);
            }
    // no fit found
    if (bp >= next_fit_ptr) return NULL;
    return bp;
}

// // first-fit
// static void *find_fit(size_t newsize){
//     // my code
//     // first-fit

//     void *bp = heap_listp;
//     char *end = mem_heap_hi();                    // last byte of current heap

//     // find first-fit free block: free & lagre enough
//     while ((bp < end) && 
//             (GET_ALLOC(HDRP(bp)) || newsize > GET_SIZE(HDRP(bp)))){
//             bp = NEXT_BLKP(bp);
//             }

//     // not found
//     if (bp >= end) return NULL;
//     return bp;

//     // // solution in book
//     // void *bp;
//     // for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
//     //     if (!GET_ALLOC(HDRP(bp)) && (newsize <= GET_SIZE(HDRP(bp)))){
//     //         return bp;
//     //     }
//     // }
//     // // no fit
//     // return NULL;
// }


static void place(void *bp, size_t newsize){
    
    // solution in book

    // size of free block
    size_t csize = GET_SIZE(HDRP(bp));

    // spliting: size of remaining free is greater or equal than 16(minimum block size)
    if ((csize - newsize) >= (2 * DSIZE)){
        // allocation block at bp
        PUT(HDRP(bp), PACK(newsize, 1));
        PUT(FTRP(bp), PACK(newsize, 1));
        bp = NEXT_BLKP(bp);

        // setting free block at remaining of original free block
        PUT(HDRP(bp), PACK(csize-newsize, 0));
        PUT(FTRP(bp), PACK(csize-newsize, 0));
    }
    // not splitting
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}