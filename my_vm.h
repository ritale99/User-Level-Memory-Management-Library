#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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

// Represents a page directory entry
//typedef unsigned long pde_t;	//essentially a pte_t *;
typedef pte_t * pde_t;

#define TLB_SIZE 120	//120 entries?

//Structure to represents TLB
struct tlb {

    //Assume your TLB is a direct mapped TLB of TBL_SIZE (entries)
    //You must also define wth TBL_SIZE in this file.
    //Assume each bucket to be 4 bytes

};
struct tlb tlb_store;

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


void print_TLB_missrate();
void SetPhysicalMem();
pte_t* Translate(pde_t *pgdir, void *va);
int PageMap(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *myalloc(unsigned int num_bytes);
void myfree(void *va, int size);
void PutVal(void *va, void *val, int size);
void GetVal(void *va, void *val, int size);
void MatMult(void *mat1, void *mat2, int size, void *answer);

//helper function
//convert addr to binary value
char * hextobin(void *addr);

//convert a string of binary value to hexadecimal
void * bintohex(char *bin);

#endif
