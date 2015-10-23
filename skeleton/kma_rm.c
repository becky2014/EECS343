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
	void* pre;
	void* next;
}free_block;

/************Global Variables*********************************************/
kma_page_t* entry = NULL;		//entry pointer, always points to the free memory
//points to the first page
/************Function Prototypes******************************************/
kma_page_t* firstFit(int size);

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
	if(!entry){	//if linkedlist is null //add it to linked list, it's the head pointer of the linkedlist	
		kma_page_t* page = get_page();
		//save the structure returned from get_page() at the beginning of each page
		*((kma_page_t**)page->ptr) = page; 
		entry = page->ptr + sizeof(kma_page_t*);
		free_block* head = (free_block*)(page->ptr + sizeof(kma_page_t*));
		head->size = PAGESIZE - sizeof(kma_page_t*));
		head->next = NULL;
	}
	//linkedlist is not null, find the first suitable free block
	//return the node to the free block
	res = (void*) firstFit(size);
	return res;
}

//find the first free block in the linkedlist
kma_page_t* firstFit(int size){
	//traverse the linkedlist 
	kma_page_t* temp = entry;	//entry is the head of the linkedlist
	kma_page_t* pre = NULL;
	kma_page_t* res = NULL;
	while(temp != NULL){
		if(temp->size < size){
			pre = temp;
			temp = (kma_page_t*)temp->ptr;	//move the pointer to next free node
		}
		else{				   //(temp->size >= size)
			//remove this node from linkedlist
			if(temp->size - size == 0){
				printf("exact size \n");
				if(temp == entry){		//head pointer
					entry = entry->ptr;
				}
				else if(temp->ptr == NULL){		//tail pointer
					pre->ptr = NULL;
				}
				else{
					pre->ptr = temp->ptr;
				}
				return temp;
			}
			res = temp;
			temp = temp + size;
			temp->size = res->size - size;
			temp->ptr = res->ptr;    ///////////move pointer and the next for pointer also lost
			if(pre != NULL){
				pre->ptr = temp;
			}
			else{
				entry = temp;
			}
			return res;
		}
	}
	//if it runs out of free lists, then get a new page
	if(entry == NULL){
		// printf("use up one page\n");
		return kma_malloc(size);
	}
	else{
		//printf("get another new page\n");
		kma_page_t* newpage = get_page();
		newpage->ptr = NULL;
		newpage->size = PAGESIZE;
		pre->ptr = newpage;
		return firstFit(size);
	}
}

void
kma_free(void* ptr, kma_size_t size)
{
	//add the pointer back to the linkedlist
	// printf("call free function %d\n", size);
	kma_page_t* temp = entry;
	kma_page_t* pre = NULL;
	if(entry == NULL){		//the linkedlist is empty
		entry = ptr;
		entry->ptr = NULL;
		entry->size = size;
		return;
	}
	while(temp != NULL){
		// printf("temp->size %d\n",temp->size);
		// printf("in while loop kma free\n");
		if((kma_page_t*)ptr > temp){
			// printf("move pointer forward \n"); 
			pre = temp; 
			temp = (kma_page_t*)temp->ptr;
			continue;
		}
		else{
			if(temp == entry){	//insert before the head node  pre == NULL
				printf("insert before the head node\n");
				if((kma_page_t*)ptr + size == temp){
					//merge into one node
					entry = (kma_page_t*)ptr;
					entry->size = size + temp->size;
					printf("merge at head\n");
				}
				else{	//only change the pointer not the size
					printf("insert at head\n");
					((kma_page_t*)ptr)->ptr = entry;	//insert it at head
					((kma_page_t*)ptr)->size = size;
					entry = (kma_page_t*)ptr;
				}
			}
			else {		//insert node in the middle of the linkedlist 
				printf("insert node in the middle of the linkedlist\n");
				if((kma_page_t*)ptr + size == temp){ //merge the middle node with latter node
					printf("merge 2 and 3 middle\n");
					((kma_page_t*)ptr)->size = size + temp->size;
					// printf("size 2 and 3 %d\n", ((kma_page_t*)ptr)->size);
					((kma_page_t*)ptr)->ptr = temp->ptr;
				}
				else{
					((kma_page_t*)ptr)->size = size;
					((kma_page_t*)ptr)->ptr = temp;
				}
				//merge the first node and middle node if necceary
				// printf("pre + pre->size: %d\n", entry->size);
				// printf("(kma_page_t*)pt: %p\n", (kma_page_t*)ptr);
				// printf("(temp-size     : %p\n", temp);
				if(pre + pre->size == (kma_page_t*)ptr){
					printf("merge 1 and 2 middle\n");
					pre->size = pre->size + ((kma_page_t*)ptr)->size;
					pre->ptr = ((kma_page_t*)ptr)->ptr;
				}
				else{
					pre->ptr = (kma_page_t*)ptr;
				}
			}
			break;
		}
	}
	if(temp == NULL){		//insert at tail
		if(pre+size == (kma_page_t*)ptr){
			pre->size = size + pre->size;
			printf("merge at tail\n");
		}
		else{
			pre->ptr = (kma_page_t*)ptr;
			((kma_page_t*)ptr)->ptr = NULL;
			((kma_page_t*)ptr)->size = size;
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
