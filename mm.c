#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"
team_t team = {
    "Artists",
    "Parmar Raja Vijay",
    "rajaparmar@cse.iitb.ac.in",
    "Prateek Agarwal",
    "prateekag@cse.iitb.ac.in"
};
#define ALIGNMENT 8
#define IS_ALIGNED(p)  ((((unsigned int)(p)) % ALIGNMENT) == 0)
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define MINSIZE (2*SIZE_T_SIZE +2* ALIGN(sizeof(void *)))
#define PUTSIZEINHEADER(bp,size) ((((size_t *)(bp)))[0] = (size))
#define GETSIZEFROMHEADER(bp) (*(size_t *)((char* )bp))
#define PUTSIZEINFOOTER(bp,size) (((size_t *)((char* )bp+(size&~0x1)-SIZE_T_SIZE))[0] = (size))
#define SETALLOCATEBIT(size) (size|= 0x1)
#define COPYNEXT(dest,src) ((dest)= *(size_t **)  ((char *)src+SIZE_T_SIZE + ALIGN(sizeof(void *)))) 
#define COPYPREVIOUS(dest,src) ((dest)=  *(size_t **)((char *)src+SIZE_T_SIZE ))
#define SETNEXT(dest,src) (*(size_t **)((char *)(dest) +SIZE_T_SIZE + ALIGN(sizeof(void *))) =(size_t *)(src))
#define SETPREVIOUS(dest,src) (*(size_t **)((char *)(dest) +SIZE_T_SIZE ) = (size_t *)(src))
int mm_init(void);
void * mm_malloc(size_t);
void mm_free(void *);
static char * mm_heap;
static char *mm_head;
void *mm_realloc(void *, size_t);
void insertfreelist(void *bp, void * start);
void delete_from_free_list(void *bp);
void * search_free_block(size_t); 

int mm_init(void)
{   
     mm_heap = 0;
    mm_heap = 0;
    mm_head =mem_sbrk(MINSIZE);
    size_t minsize = MINSIZE|0x1;
    PUTSIZEINHEADER(mm_head,minsize);
    PUTSIZEINFOOTER(mm_head,minsize);
    char * X = 0;
   SETNEXT(mm_head, X);
   SETPREVIOUS(mm_head,X);  
    mm_heap = mm_head;
	return 0;
}
void *mm_malloc(size_t payload)
{   
    size_t size = ALIGN(payload)+2*SIZE_T_SIZE;
     long int a = (long int) size- MINSIZE;
	if (a<0) size  = MINSIZE;
    void * bp;
    bp = search_free_block(size);    
     if (bp!= NULL){
     return (char *)bp+SIZE_T_SIZE;}
    bp = mem_sbrk(size);
    if (bp == (void *)-1)	return NULL;
    SETALLOCATEBIT(size);
    PUTSIZEINHEADER(bp,size);
    PUTSIZEINFOOTER(bp,size);
    mm_heap = bp;
    return (char *)bp+SIZE_T_SIZE;
}

