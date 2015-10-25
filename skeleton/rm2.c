/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 *    Revision 1.3  2004/12/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 
 ************************************************************************/

/************************************************************************
 Project Group: NetID1, NetID2, NetID3
 
 ***************************************************************************/

#ifdef KMA_RM
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"


/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

typedef struct{		//save all free blocks in double linkedlist
	int size;
	void* next;
}free_block;

/************Global Variables*********************************************/
// kma_page_t* entry = NULL;		//entry pointer, always points to the free memory

free_block* entry = NULL;
//points to the first page
/************Function Prototypes******************************************/
free_block* firstFit(int size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	printf("size %d\n", size);
	if ((size + sizeof(kma_page_t*)) > PAGESIZE)
	{ 	// requested size too large
		return NULL;
	}

	void* res = NULL;
	if(entry == NULL){	//if linkedlist is null //add it to linked list, it's the head pointer of the linkedlist	
		kma_page_t* page = get_page();
		//save the structure returned from get_page() at the beginning of each page
		*((kma_page_t**)page->ptr) = page; 
		// entry = page->ptr + sizeof(kma_page_t*);
		entry = (free_block*)(page->ptr + sizeof(kma_page_t*) + sizeof(void*));
		// printf("entry 1 %p\n", entry);
		entry->next = NULL;
		entry->size = PAGESIZE - sizeof(kma_page_t*)- sizeof(void*);
		// printf("entry size %d\n", entry->size);
		
		// entry = (free_block*)((long)entry + sizeof(void*));
		// printf("entry 2 %p\n", entry);
	}
	//linkedlist is not null, find the first suitable free block
	//return the node to the free block
	res = (void*) firstFit(size);

	return res;
}

//find the first free block in the linkedlist
free_block* firstFit(int size){
	printf("call firstfit function\n");
	//traverse the linkedlist 
	free_block* temp = entry;	//entry is the head of the linkedlist
	printf("temp size  %d\n", temp->size);
	free_block* pre = NULL;
	free_block* res = NULL;

	while(temp != NULL){
		printf("in while loop\n");
		printf("temp->size    %d\n",temp->size);
		if(temp->size >= size + sizeof(void*)){
			//remove this node from linkedlist
			if(temp->size - size - sizeof(void*) == 0){
				printf("exact size \n");
				if(temp == (free_block*)entry){		//head pointer
					entry = temp->next;
				}
				else if(temp->next == NULL){		//tail pointer
					pre->next = NULL;
				}
				else{
					pre->next = temp->next;
				}
				return temp;
			}
			// printf("actual size larger than needed size\n");
			// printf("pointer temp outside the 1 %p\n", temp);
			// printf("pointer temp outside the 2 %p\n", (free_block*)((long)temp + size));
			//actual size > needed size
			if(pre != NULL){
				res = temp;
				res->size = size;
				res->next = NULL;
				res = (free_block*)((long)res + sizeof(void*));
				temp = (free_block*)((long)temp + size + sizeof(void*));
				pre->next = temp;
			}
			else{
				res = temp;
				entry->size = res->size - size;
				entry->next = res->next;
				temp = (free_block*)((long)temp + size);
				entry = temp;


				// entry = temp;
				// entry->size = temp->size;
				printf("entry %p\n", entry);
				printf("temp size %d\n", temp->size);
				printf("change entry\n");
				
			}
			return res;




			// res = temp;
			// temp = (free_block*)((long)temp + size + sizeof(void*));
			// temp->size = res->size - size;
			// // temp->next = res->next;    ///////////move pointer and the next for pointer also lost
			
			// if(res == NULL){
			// 	printf("res is null\n");
			// }
			// return res;
		}
		printf("outside if \n");
		pre = temp;
		temp = temp->next;

	}
	printf("outside the while loop");
	//if it runs out of free lists, then get a new page
	// if(entry == NULL){
	// 	// printf("use up one page\n");
	// 	return kma_malloc(size);
	// }
	// else{
	// 	//printf("get another new page\n");
	// 	kma_page_t* newpage = get_page();
	// 	newpage->ptr = NULL;
	// 	newpage->size = PAGESIZE;
	// 	pre->ptr = newpage;
	// 	return firstFit(size);
	// }
}

