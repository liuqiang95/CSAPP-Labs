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
#define MIN(x, y) ((x) < (y)? (x) : (y))


/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* head and foot from: size|0|color|alloc */
#define PACK(bsize, type, color, alloc) ((bsize) | (type) | (color) | (alloc))
#define PUT(p, val) (*(unsigned int *)(p) = val)
#define GET_BSIZE(p) (*(unsigned int *)(p) & ~0x7)
#define GET_ALLOC(p) (*(unsigned int *)(p) & 0x1)


/* block: type sign */
#define LNODE 0
#define TNODE 2
#define GET_TYPE(p) (*(unsigned int *)(p) & 0x2)
/* red-black tree: color sign */
#define BLACK 0
#define RED 4
#define GET_COLOR(p) (*(unsigned int *)(p) & 0x4)


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

typedef struct RBTNode{
    /*29 bit store bsize，3 bit store color, type and alloc flag, | 29 | type | color | alloc| */
    unsigned int val; 
    struct RBTNode *left;
    struct RBTNode *right;
    struct RBTNode *parent;
    DLinkNode *next;
}RBTNode;

static char *heap_start = NULL; 
static char *heap_end = NULL;
static SLinkNode *slink1_head = NULL;
static DLinkNode *dlink2_head = NULL;
static DLinkNode *dlink3_head = NULL;
static RBTNode *rbt_root = NULL;


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


void  check_dlist(DLinkNode *dp){
    dbg_print("+++++++++++++++++");
    DLinkNode *dnp = NULL;
    switch(GET_BSIZE(dp)/ALIGNMENT){
        case 2:
            dnp = dlink2_head;
            break;
        case 3:
            dnp = dlink3_head;
            break;
        default:
            while(GET_TYPE(dp)!=TNODE)
                dp = dp->prev;
            dnp = ((RBTNode *)dp)->next;
            dbg_print("Tree Node: %x %d", dp, GET_BSIZE(dp));
           // dbg_assert(GET_BSIZE(dp) == GET_BSIZE(dnp));
    }
    if(dnp == NULL)
        dbg_print(" this list is NULL");
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
        switch(GET_BSIZE(dnp)/ALIGNMENT){
            case 2:
                if(dnp == dlink2_head)
                    dlink2_head = dnp->next;
                break;
            case 3:
                if(dnp == dlink3_head)
                    dlink3_head = dnp->next;
                break;
            default:
                dbg_print("delete erro");
        }
       
    }
    else{
        if(GET_TYPE(dnp->prev) == TNODE){
            dbg_print("dnp->prev is TNODE");
            ( (RBTNode *) (dnp->prev) )->next = dnp->next;
        }
        else{
            dnp->prev->next = dnp->next;
        }    
    }

    if(dnp->next != NULL){
        dbg_print("dnp->next != NULL");
        dnp->next->prev = dnp->prev;
    }

    check_dlist(dnp);
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(dnp));
}

void dlink_insert(DLinkNode *dnp){
    dbg_begin();
    if(dnp == NULL || GET_BSIZE(dnp) < 2*DSIZE){
        dbg_print("dlink_insert erro");
        return;
    }
    switch(GET_BSIZE(dnp)/ALIGNMENT){
        case 2:
            if(dlink2_head== NULL){
                dnp->prev = dnp->next = NULL;
            }else{
                dnp->next = dlink2_head;
                dlink2_head->prev = dnp;
                dnp->prev = NULL;
            }
            dlink2_head = dnp;
            break;
        case 3:
            if(dlink3_head== NULL){
                dnp->prev = dnp->next = NULL;
            }else{
                dnp->next = dlink3_head;
                dlink3_head->prev = dnp;
                dnp->prev = NULL;
            }
            dlink3_head = dnp;
            break;
        default:
            dbg_print("delete dlink node erro");
            break;
    }
    check_dlist(dnp);
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(dnp));
}


void print_rbt(RBTNode *root){
    if(root==NULL)
        return;
    print_rbt(root->left);
    if(root == rbt_root){
        dbg_print("root:");
        if(root->parent!=NULL){
            dbg_print("!!!root parent: %d",GET_BSIZE(root->parent));
        }
    }
    dbg_print("%d",GET_BSIZE(root));

    print_rbt(root->right);
}

void check_rbt(){
    dbg_print("--------------------------------");
    if(rbt_root == NULL)
        dbg_print("root is NULL");
    print_rbt(rbt_root);
    dbg_print("--------------------------------");
}