void mm_free(void *ptr)
{        void * bp = ((char *)ptr - SIZE_T_SIZE );
        //size_t physicalprevsize =*(size_t *) ( (char *)bp -SIZE_T_SIZE);
        size_t bpsize = GETSIZEFROMHEADER(bp);
        bpsize = bpsize&~0x1;
        
        void * temp ;
        if (bp == mm_heap){
            size_t physicalprevsize =*(size_t *) ( (char *)bp -SIZE_T_SIZE);
		    size_t prevoccupied = physicalprevsize & 0x1; 
            if (prevoccupied){
                PUTSIZEINHEADER(bp,bpsize); 
                PUTSIZEINFOOTER(bp,bpsize);
                insertfreelist(bp,mm_head);
                return;
            }
            else{
                size_t bpsize = *(size_t *) (bp); 
                bpsize = bpsize&~0x1;
                size_t newsize = bpsize + physicalprevsize;
                void * prevblock = (char *) bp - physicalprevsize;
                COPYPREVIOUS(temp,prevblock);
                delete_from_free_list(prevblock);
                PUTSIZEINHEADER(prevblock,newsize);
                PUTSIZEINFOOTER(prevblock, newsize);
                insertfreelist(prevblock,mm_head);
                mm_heap = prevblock;
                return;}
        }
        else{//it is not the last block
            size_t physicalprevsize =*(size_t *) ( (char *)bp -SIZE_T_SIZE);
		    size_t prevoccupied = physicalprevsize & 0x1;
             void * nextblock = (char *)bp + bpsize;
            size_t physicalnextsize = GETSIZEFROMHEADER(nextblock);
            size_t nextoccupied = physicalnextsize& 0x1;
            if (prevoccupied&&nextoccupied){
                PUTSIZEINHEADER(bp,bpsize); 
                PUTSIZEINFOOTER(bp,bpsize);
                insertfreelist(bp,mm_head);                
                return;}
            
            else if (prevoccupied && !nextoccupied){
                size_t newsize = bpsize + physicalnextsize;
                COPYPREVIOUS(temp,nextblock);
                delete_from_free_list(nextblock);
                PUTSIZEINHEADER(bp,newsize);
                PUTSIZEINFOOTER(bp,newsize);
                insertfreelist(bp,mm_head);
                if (nextblock == mm_heap){
                    mm_heap = bp;
                }
                return;
                }  
            else if (!prevoccupied&&nextoccupied){
                void * prevblock = (char *) bp - physicalprevsize;
                size_t newsize =physicalprevsize+ bpsize;
                COPYPREVIOUS(temp,prevblock);
                delete_from_free_list(prevblock);
                PUTSIZEINHEADER(prevblock,newsize);
                PUTSIZEINFOOTER(prevblock,newsize);
                insertfreelist(prevblock, mm_head);
                return;
                
            }
            else{
                void * prevblock = (char *) bp - physicalprevsize;
                size_t newsize = physicalnextsize+ physicalprevsize+bpsize;
                COPYPREVIOUS(temp,prevblock);
                delete_from_free_list(prevblock);
                COPYPREVIOUS(temp,nextblock);
                delete_from_free_list(nextblock);
                PUTSIZEINHEADER(prevblock,newsize);
                PUTSIZEINFOOTER(prevblock, newsize);
                insertfreelist(prevblock,mm_head);
                if (nextblock == mm_heap){
                    mm_heap = prevblock;
                }
                return;
            }          
        }
        return;
        }