void
kma_free(void* ptr, kma_size_t size)
{
	//add the pointer back to the linkedlist
	printf("call free function %d\n", size);
	free_block* temp = (free_block*)entry;
	free_block* pre = NULL;
	if(entry == NULL){		//the linkedlist is empty
		entry = ptr;
		((free_block*)entry)->next = NULL;
		((free_block*)entry)->size = size;
		return;
	}
	while(temp != NULL){
		// printf("temp->size %d\n",temp->size);
		// printf("in while loop kma free\n");
		if((free_block*)ptr > temp){
			// printf("move pointer forward \n"); 
			pre = temp; 
			temp = temp->next;
		}
		else{
			if(temp == entry){	//insert before the head node  pre == NULL
				printf("insert before the head node\n");
				if((free_block*)ptr + size == temp){
					//merge into one node
					entry = entry - size;
					((free_block*)entry)->size = size + temp->size;
					((free_block*)entry)->next = temp->next;
					printf("merge at head\n");
				}
				else{	//only change the pointer not the size
					printf("insert at head\n");
					((free_block*)ptr)->next = entry;	//insert it at head
					((free_block*)ptr)->size = size;
					entry = (free_block*)ptr;    ////!!!!not sure if it's correct way to move the entry pointer
					printf("size 1,2 %d %d\n", entry->size, ((free_block*)(entry->next))->size);
				}
			}
			else {		//insert node in the middle of the linkedlist 
				printf("insert node in the middle of the linkedlist\n");
				if(ptr + size == temp){ //merge the middle node with latter node
					printf("merge 2 and 3 middle\n");
					((free_block*)ptr)->size = size + temp->size;
					// printf("size 2 and 3 %d\n", ((kma_page_t*)ptr)->size);
					((free_block*)ptr)->next = temp->next;
				}
				else{
					((free_block*)ptr)->size = size;
					((free_block*)ptr)->next = temp;
				}
				//merge the first node and middle node if necceary
				// printf("pre + pre->size: %d\n", entry->size);
				// printf("(kma_page_t*)pt: %p\n", (kma_page_t*)ptr);
				// printf("(temp-size     : %p\n", temp);
				if(pre + pre->size == (free_block*)ptr){
					printf("merge 1 and 2 middle\n");
					pre->size = pre->size + ((free_block*)ptr)->size;
					pre->next = ((free_block*)ptr)->next;
				}
				else{
					pre->next = ptr;
				}
			}
			break;
		}
	}
	if(temp == NULL){		//insert at tail
		if(pre+size == (free_block*)ptr){
			pre->size = size + pre->size;
			printf("merge at tail\n");
		}
		else{
			pre->next = (free_block*)ptr;
			((free_block*)ptr)->next = NULL;
			((free_block*)ptr)->size = size;
		}
	}

	//free page
	// kma_page_t* temp2 = entry;
	// kma_page_t* pre2 = NULL;
	// while(temp2 != NULL && (temp2->size == (PAGESIZE - sizeof(kma_page_t*)))){
	// 	printf("~~~~~~~~~\n");
	// 	pre2 = temp2;
	// 	// if(temp2->size == (PAGESIZE - sizeof(kma_page_t*))){
	// 		printf("free page\n");
	// 		if(temp2 == entry){  //delete head node
	// 			entry = entry->ptr;
	// 			free_page(pre2);
	// 			temp2 = entry;
	// 		}
	// 		else if(temp2->ptr == NULL){ //delete last node
	// 			pre->ptr = NULL;
	// 			free_page(temp2);
	// 			// temp2 = NULL;
	// 		}
	// 		else{
	// 			pre2->ptr = temp2->ptr;
	// 			free_page(temp2);
	// 			temp2 = pre2->ptr;
	// 		}
	// 	// }
	// 	// else{
	// 		// temp2 = temp2->ptr;
	// 	// }
	// }
}