void set_color(RBTNode *tnp, int color){
    tnp->val = (tnp->val & ~0x4) | color;
}
void set_type(RBTNode *tnp, int type){
    tnp->val = (tnp->val & ~0x2) | type;
}
int is_left(RBTNode *tnp){
    return tnp->parent->left == tnp;
}
int is_right(RBTNode *tnp){
    return tnp->parent->right == tnp;
}
RBTNode *child(RBTNode *tnp){
    return tnp->left==NULL? tnp->right: tnp->left;
}

/*only need mind parent, left and right, color.
 *case1: p1->right = p2
 *case2: p1 -> down -> p2
 *deal with family before themselves
 */
void swap(RBTNode *p1, RBTNode *p2){
    dbg_begin();
    int clo1 = GET_COLOR(p1), clo2 = GET_COLOR(p2);
    set_color(p2, clo1);
    set_color(p1, clo2);
    if(p1->right != p2){
        RBTNode *p1p = p1->parent, *p2p = p2->parent;
        RBTNode *p1l = p1->left, *p2l =p2->left;
        RBTNode *p1r = p1->right, *p2r = p2->right;

        if(p1p == NULL){
            dbg_assert(p1 == rbt_root);
            rbt_root = p2;
        }
        else if(is_left(p1))
            p1p->left = p2;
        else
            p1p->right = p2;
        if(is_left(p2))
            p2p->left = p1;
        else
            p2p->right = p1;
        p1->parent = p2p;
        p2->parent = p1p;

        if(p2l!=NULL)
            p2l->parent = p1;
        if(p1l!=NULL)
            p1l->parent = p2;
        p1->left = p2l;
        p2->left = p1l;

        if(p2r!=NULL)
            p2r->parent = p1;
        if(p1r!=NULL)
            p1r->parent = p2;
        p1->right = p2r;
        p2->right = p1r;      
    }
    else{
        RBTNode *p1p = p1->parent, *p2r = p2->right;
        RBTNode *p1l = p1->left, *p2l =p2->left; 

        if(p1p == NULL){
            dbg_assert(p1 == rbt_root);
            rbt_root = p2;
        }
        else if(is_left(p1))
            p1p->left = p2;
        else
            p1p->right = p2;
        p2->parent = p1p;

        if(p2l!=NULL)
            p2l->parent = p1;
        if(p1l!=NULL)
            p1l->parent = p2;
        p1->left = p2l;
        p2->left = p1l;

        if(p2r!=NULL)
            p2r->parent = p1;
        p1->right = p2r;

        p1->parent = p2;
        p2->right = p1;
    }
    check_rbt();
}

/*MIND: replace can't change the node position */
RBTNode *replace_at_delete(RBTNode *tnp){
    if(tnp->left == NULL || tnp->right == NULL)
        return tnp;
    RBTNode *new_tnp = tnp->right;
    while (new_tnp->left!=NULL)
        new_tnp=new_tnp->left;
    swap(tnp,new_tnp);

    return tnp;
}

void rbt_delete(RBTNode *tnp)
{
    dbg_begin();
    if(tnp == NULL){
        dbg_print("rbt_delete erro");
    }
    /* make the delete node have one no-empty child */
    tnp = replace_at_delete(tnp);
    if(tnp->parent == NULL){
        dbg_assert(tnp == rbt_root);
        rbt_root =  child(tnp);
        if(rbt_root!=NULL)
            rbt_root->parent = NULL;
    }
    else if(is_left(tnp)){
        tnp->parent->left = child(tnp);
        if(tnp->parent->left!=NULL)
            child(tnp)->parent = tnp->parent;
    }else{
        tnp->parent->right = child(tnp);
        if(tnp->parent->right!=NULL)
            child(tnp)->parent = tnp->parent;
    }
    /* 
     * case 1: node is root, remian: node must has parent
     * case 2: node is red, remian: node must be black, also has sibling
     * case 3: node is black and child(not null) is red, REMAIN: node children are both null
     * case 4 ~ 8, we need consider the color of parent, sibling and its children
     * case 4: siblig is red (sib's children are both black, obviously)
     * case 5: parent is red, sibling's children are both black(both must be null)
     * case 6: all is black (sib's children are both null)
     * case 7: sibling's inner child is red, the other is black
     * case 8: sibling's extern child is red, the other arbitrarily
     */
    check_rbt();
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(tnp));

} 

