#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/

team_t team = {
    "Malloc lab",
    "jinh",
    "20210914",
    "king",
    "20210914"
    };

// Basic constants and macros:
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE (2 * WSIZE)	 /* Doubleword size (bytes) */
#define CHUNKSIZE (1 << 12)	 /* Extend heap by this amount (bytes) */

/*Max value of 2 values*/
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p. */
#define GET(p) (*(uintptr_t *)(p))
#define PUT(p, val) (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((void *)(bp) - WSIZE)
#define FTRP(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLK(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLK(bp) ((void *)(bp) - GET_SIZE((void *)(bp)-DSIZE))

/* Given ptr in free list, get next and previous ptr in the list */
#define GET_NEXT_PTR(bp) (*(char **)(bp + WSIZE))
#define GET_PREV_PTR(bp) (*(char **)(bp))

/* Puts pointers in the next and previous elements of free list */
#define SET_NEXT_PTR(bp, qp) (GET_NEXT_PTR(bp) = qp)
#define SET_PREV_PTR(bp, qp) (GET_PREV_PTR(bp) = qp)

/* 글로벌 변수 */
static char *heap_listp = 0;
static char *free_list_start = 0;

/* Function prototypes */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/* Function prototypes for maintaining free list*/
static void insert_in_free_list(void *bp);
static void remove_from_free_list(void *bp);

/* heap consistency checker routines: 이건 직접 로직을 구현하진 않고 참고 코드 그대로. */
static void checkblock(void *bp);
static void checkheap(bool verbose);
static void printblock(void *bp);

int mm_init(void)
{
    /* Create the initial empty heap. */
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == NULL) //
        return -1;
    /*
     *  pdding   hdr   prev   next   ftr   epilogue
     * +------+------+------+------+------+-------+
     * |  0   | 16/1 | NULL | NULL | 16/1 |  0/1  |
     * +------+------+------+------+------+-------+
     */
    PUT(heap_listp, 0);							   	   /* padding 만듬 */
    PUT(heap_listp + (1 * WSIZE), PACK(2 * DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), 0); 			       /* 가짜 prev */
    PUT(heap_listp + (3 * WSIZE), 0);	    	       /* 가짜 next */
    PUT(heap_listp + (4 * WSIZE), PACK(2 * DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));	   	   /* Epilogue header */
    free_list_start = heap_listp + 2 * WSIZE;

    /* Extend the empty heap with a free block of minimum possible block size(16 bytes) */
    if (extend_heap(4) == NULL) return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    // minimum block을 16사이즈로 정함
    if (size < 16)	 size = 16;

    /* call for more memory space */
    if ((int)(bp = mem_sbrk(size)) == -1)	return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));		 /* free block header */
    PUT(FTRP(bp), PACK(size, 0));		 /* free block footer */
    PUT(HDRP(NEXT_BLK(bp)), PACK(0, 1)); /* new epilogue header */

    /* coalesce bp with next and previous blocks */
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    //if previous block is allocated or its size is zero then PREV_ALLOC will be set.
    size_t NEXT_ALLOC = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t PREV_ALLOC = GET_ALLOC(FTRP(PREV_BLK(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Next block is only free */
    if (PREV_ALLOC && !NEXT_ALLOC)
    {
        size += GET_SIZE(HDRP(NEXT_BLK(bp)));
        remove_from_free_list(NEXT_BLK(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    /* Prev block is only free */
    else if (!PREV_ALLOC && NEXT_ALLOC)
    {
        size += GET_SIZE(HDRP(PREV_BLK(bp)));
        bp = PREV_BLK(bp);
        remove_from_free_list(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    /* Both blocks are free */
    else if (!PREV_ALLOC && !NEXT_ALLOC)
    {
        size += GET_SIZE(HDRP(PREV_BLK(bp))) + GET_SIZE(HDRP(NEXT_BLK(bp)));
        remove_from_free_list(PREV_BLK(bp));
        remove_from_free_list(NEXT_BLK(bp));
        bp = PREV_BLK(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    insert_in_free_list(bp);
    return bp;
}

void *mm_malloc(size_t size)
{
    size_t asize;	   /* 조정 block size */
    size_t extendsize; /* fit 안 됐으면 늘려야 할 사이즈 */
    void *bp;

    if (size == 0)
        return (NULL);

    // Adjust block size into multiple of 8(bytes)
    if (size <= DSIZE)	asize = 2 * DSIZE;
    else	asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    // first fit
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return (bp);
    }

    /* No fit found.  Get more memory and place the block. */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)	return (NULL);

    place(bp, asize);
    return (bp);
}

static void *find_fit(size_t asize)
{
    void *bp;
    static int last_malloced_size = 0;
    static int repeat_counter = 0;
    // 계속 똑같은 사이즈만 요청했을 때 요청 횟수가 60회가 넘어가면 아예 힙사이즈 확늘린다.(성능을 높이기 위한 방법) 
    if (last_malloced_size == (int)asize)
    {
        if (repeat_counter > 1000)
        {
            int extendsize = MAX(asize, 8 * WSIZE);
            bp = extend_heap(extendsize / 4);
            return bp;
        }
        else
            repeat_counter++;
    }
    else
        repeat_counter = 0;
    // free block list에서 first fit으로 찾기
    for (bp = free_list_start; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT_PTR(bp))
    {
        if (asize <= (size_t)GET_SIZE(HDRP(bp)))
        {
            last_malloced_size = asize;
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= 4 * WSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        remove_from_free_list(bp);
        bp = NEXT_BLK(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        remove_from_free_list(bp);
    }
}

static void insert_in_free_list(void *bp)
{
    SET_PREV_PTR(free_list_start, bp);
    SET_NEXT_PTR(bp, free_list_start);
    SET_PREV_PTR(bp, NULL);
    free_list_start = bp;
}


/*Removes the free block pointer int the free_list*/
static void remove_from_free_list(void *bp)
// 양방향 연결리스트에서 현재 노드 제거
{
    if (GET_PREV_PTR(bp))
        SET_NEXT_PTR(GET_PREV_PTR(bp), GET_NEXT_PTR(bp)); // (bp의 prev)의 next를 (bp의 next)로 설정
    else												   
        free_list_start = GET_NEXT_PTR(bp);				 
    SET_PREV_PTR(GET_NEXT_PTR(bp), GET_PREV_PTR(bp));    // (bp의 next의 prev)를 (bp의 prev)로 설정
}

void mm_free(void *bp)
{
    size_t size;
    if (bp == NULL)
        return;
    /* Free and coalesce the block. */
    size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}


void *mm_realloc(void *bp, size_t size){
    // exception cases
    if ((int)size < 0)  return NULL;
    if ((int)size == 0){
        mm_free(bp);
        return NULL;
    }

    // below: size > 0
    size_t oldsize = GET_SIZE(HDRP(bp));

    // Adjust block size into multiple of 8(bytes)
    size_t asize;
    if (size <= DSIZE)	asize = 2 * DSIZE;
    else	asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    /*if newsize가 oldsize보다 작거나 같으면 그냥 그대로 써도 됨. just return bp */
    if (asize <= oldsize) return bp;

    // next block이 free이고 그 크기를 더했을 때 asize를 감당 가능한 경우
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t csize = GET_SIZE(HDRP(NEXT_BLK(bp))) + oldsize;
    if (!next_alloc &&  (csize >= asize)){
        remove_from_free_list(NEXT_BLK(bp));
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        return bp;
    }

    // newly allocate
    void *new_ptr = mm_malloc(asize);
    place(new_ptr, asize);
    memcpy(new_ptr, bp, asize);
    mm_free(bp);
    return new_ptr;
}