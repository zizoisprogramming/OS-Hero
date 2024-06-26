#include<stdio.h>
#include<unistd.h>
#include<stdbool.h>
#include<stdlib.h>
#include <math.h>
struct block
{
    int block_size;
    int start_index;
    bool used;
    struct block* next;
};

struct block* initializeBlocks(int initial_size)
{
  struct block* head=(struct block*)malloc(sizeof(struct block));
  head->block_size=initial_size;
  head->start_index=0;
  head->used=false;
  head->next=NULL;
  return head;
  
}
struct block* allocate_process(struct block* head,int mem_size)
{
     mem_size=pow(2, ceil(log2(mem_size))); //round up to the first power of 2
     struct block* iter=head;
     struct block* min_block=head;
     while(iter) //search for min sufficient block
     {
        if(!iter->used)
        {
        if((iter->block_size>mem_size&&iter->block_size<min_block->block_size )|| min_block->used || min_block->block_size<mem_size)
            min_block=iter;
        if(iter->block_size==mem_size && !iter->used)
        {
        min_block=iter;
        break;
        }
        }
        iter=iter->next;
     }
     if(min_block->used || min_block->block_size < mem_size) return NULL; //if no free block found
     if(min_block->block_size==mem_size) //if no splitting required
     {
        min_block->used=true;
        printf("Allocated block at index %d, size %d\n", min_block->start_index, min_block->block_size);

        return min_block;
     }
     while(min_block->block_size>mem_size) //split till block size=rounded up mem size
     {
        int bSize=min_block->block_size;
        int sIndex=min_block->start_index+(bSize/2);
        struct block* new_block=(struct block*)malloc(sizeof(struct block));
        new_block->used=false;
        new_block->block_size=bSize/2;
        new_block->start_index=sIndex;
        new_block->next=min_block->next;
        min_block->next=new_block;
        min_block->block_size=bSize/2;

     }
     min_block->used=true;
     printf("Allocated block at index %d, size %d\n", min_block->start_index, min_block->block_size);
     return min_block;



}
int find_buddy(int addr,int size)
{
    return addr ^ size;
}
bool deallocate_process(struct block* head,int index)
{
    struct block* iter=head;
    while(iter) //search for memory //can be removed if block* is passed to function
    {
        if(iter->start_index==index) 
        {
            iter->used=false;
            printf("Deallocated block at index %d, size %d\n", iter->start_index, iter->block_size);
            break;
        }
        iter=iter->next;
    }
    bool merge=true;
    while(merge)
    {
        merge=false;
        iter=head;
        while(iter)
        {
            if(!iter->used)
            {
            int buddy=find_buddy(iter->start_index,iter->block_size);
            if(iter->next&&iter->next->start_index==buddy&&!iter->next->used && iter->block_size==iter->next->block_size)
            {
                //merge
                struct block* temp=iter->next;
                iter->block_size=iter->block_size*2;
                iter->next=iter->next->next;
                free(temp);
                merge=true;
            }
            }
            iter=iter->next;
        }
    }

  
}

void display_free_list(struct block* head) {
    struct block* iter = head;
    printf("Free List:\n");
    while (iter) {
        printf("Block at index %d, size %d, ", iter->start_index, iter->block_size);
        if (iter->used) {
            printf("used\n");
        } else {
            printf("unused\n");
        }
        iter = iter->next;
    }
}