void dnode_replace_tnode(RBTNode *tnp){

    dbg_assert(GET_BSIZE(tnp) == GET_BSIZE(tnp->next));
    check_dlist(tnp->next);
    RBTNode *new_tnp = (RBTNode *)(tnp->next);
    DLinkNode *next = tnp->next->next;
    RBTNode *parent = tnp->parent, *left = tnp->left, *right = tnp->right;
    if(next!= NULL)
        next->prev = (DLinkNode *)new_tnp;
    if(rbt_root == tnp){
        dbg_assert(parent == NULL);
        rbt_root = new_tnp;
    }
    else if(is_left(tnp)){
        parent->left = new_tnp;
    }
    else{
        parent->right = new_tnp;
    }
    if(right!=NULL)
        right->parent = new_tnp;
    if(left!=NULL)
        left->parent = new_tnp;

    new_tnp->val = tnp->val;
    new_tnp->parent = parent;
    new_tnp->next = next;
    new_tnp->right = right;
    new_tnp->left = left;
    if(next!=NULL)
        check_dlist(next);
    else
        dbg_print("!!!!!!%d ", GET_BSIZE(new_tnp));
  //  dbg_assert(tnp->val==new_tnp->val && (RBTNode *)(tnp->next)->next == new_tnp->next);
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(tnp));

}

/* select the position which can insert:
 * case 1: find rbt node size = bsize, then insert to the doubly linklist
 * case 2：find the last node > bsize and left child is NULL, 
 *          then insert to be left  child 
 * case 3: find the last node < bsize and right child is NULL, 
 *          then insert to be right  child 
 */
RBTNode *rbt_find_last(size_t bsize){
    RBTNode *tnp = rbt_root; 
    while (tnp!=NULL){
        dbg_print("begin loop %d B for %d B", GET_BSIZE(tnp), bsize);
        if (GET_BSIZE(tnp) > bsize){
            if(tnp->left == NULL)
                return tnp;
            else{
                tnp = tnp->left;
                
                dbg_print("left %d B", GET_BSIZE(tnp));
            }
        }
        else if(GET_BSIZE(tnp) < bsize){
            if(tnp->right == NULL)
                return tnp;
            else{
                tnp = tnp->right;
               //  dbg_print("right left loop %d B", GET_BSIZE(tnp));
                 dbg_print("right  %x  %x B", tnp, tnp->right);
            }
        }
        else{
            return tnp;
        }
    }
    if(tnp!=NULL)
        dbg_print(" %d %s %d B", bsize, __FUNCTION__, GET_BSIZE(tnp));
    else
        dbg_print(" %d %s  NULL",bsize, __FUNCTION__);
    return tnp;
}

void rbt_insert(RBTNode *tnp){
    if(tnp == NULL){
        dbg_print("rbt insert erro");
        return;
    }
    size_t bsize = GET_BSIZE(tnp);
    RBTNode *last =  rbt_find_last(bsize);
    if (last!=NULL && GET_BSIZE(last) == bsize){
        dbg_print(" insert rbt double linklist");
        DLinkNode *dnp = (DLinkNode *)tnp;
        dnp->next =  last->next;
        if(last->next != NULL)
            last->next->prev = dnp;
        dnp->prev = (DLinkNode *)last;
        last->next = dnp;
        return;
    } else{
        dbg_print(" insert rbt");
        set_type(tnp, TNODE);
        tnp->left = tnp->right = NULL;
        tnp->next = NULL;
        tnp->parent = last;
        if(last == NULL){
           rbt_root = tnp;
        }
        else if(GET_BSIZE(last) > bsize){
            last->left = tnp;
        }else{
            last->right = tnp;
        }
      //  adjust_after_insert(tnp);
    }
    dbg_print("%s %d B", __FUNCTION__, GET_BSIZE(tnp));
    check_rbt();
    return;
}

/* return ptr maybe RBTNode* or DLinkNode*  */
void *rbt_find_fit_block(size_t bsize){
    RBTNode *tnp = rbt_root; 
    while (tnp!=NULL){
        if (GET_BSIZE(tnp) >= bsize){
            if (tnp->left == NULL || GET_BSIZE(tnp->left) < bsize){
                if(tnp->next!=NULL)
                    return tnp->next;
                else
                    return tnp;   
            }
            else {
                tnp = tnp->left;
            }
        }
        else {
            tnp = tnp->right;
        }
    }
    if(tnp!=NULL)
        dbg_print(" %d %s %d B", bsize, __FUNCTION__, GET_BSIZE(tnp));
    else
        dbg_print("can't find in rbt");
    return tnp;  
}



