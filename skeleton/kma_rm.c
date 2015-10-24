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

typedef struct{
	free_block* head;
	// void* next;		//next pointer
}header;

/************Global Variables*********************************************/
// kma_page_t* entry = NULL;		//entry pointer, always points to the free memory

free_block* entry = NULL;
//points to the first page
/************Function Prototypes******************************************/
free_block* firstFit(int size);
free_block* findPosition(int size);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	printf("size %d ++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n", size);
	if ((size + sizeof(kma_page_t*)) > PAGESIZE)
	{ 	// requested size too large
		return NULL;
	}

	void* res = NULL;
	if(entry == NULL){	//if linkedlist is null //add it to linked list, it's the head pointer of the linkedlist	
		kma_page_t* page = get_page();
		//save the structure returned from get_page() at the beginning of each page
		*((kma_page_t**)page->ptr) = page; 
		entry = (free_block*)(page->ptr + sizeof(kma_page_t*));
		entry->size = PAGESIZE - sizeof(kma_page_t*);
		entry->next = NULL;
		printf("entry 1 %p\n", entry);
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
				if(temp == entry){		//head pointer
					entry = temp->next;
				}
				else if(temp->next == NULL){		//tail pointer
					pre->next = NULL;
				}
				else{
					pre->next = temp->next;
				}
				temp = (free_block*)((long)temp + sizeof(void*));
				return temp;
			}
			// printf("actual size larger than needed size\n");
			// printf("pointer temp outside the 1 %p\n", temp);
			// printf("pointer temp outside the 2 %p\n", (free_block*)((long)temp + size));
			//actual size > needed size
			if(pre != NULL){
				int tempsize = temp->size;
				free_block* tempnext = temp->next; //save temp->next
				temp = (free_block*)((long)temp + sizeof(void*));
				res = temp;

				temp = (free_block*)((long)temp + size);
				temp->size = tempsize - sizeof(void*) - size;		//newsize
				printf("temp new size %d\n", temp->size);

				temp->next = tempnext;		//still the old temp->next
				pre->next = temp;
			}
			else{
				int entrysize = temp->size;
				free_block* tempnext = temp->next;
				// printf("entrysize1 %d\n", entrysize);
				temp->size = size;
				temp->next = NULL;
				temp = (free_block*)((long)temp + sizeof(void*));
				// printf("temp %p\n", temp);
				res = temp;
				entry = (free_block*)((long)temp + size);
				entry->next = tempnext;
				entry->size = entrysize - size - sizeof(void*);

				printf("change entry\n");
				
			}
			return res;
		}
		printf("outside if \n");
		pre = temp;
		temp = temp->next;

	}
	printf("outside the while loop\n");
	//if it runs out of free lists, then get a new page
	if(entry == NULL){
		printf("use up one page\n");
		return kma_malloc(size);
	}
	else{
		printf("get another new page\n");
		kma_page_t* newpage = get_page();
		*((kma_page_t**)newpage->ptr) = newpage; 
		free_block* newpage_temp = (free_block*)(newpage->ptr + sizeof(kma_page_t*));
		newpage_temp->size = PAGESIZE - sizeof(kma_page_t*);
		newpage_temp->next = NULL;
		pre->next = newpage_temp;

	// 	newpage->ptr = NULL;
	// 	newpage->size = PAGESIZE;
	// 	pre->ptr = newpage;
		return firstFit(size);
	}
}



void
kma_free(void* ptr, kma_size_t size)
{
	//add the pointer back to the linkedlist
	printf("call free function size -----------------------------------------------%d\n", size);
	free_block* temp = (free_block*)entry;
	free_block* pre = NULL;
	if(entry == NULL){		//the linkedlist is empty
		ptr = (void*)((long)ptr - sizeof(void*));
		entry = (free_block*)ptr;
		entry->next = NULL;
		entry->size = size;
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
					ptr = (void*)((long)ptr - sizeof(void*));
					((free_block*)ptr)->size = size + temp->size + sizeof(void*);
					((free_block*)ptr)->next = temp->next;
					entry = (free_block*)ptr;
					printf("merge at head\n");
				}
				else{	//only change the pointer not the size
					printf("insert at head\n");
					ptr = (void*)((long)ptr - sizeof(void*));
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
					ptr = (void*)((long)ptr - sizeof(void*));
					((free_block*)ptr)->next = temp->next;
					if(temp->next == NULL){
						printf("merge with the tail\n");
						((free_block*)ptr)->size = size + temp->size;
						printf("tail size%d\n", temp->size);
					}
					else{
						((free_block*)ptr)->size = size + temp->size + sizeof(void*);
					}
					
					// printf("size 2 and 3 %d\n", ((kma_page_t*)ptr)->size);
				}
				else{
					ptr = (void*)((long)ptr - sizeof(void*));
					((free_block*)ptr)->size = size;
					((free_block*)ptr)->next = temp;
				}
				//merge the first node and middle node if necceary
				free_block* pre_temp = (free_block*)((long)pre + pre->size + sizeof(void*));
				if(pre_temp == ptr){
					printf("merge 1 and 2 middle\n");
					pre->size = pre->size + ((free_block*)ptr)->size + sizeof(void*);
					pre->next = ((free_block*)ptr)->next;
				}
				else{
					pre->next = ptr;
				}
			}
			break;
		}
	}
	// if(temp == NULL){		//insert at tail
	// 	if(pre+size == (free_block*)ptr){
	// 		pre->size = size + pre->size;
	// 		printf("merge at tail\n");
	// 	}
	// 	else{
	// 		pre->next = (free_block*)ptr;
	// 		((free_block*)ptr)->next = NULL;
	// 		((free_block*)ptr)->size = size;
	// 	}
	// }
	// printf("free page\n");
	//free page
	free_block* freepage = entry;
	free_block* freepage_pre = NULL;
	int freepage_size = PAGESIZE - sizeof(kma_page_t*) - sizeof(void*);

	while(freepage != NULL){

		if(freepage->size == freepage_size){
			if(freepage == entry && freepage->next == NULL){
				freepage = (free_block*)((long)freepage - sizeof(kma_page_t*));
				kma_page_t* temp33 = (kma_page_t*) freepage;

				free_page(temp33);

				entry = NULL;

				return;


			}		//only node

			// printf(" I wanna free you\n");
			if(freepage == entry){  //delete the head node
				printf("free head node\n");
				entry = freepage->next;
				freepage = (free_block*)((long)freepage - sizeof(kma_page_t*));
				kma_page_t* temp33 = (kma_page_t*)freepage;

				free_page(temp33);
				freepage = entry;
				continue;
			}
			else if(freepage->next == NULL){	//tail
				printf("free tail node\n");

				freepage_pre->next = NULL;
				kma_page_t* temp33 = (kma_page_t*)freepage;

				temp33 = (kma_page_t*)((long)temp33 - sizeof(kma_page_t*));
				

				free_page(temp33);
				printf("finish free the page\n");
				// return;
			}
			else{
				printf("free middle node\n");
				freepage_pre->next = freepage->next;
				freepage = (free_block*)((long)freepage - sizeof(kma_page_t*));
				kma_page_t* temp33 = (kma_page_t*)freepage;

				free_page(temp33);
				freepage = freepage_pre->next;
				continue;
			}
		}
		freepage = freepage->next;
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
	}
}



#endif // KMA_RM
