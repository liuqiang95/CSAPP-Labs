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
    "Liu Qiang",
    /* First member's email address */
    "liuqiang2016@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* Print debug info*/
//#define DEBUG
#ifdef DEBUG
#define dbg_print(format, ...)  do {     \
        fprintf( stderr,format"  Line:%d\n", ## __VA_ARGS__, __LINE__ );    \
    }while(0)
#define dbg_begin()  do{ dbg_print("begin %s", __FUNCTION__);}while(0)
#define dbg_end() do{ dbg_print("end %s\n\n", __FUNCTION__);}while(0)
#define dbg_assert(exp) do {    \
        assert((exp));  \
    }while(0)                               
#else
#define dbg_print(format,...) 
#define dbg_assert(exp)  
#define dbg_begin()  
#define dbg_end()                                                 
#endif


/* alignment 8 Bytes /single word (4) / double word (8) */
#define WSIZE 4
#define DSIZE 8
#define ALIGNMENT DSIZE
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount(4KB, one page in Virtual Memory) */

/* block: alloc sign */
#define FREE 0
#define ALLOC 1
#define MAX(x, y) ((x) > (y)? (x) : (y))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* head and foot from: size|0|color|alloc */
#define PACK(bsize, type, color, alloc) ((bsize) | (type) | (color) | (alloc))
#define PUT(p, val) (*(unsigned int *)(p) = val)
#define GET_BSIZE(p) (*(unsigned int *)(p) & ~0x7)
#define GET_ALLOC(p) (*(unsigned int *)(p) & 0x1)

/* Given block ptr bp, compute its header or footer */
#define HDRP(bp) ((char *)(bp) - WSIZE) 
#define FTRP(bp) ((char *)(bp) + GET_BSIZE(HDRP(bp))) 
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BP(bp) ((char *) (bp) + GET_BSIZE(HDRP(bp)) + DSIZE)
#define PREV_BP(bp) ((char *) (bp) - GET_BSIZE(((char *) (bp) - DSIZE)) - DSIZE)

typedef struct DLinkNode{
    /*29 bit store bsize, 2 bit store type and alloc, | 29 | type | 0 | alloc|*/
    unsigned int val;
    struct DLinkNode *next;
    struct DLinkNode *prev;
}DLinkNode;

typedef struct SLinkNode{
    /* 29 bit store bsize, 1 bit store alloc | 29 | 0 | 0 | alloc|,
     we can regonize SLinkNode from size */
    unsigned int val;
    struct SLinkNode *next;
}SLinkNode;

static char *heap_start = NULL; 
static char *heap_end = NULL;
static SLinkNode *slink1_head = NULL;
static DLinkNode *dlink_head = NULL;




/* function about LinkList */
void slink_delete(SLinkNode *snp){
    if(slink1_head == NULL){
        dbg_print("SLinkNode head is NULL");
        return;
    }
    if(slink1_head == snp){
        slink1_head = slink1_head->next;
        return;
    }
    SLinkNode *prev = slink1_head, *ptr = slink1_head->next;
    while(ptr!=snp){
        prev = ptr;
        ptr = ptr->next;
    }
    if(ptr == NULL){
        dbg_print(" SLinkNode can't be  found");
        return;
    }
    prev-> next = snp->next;
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(snp));
}

void slink_insert(SLinkNode *snp){
    if(snp == NULL){
        dbg_print("slink insert erro");
        return;
    }
    if(slink1_head != NULL)
        snp->next = slink1_head;
    slink1_head = snp;
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(snp));
}

/* MIND */

void  check(){
	dbg_print("+++++++++++++++++");
	DLinkNode *dnp = dlink_head;
	while(dnp!=NULL){
		dbg_print("%x  %d", dnp, GET_BSIZE(dnp));
		dbg_assert(dnp->next != dnp);
		dnp = dnp->next;
	}
	dbg_print("+++++++++++++++++");
}
void dlink_delete(DLinkNode *dnp){
	dbg_begin();
    if(dnp ==NULL){
        dbg_print("delete NULL");
        return;
    }
    // delete node is head of 
    if(dnp->prev == NULL){
        if(dnp == dlink_head)
            dlink_head = dnp->next;
        else
            dbg_print("dnode erro");
    }
    else{
         dnp->prev->next = dnp->next;
    }
    if(dnp->next != NULL)
        dnp->next->prev = dnp->prev;
    check();
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(dnp));
}