void *find_fit_block(size_t bsize){
    /* find the ptr pointer to the fit node at first*/
    dbg_begin();
    void *np = NULL, *bp = NULL;
    switch(bsize/ ALIGNMENT){
        case 0:
            dbg_print("find erro 0bytes");
            break;
        case 1:
            if (slink1_head!=NULL){
                np = slink1_head;
                break;
            }
        case 2:
            if (dlink2_head!=NULL){
                np = dlink2_head;
                break;
            }
        case 3:
            if (dlink3_head!=NULL){
                np = dlink3_head;
                break;
            }
        default:
            np = rbt_find_fit_block(bsize);     
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

void add_free_block(void *bp){
    if (bp == NULL || GET_BSIZE(HDRP(bp)) == 0)
        return;
    switch(GET_BSIZE(HDRP(bp))/ ALIGNMENT){
        case 1:
            slink_insert((SLinkNode *)HDRP(bp)); 
            break; 
        case 2:
        case 3:
            dlink_insert((DLinkNode *)HDRP(bp));  
            break;
        default:
            rbt_insert((RBTNode *)HDRP(bp));
            break;
    }
    dbg_print("%s %d B",__FUNCTION__, GET_BSIZE(HDRP(bp)));
}



void erase_free_block(void *bp){
    if(bp == NULL || GET_BSIZE(HDRP(bp)) == 0)
        return;

    switch(GET_BSIZE(HDRP(bp))/ ALIGNMENT){
        case 1:
            slink_delete((SLinkNode *)(HDRP(bp)));
            break;
        case 2:
        case 3:
            dlink_delete((DLinkNode *)(HDRP(bp)));  
            break;
        default:
            if(GET_TYPE(HDRP(bp)) == TNODE){
                if(((RBTNode *)HDRP(bp))-> next!=NULL){
                    dnode_replace_tnode((RBTNode *)HDRP(bp));
                }else{
                    rbt_delete((RBTNode *) (HDRP(bp)));
                }
            }
            else{
                dlink_delete((DLinkNode *)(HDRP(bp)));      
            }
    }
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

void check_block(void *bp){
	dbg_assert(*(unsigned *)(HDRP(bp)) == *(unsigned *)(FTRP(bp)));
}

void check_heap(){
	void *ptr = heap_start;
	if(heap_start == NULL){
		dbg_print("heap_start == NULL");
    }
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

void realloc_check(void *new_bp, void *bp, size_t bsize){
    int num = bsize/WSIZE;
    dbg_print("/////////////////////////////////%s: %x   %x   %d", __FUNCTION__, new_bp, bp, num);
    while(num){
        if(*(int *)new_bp != *(int *)bp){
             dbg_print("prev: %d  %d  %d", num, *((int *)new_bp -1),*((int *)bp-1));
            dbg_print("%d  %d  %d", num, *(int *)new_bp,*(int *)bp);
            new_bp = (int *)new_bp + 1;
            bp = (int *)new_bp + 1;
            dbg_print("next : %d  %d  %d", num, *(int *)new_bp,*(int *)bp);
            dbg_assert(*(int *)new_bp == *(int *)bp);
        }
        new_bp = (int *)new_bp + 1;
        bp = (int *)new_bp + 1;
        num--;
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
    rbt_root = NULL;
    dlink3_head = dlink2_head = NULL;
    slink1_head  = NULL;
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
    if(heap_start == NULL || bp == NULL || (char *)bp <= heap_start || (char *)bp >= heap_end)
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
    dbg_print("`````````````````````%s %x %d B in %d", __FUNCTION__,bp, new_bsize, bsize);
    void *new_bp = bp, *remain_bp = NULL;
    if(new_bsize <= bsize){
        /* allocated block after maybe need coalesce */
        remain_bp = coalesce(split_block(bp, new_bsize));
        add_free_block(remain_bp);
    }
    else{
        if((new_bp = mm_malloc(new_bsize)) == NULL)
            return NULL;
        memcpy(new_bp, bp, bsize);
        dbg_print("**************************");
        realloc_check(new_bp, bp, MIN(bsize, new_bsize));
        mm_free(bp);
    }
    realloc_check(new_bp, bp, MIN(bsize, new_bsize));
    check_heap();
    dbg_end();
    return new_bp;
}