#endif // KMA_RM




				// printf("entry %p\n", entry);
				// printf("entrysize %d\n", entry->size);
				// printf("void* %d\n", sizeof(void*));

				// printf("entrysize %d\n", entry->next);
				// temp = (free_block*)((long)temp + size);
				// int entrysize = entry->size;
				// printf("entrysize %d\n", entry->size);
				// entry->size = entrysize - size - sizeof(void*);
				// entry->next = res->next;
				// entry = temp;



				// entry = (free_block*)((long)entry + sizeof(void*));
				// printf("entry %p\n", entry);
				// res = entry;
				// entry = (free_block*)((long)entry + size);
				// printf("entry %p\n", entry);
				// temp = (free_block*)((long)temp + size);
				// temp = entry;
				

				// printf("entry %p\n", entry);


				// res = (free_block*)((long)temp + size);


				// temp = (free_block*)((long)temp + size);
				// entry = temp;


				// entry = temp;
				// entry->size = temp->size;
				
				// printf("temp size %d\n", temp->size);




// if((tempPageid == temp->page_id) && (ptr + size == temp)){ //merge the middle node with latter node
				// 	printf("merge 2 and 3 middle\n");
				// 	ptr = (void*)((long)ptr - sizeof(void*));
				// 	((free_block*)ptr)->next = temp->next;
				// 	// if(temp->next == NULL){
				// 		printf("merge with the tail\n");
				// 		((free_block*)ptr)->size = size + temp->size + sizeof(void*);
				// 		// printf("tail size%d\n", ptr->size);
				// 	// }
				// 	// else{
				// 	// 	((free_block*)ptr)->size = size + temp->size + sizeof(void*);
				// 	// }
				// 	printf("after merge the size is %d\n", ((free_block*)ptr)->size);
				// 	// printf("size 2 and 3 %d\n", ((kma_page_t*)ptr)->size);
				// }
				// else{
				// 	ptr = (void*)((long)ptr - sizeof(void*));
				// 	((free_block*)ptr)->size = size;
				// 	((free_block*)ptr)->next = temp;
				// }




				// free_block* pre_temp = (free_block*)((long)pre + pre->size + sizeof(void*));
				// if((prePageid == tempPageid) && (pre_temp == ptr)){
				// 	printf("merge 1 and 2 middle\n");
				// 	pre->size = pre->size + ((free_block*)ptr)->size + sizeof(void*);
				// 	pre->next = ((free_block*)ptr)->next;
				// 	printf("after merge the size is %d\n", pre->size);
				// }
				// else{
				// 	pre->next = ptr;
				// }







				
				// if((tempPageid == temp->page_id) && ((free_block*)ptr + size == temp)){
				// 	//merge into one node
				// 	ptr = (void*)((long)ptr - sizeof(void*));
				// 	((free_block*)ptr)->size = size + temp->size + sizeof(void*);
				// 	((free_block*)ptr)->next = temp->next;
				// 	entry = (free_block*)ptr;
				// 	printf("merge at head size is %d\n", entry->size);
				// }
				// else{	//only change the pointer not the size
				// 	printf("insert at head\n");
				// 	ptr = (void*)((long)ptr - sizeof(void*));
				// 	((free_block*)ptr)->next = entry;	//insert it at head
				// 	((free_block*)ptr)->size = size;
				// 	entry = (free_block*)ptr;    ////!!!!not sure if it's correct way to move the entry pointer
				// 	// printf("size 1,2 %d %d\n", entry->size, ((free_block*)(entry->next))->size);
				// }