void dlink_insert(DLinkNode *dnp){
   dbg_begin();
    if(dnp == NULL || GET_BSIZE(dnp) < 2*DSIZE){
        dbg_print("dlink_insert erro");
        return;
    }
    
    if(dlink_head == NULL){
        dlink_head = dnp;
        dnp->prev = NULL;
        dnp->next = NULL;
    }else{
        dnp->next = dlink_head;
        dlink_head->prev = dnp;
        dnp->prev = NULL;
        dlink_head = dnp;
    }
    check();
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(dnp));
}



void erase_free_block(void *bp){
    if(bp == NULL || GET_BSIZE(HDRP(bp)) == 0)
        return;
    if(GET_BSIZE(HDRP(bp)) == ALIGNMENT)
        slink_delete((SLinkNode *)(HDRP(bp)));
    else
        dlink_delete((DLinkNode *)(HDRP(bp)));
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(HDRP(bp)));
}

void updata_block(void *bp, size_t bsize, int alloc){
    if(bp == NULL)
        return;
    PUT((char *)bp - WSIZE, bsize | alloc);
    PUT((char *)bp + bsize, bsize | alloc);
    dbg_assert(GET_BSIZE(HDRP(bp)) == bsize );
}

void *coalesce(void *bp){
    if(bp == NULL)
        return NULL;
   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BP(bp)));    //if 0, free
   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BP(bp)));
   size_t bsize = GET_BSIZE(HDRP(bp));
   dbg_print("begin %s %d B", __FUNCTION__, GET_BSIZE(HDRP(bp)));
   switch ((prev_alloc << 1) | next_alloc){
        case 0: // next and prev both free
            dbg_print("next is free %d B", GET_BSIZE(HDRP(NEXT_BP(bp))));
            bsize += GET_BSIZE(HDRP(NEXT_BP(bp))) + DSIZE;
            erase_free_block(NEXT_BP(bp));
        case 1: // only prev is free
            dbg_print("prev is free %d B", GET_BSIZE(HDRP(PREV_BP(bp))));
            bsize += GET_BSIZE(HDRP(PREV_BP(bp))) + DSIZE;
            erase_free_block(PREV_BP(bp));
            bp = PREV_BP(bp);
            break;
        case 2:// only next is free
            dbg_print("next is free %d B", GET_BSIZE(HDRP(NEXT_BP(bp))));
            bsize += GET_BSIZE(HDRP(NEXT_BP(bp))) + DSIZE;
            erase_free_block(NEXT_BP(bp));
            break;
   }
   /* initializet node info */
   updata_block(bp, bsize, FREE);
   dbg_assert(GET_BSIZE(HDRP(bp)) + WSIZE == FTRP(bp) - HDRP(bp));
   dbg_print("after %s %d B", __FUNCTION__, GET_BSIZE(HDRP(bp)));
   return bp;
};

void *extend_heap(size_t bytes){
	dbg_begin();
    char *bp;
    size_t bsize = ALIGN(bytes);
    /* Allocate an even number of words to maintain alignment */
    if ((bp = mem_sbrk (bsize + DSIZE)) == (void *)-1)
        return NULL;
    updata_block(bp, bsize, FREE);
    PUT(HDRP(NEXT_BP(bp)), ALLOC); /* New epilogue header */
    heap_end = NEXT_BP(bp);
    bp = coalesce(bp);
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(HDRP(bp)));
    return bp;
}

void add_free_block(void *bp){
    if (bp == NULL || GET_BSIZE(HDRP(bp)) == 0)
        return;
    if(GET_BSIZE(HDRP(bp)) == ALIGNMENT)
		slink_insert((SLinkNode *)HDRP(bp));  
	else  
        dlink_insert((DLinkNode *)HDRP(bp));
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(HDRP(bp)));
}

/* allacate and return remain block */
void *split_block(void *bp, size_t bsize){
    size_t old_bsize = GET_BSIZE(HDRP(bp));
    if(bp==NULL||bsize > old_bsize)
        return NULL;
    updata_block(bp, bsize, ALLOC);
    if(old_bsize == bsize)     
        return NULL;
    size_t new_bsize = old_bsize - bsize - DSIZE;
    void *new_bp = NEXT_BP(bp);
    updata_block(new_bp, new_bsize, FREE);
    dbg_print("%s allocte %d B, remain %d B", __FUNCTION__, bsize, new_bsize);
    return new_bp;
}

void *find_fit_block(size_t bsize){
    /* find the ptr pointer to the fit node at first*/
    dbg_begin();
    void *np = NULL, *bp = NULL;
    if(bsize < ALIGNMENT)
    	return NULL;
    else if(bsize == ALIGNMENT){
    	if (slink1_head!=NULL)
    		np = slink1_head;
    }else{
    	DLinkNode *dnp = dlink_head;
    	while(dnp!=NULL){
    		dbg_print("loop %d %d", GET_BSIZE(dnp), bsize);
    		if(GET_BSIZE(dnp)>=bsize){
    			np = dnp;
    			break;
    		}else{
    			dnp = dnp->next;
    		}
    	}
    }
    if(np!=NULL){
        bp = (char *)np + WSIZE;
        dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(HDRP(bp)));
    }
    else{
        dbg_print("can't found fit block for %d B", bsize);
    }
    return bp;
}