void insertfreelist(void * bp, void *head){
    void *nexttohead;
    nexttohead = *(size_t **) ((char *)head +SIZE_T_SIZE + ALIGN(sizeof(void *))) ;
    COPYNEXT(nexttohead,head);
    if(nexttohead==NULL){
        SETNEXT(head,bp);
        SETPREVIOUS(bp,head);
        SETNEXT(bp, NULL);           
    }
    else{
        SETPREVIOUS(bp,head);
        SETNEXT(bp, nexttohead);
        SETNEXT(head,bp);
        SETPREVIOUS(nexttohead, bp);
    }
        return;
}
void delete_from_free_list(void *bp){
    void * next;
    void * prev;
    COPYNEXT(next,bp);
    COPYPREVIOUS(prev,bp);
    if (next != NULL){
        SETNEXT(prev,next);
        SETPREVIOUS(next,prev);
    }
    else        SETNEXT(prev, NULL);
    return ;
}
void * search_free_block(size_t size){
    void * curr;
    COPYNEXT(curr, mm_head);
    while (curr!= NULL){
        size_t freeblocksize  = GETSIZEFROMHEADER(curr);
        long int cmp = (long int) freeblocksize - size;
        if (cmp >= 0){
          
          void * xyz = 0;
          COPYPREVIOUS(xyz,curr);
            delete_from_free_list(curr);
            size_t minsize = MINSIZE;
            size_t remaining = freeblocksize - size;
            long int c = (long int)remaining - minsize;
            if (c >= 0){
                SETALLOCATEBIT(size);
                PUTSIZEINHEADER(curr,size);
                PUTSIZEINFOOTER(curr,size);
                 void * splittedfreeblock;
                splittedfreeblock = (char *) curr +(size&~0x1);
                PUTSIZEINHEADER(splittedfreeblock,remaining);
                PUTSIZEINFOOTER(splittedfreeblock, remaining);
                insertfreelist(splittedfreeblock,xyz);
                if (curr == mm_heap){
                    mm_heap = splittedfreeblock;
                }  
                return curr;        
            }
            else{
                size = freeblocksize;
                SETALLOCATEBIT(size);
                PUTSIZEINHEADER(curr,size);
                PUTSIZEINFOOTER(curr,size);
                return curr;
            }
        }

        void * temp;
		COPYNEXT(temp, curr);
		curr = temp; 

    }
return NULL;
}
void *mm_realloc(void *ptr, size_t payload)
{

    if (ptr == NULL) return mm_malloc(payload);
    if (payload == 0) {
		mm_free(ptr);
		return NULL;}
     void * bp = ((char *)ptr - SIZE_T_SIZE );
     size_t blocksize = GETSIZEFROMHEADER(bp);//it is the currentblock
     blocksize= (blocksize &(~0x1));
     size_t requestedsize = ALIGN(payload)+2*SIZE_T_SIZE;//which user has asked fo
     long int realloc1 = (long int) requestedsize-blocksize;
    if ((bp == mm_heap)&& realloc1 >0){
        size_t extendsize = (size_t) realloc1;
        mem_sbrk(extendsize);
        requestedsize = (requestedsize|0x1);
        PUTSIZEINHEADER(bp, requestedsize);
        PUTSIZEINFOOTER(bp,requestedsize);
        return ptr;
    }
    else if ((bp == mm_heap)&& realloc1 == 0){
        return ptr;}
    
    else if ((bp ==mm_heap )&&(realloc1 <0)) {
        //assert(5==6);
        size_t remainingfree = blocksize - requestedsize;
        long int splittingrequired = (long int) remainingfree - MINSIZE;
         if (splittingrequired>= 0){
                SETALLOCATEBIT(requestedsize);
                PUTSIZEINHEADER(bp,requestedsize);
                PUTSIZEINFOOTER(bp,requestedsize);
                 void * splittedfreeblock;
                splittedfreeblock = (char *) bp +(requestedsize&~0x1);
                PUTSIZEINHEADER(splittedfreeblock,remainingfree);
                PUTSIZEINFOOTER(splittedfreeblock, remainingfree);
                insertfreelist(splittedfreeblock,mm_head);
                mm_heap = splittedfreeblock;
         }              
            return ptr;
    }
    else if(realloc1>=0) {
        
        void * physicalnextblock = 0;
        physicalnextblock = (char *)bp + blocksize;
        size_t physicalnextsize = GETSIZEFROMHEADER(physicalnextblock);
            size_t nextoccupied = physicalnextsize& 0x1;
            long int check = (long int)requestedsize - blocksize- (physicalnextsize&~0x1); 
            long int splittingrequired = (long int) check - MINSIZE;
        if ((nextoccupied)||(check <0)){
            void *oldptr = ptr;
             void * newptr = mm_malloc(payload);
            size_t oldpayload = blocksize - 2*SIZE_T_SIZE;
             memcpy(newptr, oldptr, oldpayload);
             mm_free(oldptr);
             return newptr;
        }
        else if (splittingrequired  <0 ){
            size_t size2 = blocksize+ (physicalnextsize&~0x1);
            SETALLOCATEBIT(size2);
            PUTSIZEINHEADER(bp,size2);
            PUTSIZEINFOOTER(bp,size2);
            if (physicalnextblock == mm_heap){
                mm_heap  = bp;
               
            }
             return ptr;
        }
        else{         
        
                SETALLOCATEBIT(requestedsize);
                PUTSIZEINHEADER(bp,requestedsize);
                PUTSIZEINFOOTER(bp,requestedsize);
                 void * splittedfreeblock;
                splittedfreeblock = (char *) bp +(requestedsize&~0x1);
                size_t remainingfree =(size_t)check ;
                PUTSIZEINHEADER(splittedfreeblock,remainingfree);
                PUTSIZEINFOOTER(splittedfreeblock, remainingfree);
                insertfreelist(splittedfreeblock,mm_head);
                if (physicalnextblock == mm_heap){
                mm_heap  = splittedfreeblock;} 
            return ptr;
        }    
        }
        
    
    else{
        size_t remainingfree = blocksize - requestedsize;
        
        long int splittingrequired = (long int) remainingfree - MINSIZE;
         if (splittingrequired>= 0){
                SETALLOCATEBIT(requestedsize);
                PUTSIZEINHEADER(bp,requestedsize);
                PUTSIZEINFOOTER(bp,requestedsize);
                 void * splittedfreeblock;
                splittedfreeblock = (char *) bp +(requestedsize&~0x1);
                PUTSIZEINHEADER(splittedfreeblock,remainingfree);
                PUTSIZEINFOOTER(splittedfreeblock, remainingfree);
                insertfreelist(splittedfreeblock,mm_head);
         }       
                
            return ptr;

    }
    
    return ptr;}

       
    
    
    
    