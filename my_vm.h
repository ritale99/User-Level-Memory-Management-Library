#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096

// Maximum size of your memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024 //4GB	//how much we can have, theoretically, virt mem

#define MEMSIZE 1024*1024*1024					//how much we currently have, phys mem

// Represents a page table entry
typedef unsigned long pte_t;	//store phys addr
//typedef void * pte_t;

typedef struct pte_t_node {
	unsigned int index_in_virt_bitmap;
	pte_t base_va_addr;
	pte_t * data;
} pte_t_node;

// Represents a page directory entry
//typedef unsigned long pde_t;	//essentially a pte_t *;
typedef pte_t_node * pde_t;

#define TLB_SIZE 120	//120 entries

//entry of the TLB contains the va, pa and time of insertion
struct tlb_entry{
	void *va;
	void *pa;
	time_t time;
};

//Structure to represents TLB
//Contains array of tlb_entry structs
struct tlb {

    	//Assume your TLB is a direct mapped TLB of TLB_SIZE (entries)
    	//You must also define wth TBL_SIZE in this file.
    	//Assume each bucket to be 4 bytes
	struct tlb_entry entries[TLB_SIZE];


};

struct tlb tlb_store;


typedef struct next_avail_node {
	int index;
	void * addr;
	struct next_avail_node * next;
} next_avail_node;

typedef struct int3_node {
	int outer;
	int inner;
	int offset;
} int3_node;
//NOTE: keep page_dir and bitmap away from memory we are simulating.

//physical memory
char * mem;

//page directory
pde_t * page_dir;

//physical page bit map
char * phys_bit_map;
//virtual page bit map
char * virt_bit_map;

//number of virtual pages
unsigned int virt_page_count;
//number of physical pages
unsigned int phys_page_count;
unsigned int outer_bit_count;
unsigned int inner_bit_count;
unsigned int offset_bit_count;
unsigned int outer_page_count;	//number of page tables we are allow to have
unsigned int inner_page_count;	//number of pages per table we are allow to have

void print_TLB_missrate();
void SetPhysicalMem();
pte_t* Translate(pde_t *pgdir, void *va);
int PageMap(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *myalloc(unsigned int num_bytes);
next_avail_node* get_next_avail_phys (int num_pages);
next_avail_node* get_next_avail_virt (int num_pages);
void myfree(void *va, int size);
void PutVal(void *va, void *val, int size);
void GetVal(void *va, void *val, int size);
void MatMult(void *mat1, void *mat2, int size, void *answer);
double misses;
double accesses;


//helper function

//free a link list
void freeLL(next_avail_node * head);

void FreeSelectedVirt(int index, int size);

void FreeSelectedPhys(next_avail_node * node);

//get 3 decimal value (outer, inner, offset) of the binary string
int3_node * Get3DecimalOfBin(void * addr);

//convert a string of binary value to hexadecimal
unsigned long bintohex(char *bin);

void * ConvertIndexToVA(unsigned int index);

#endif