void check_block(void *bp){
	dbg_assert(*(unsigned *)(HDRP(bp)) == *(unsigned *)(FTRP(bp)));
}


void check_heap(){
	void *ptr = heap_start;
	if(heap_start == NULL)
		dbg_print("heap_start == NULL");
	dbg_assert(GET_BSIZE(ptr) == 0 && GET_ALLOC(ptr)==ALLOC);
	ptr =  (char *)ptr + WSIZE;
	dbg_assert(GET_BSIZE(ptr) == 0 && GET_ALLOC(ptr)==ALLOC);
	ptr =  (char *)ptr + WSIZE;
	dbg_assert(GET_BSIZE(ptr) == 0 && GET_ALLOC(ptr)==ALLOC);
	void *bp = (char *)ptr + WSIZE;
	while(*(unsigned *)(HDRP(bp)) != ALLOC){
		check_block(bp);
		bp = NEXT_BP(bp);
	}
}

/* 
 * mm_init - initialize the malloc package. 
 * heap 4[4|4][4 | ... | 4][4 |
 * - pad[Prologue][h - ... - f][epilogue
 */
int mm_init(void)
{ 
    dbg_begin();
    heap_start = NULL;
    slink1_head  = NULL;
    dlink_head = NULL;
    if ((heap_start  = mem_sbrk(DSIZE * 2))==(void *)-1)
        return -1;
    PUT(heap_start, ALLOC);  //padding          
    PUT(heap_start + WSIZE, ALLOC);  //Prologue block header
    PUT(heap_start + 2*WSIZE, ALLOC);  //Prologue block footer
    PUT(heap_start + 3*WSIZE, ALLOC); //Epilogue block header
    void *bp;
    if ((bp = extend_heap(CHUNKSIZE)) == NULL){
        heap_start = NULL; // clear heap，back to uninit
        return -1;
    } 
    add_free_block(bp);
    check_heap();
    dbg_print("init: %s apply for %d B",__FUNCTION__,GET_BSIZE(HDRP(bp)));
    return 0;
}

/* 
 * mm_malloc - Allocate a block by search r-b tree, if NOT, extend heap.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t bytes)
{
   
    void *bp, *new_bp;
    size_t bsize, extendsize;
     bsize = ALIGN(bytes);
    dbg_print("%s %d B", __FUNCTION__, bsize);
    /* first init heap */
    if (heap_start == NULL)
        if(mm_init() == -1)
            return NULL;
    /* no need to malloc */
    if(bytes == 0)
        return NULL;
    /* Search r-b tree for a fit */
   
    
    if((bp = find_fit_block( bsize )) == NULL){
        extendsize = MAX(CHUNKSIZE, bsize);
        /* apply for more memory for heap, may two free block adjoin, so need */
        if ((bp = extend_heap(extendsize))== NULL)
            return NULL;
    }
    else{
        erase_free_block(bp);
    }
   // erase_free_block(bp);
    new_bp = split_block(bp, bsize);
    add_free_block(new_bp);
    check_heap();
    dbg_end();
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{   
	
    dbg_print("%s %d B", __FUNCTION__, GET_BSIZE(HDRP(bp)));
    if(heap_start == NULL || bp == NULL || bp <= heap_start || bp >= heap_end)
        return;
    if(GET_ALLOC(HDRP(bp)) == 0)
        return;
    void *new_bp = coalesce(bp);
    add_free_block(new_bp);
    check_heap();
    dbg_end();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t bytes)
{

    if (bp == NULL)
        return mm_malloc(bytes);
    if (bytes == 0)
        return NULL;
    size_t bsize = GET_BSIZE(HDRP(bp));
    size_t new_bsize = ALIGN(bytes);
    dbg_print("%s %d B in %d", __FUNCTION__, new_bsize, bsize);
    void *new_bp;
    if(new_bsize <= bsize){
        /* allocated block after maybe need coalesce */
        new_bp = coalesce(split_block(bp, new_bsize));
        add_free_block(new_bp);
        return bp;
    }
    if((new_bp = mm_malloc(bytes)) == NULL)
        return NULL;
    memcpy(new_bp, bp, bsize);
    mm_free(bp);
    check_heap();
    dbg_end();
    return new_bp;
}

