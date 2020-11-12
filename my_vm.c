#include "my_vm.h"

int init;

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;

    /*Part 2 Code here to calculate and print the TLB miss rate*/




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

/*
Function responsible for allocating and setting your physical memory 
*/
void SetPhysicalMem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
	if ( init ) return;

    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

	//malloc mem
	//calculate # of physical and virtual pages
	//	# of virtual pages = MAX_MEMSIZE / size of a single page (pgsize)
	// 	# of physical pages = MEMSIZE / pgsize
	//initialize virtual and physical bitmaps
	//	physical bitmap would be #phys_pages size
	//	virtual bitmap would be #virt_pages size
	unsigned int delta;

	//calculate virtual page count
	virt_page_count = MAX_MEMSIZE / PGSIZE;
	//calculate physical page count
	phys_page_count = MEMSIZE / PGSIZE;

	//calculate offset bit count
	offset_bit_count = (unsigned int) ceil(log(PGSIZE)/log(2));
	if ( offset_bit_count >= 32 ) {
		printf("offset too high\n");
		exit(0);
	}
	//calculate the remain bit to distribute
	delta = 32 - offset_bit_count;

	//calculate inner bit count
	inner_bit_count = delta / 2;
	//calculate outer bit count
	outer_bit_count = delta - inner_bit_count;
	//calculate page table count for outer
	outer_page_count = pow(2, outer_bit_count);
	//calculate page count for inner
	inner_page_count = pow(2, inner_bit_count);

	//allocate mem
	mem = (char*) malloc(sizeof(char) * MEMSIZE);
	//check if mem was allocated
	if ( mem == NULL ) return;
	//allocate page directory
	page_dir = (pde_t*) malloc(sizeof(pde_t) * phys_page_count);
	if(page_dir == NULL) {
		free(mem);
		exit(0);
	}
	//create physical bit map
	phys_bit_map = (char*) malloc(sizeof(char) * phys_page_count);
	if(phys_bit_map == NULL) {
		free(mem);
		free(page_dir);
		exit(0);
	}
	//create virtual bit map
	virt_bit_map = (char*) malloc(sizeof(char) * virt_page_count);
	if(virt_bit_map == NULL){
		free(phys_bit_map);
		free(page_dir);
		free(mem);
		exit(0);
	}

	//initialize
	memset(phys_bit_map, 0, sizeof(char) * phys_page_count);
	memset(virt_bit_map, 0, sizeof(char) * virt_page_count);

	//printf("size of physical memory: %lld\nphysical page count: %lld\nvirtual page count: %lld\n", sizeof(mem), phys_page_count, virt_page_count);
	init = 1;

	//TEMPT
	//in this test sample I allocated 3 things
	//int *, char *, and char *, of size int, char * 12, char * 12
	//respectively. I then allocate space for a temp page table
	//and store each of the created item in order given below.
	//The temp page table is then stored in the 2nd index of the
	//page directory
	void * addr1 = NULL;
	void * addr2 = NULL;
	void * addr3 = NULL;

	int * tempt_int = (int *) malloc(sizeof(int));
	*tempt_int = 90;

	char * char_arr = (char *) malloc(sizeof(char) * 12);
	char_arr = "hello world";

	char * char_arr2 = (char *) malloc(sizeof(char) * 12);
	char_arr2 = "test string";

	addr1 = &char_arr;
	addr2 = &tempt_int;
	addr3 = &char_arr2;

	pte_t * tempt_page_table = (pte_t *) (malloc(sizeof(pte_t)*PGSIZE));
	tempt_page_table[0] = (pte_t)char_arr;
	tempt_page_table[1] = (pte_t)tempt_int;
	tempt_page_table[2] = (pte_t)char_arr2;

	page_dir[1] = tempt_page_table;
	//TEMPT
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address
	if (va == NULL) return NULL;

	pde_t selPgdir;
	pte_t selPgtab;

	//retrieve the bit string for the given address
	char * bin = hextobin(va);
	unsigned int offset = 0;
	unsigned int inner = 0;
	unsigned int outer = 0;
	//counter
	int i = 0;
	//counter to keep track of which bit we are on
	int counter = 0;
	//length of the bin
	size_t length;
	//error checking
	if ( bin == NULL ) {
		printf("an error occurred on 78\n");
		return NULL;
	}

	//get the length of the binary string
	length = strlen(bin);
	//retrieve the value for offset, inner, and outer
	for(i = length - 1 ; i >= 0; i--){
		//reset counter for every time we pass a bit count
		if ( (length - 1 - i) == offset_bit_count ||  (length-1 - i - offset_bit_count) == inner_bit_count ||
				(length - 1 - i - offset_bit_count - inner_bit_count) == outer_bit_count){
			counter = 0;
		}
		if (bin[i] == '1') {
			//get value for offset bit
			if ((length - 1 - i) < offset_bit_count) {
				offset += (unsigned int) pow(2, counter);
			}
			//get value for inner set bit
			else if ((length - 1 - i - offset_bit_count) < inner_bit_count) {
				inner += (unsigned int) pow(2, counter);
			}
			//get value for outer set bit
			else if ((length - 1 - i - offset_bit_count - inner_bit_count)
					< outer_bit_count) {
				outer += (unsigned int) pow(2, counter);
			}
		}
		counter++;
	}
	//debug
	printf("%s\n", bin);
	printf("%d\n%d\n%d\n", offset, inner, outer);
	//free bin string
	free(bin);

	//check if outer's index value is greater than
	//physical page count
	if (outer >= outer_page_count) {
		printf("Requested page directory %d is greater than the physical page count: %d\n", outer, outer_page_count);
		return NULL;
	}
	if (inner >= inner_page_count){
		printf("Requested inner page table %d is greater than the inner page table count %d\n", inner, inner_page_count);
	}
	if ( offset >= PGSIZE ) {
		printf("Requested offset: %d is greater than the PGSIZE: %d\n", offset, PGSIZE);
		return NULL;
	}
	//since pte_t is a void *
	//pte_t points to the starting address of a physical frame
	//we can add the offset to that starting address to find our
	//intended physical page

	printf("return physical address: %x\n", ((pgdir[outer][inner]) + offset));

	//return as pointer to pte_t + offset
	return (pte_t *) ((pgdir[outer][inner]) + offset);
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
PageMap(pde_t *pgdir, void *va, void *pa)
{

	/*HINT: Similar to Translate(), find the page directory (1st level)
	    and page table (2nd-level) indices. If no mapping exists, set the
	    virtual to physical mapping */
	if (va == NULL) return NULL;

	pde_t selPgdir;
	pte_t selPgtab;

	//retrieve the bit string for the given address
	char * bin = hextobin(va);
	unsigned int offset = 0;
	unsigned int inner = 0;
	unsigned int outer = 0;
	//counter
	int i = 0;
	//counter to keep track of which bit we are on
	int counter = 0;
	//length of the bin
	size_t length;
	//error checking
	if ( bin == NULL ) {
		printf("an error occurred on 280\n");
		return NULL;
	}

	//get the length of the binary string
	length = strlen(bin);
	//retrieve the value for offset, inner, and outer
	for(i = length - 1 ; i >= 0; i--){
		//reset counter for every time we pass a bit count
		if ( (length - 1 - i) == offset_bit_count ||  (length-1 - i - offset_bit_count) == inner_bit_count ||
			(length - 1 - i - offset_bit_count - inner_bit_count) == outer_bit_count){
			counter = 0;
		}
		if (bin[i] == '1') {
			//get value for offset bit
			if ((length - 1 - i) < offset_bit_count) {
				offset += (unsigned int) pow(2, counter);
			}
			//get value for inner set bit
			else if ((length - 1 - i - offset_bit_count) < inner_bit_count) {
				inner += (unsigned int) pow(2, counter);
			}
			//get value for outer set bit
			else if ((length - 1 - i - offset_bit_count - inner_bit_count)
					< outer_bit_count) {
				outer += (unsigned int) pow(2, counter);
			}
		}
		counter++;
	}
	//debug
	printf("%s\n", bin);
	printf("%d\n%d\n%d\n", offset, inner, outer);
	//free bin string
	free(bin);
	//check if outer's index value is greater than
	//physical page count
	if (outer >= outer_page_count) {
		printf("Requested page directory %d is greater than the physical page count: %d\n", outer, outer_page_count);
		return NULL;
	}
	if (inner >= inner_page_count){
		printf("Requested inner page table %d is greater than the inner page table count %d\n", inner, inner_page_count);
	}
	if ( offset >= PGSIZE ) {
		printf("Requested offset: %d is greater than the PGSIZE: %d\n", offset, PGSIZE);
		return NULL;
	}
	if(((pgdir[outer][inner]) + offset) == NULL){
	//the mapping doesn't exist, enter it
		*((pgdir[outer][inner]) + offset) = pa;
		printf("New mapping was set");
		return -1;
	}else{
		printf("The mapping exists");
		return 0;
	}
}
/*Function that gets the next available physical page
*/
void *get_next_avail_phys(int num_pages) {

	//Use physical address bitmap to find the next free page
	int curr_page; int num_free = 0;
	for(curr_page = 0; curr_page < phys_page_count; curr_page++){
		if(phys_bit_map[curr_page] == '0'){

			num_free++;
			if(num_free == num_pages){
				return &phys_bit_map[curr_page];
			}
		}
	}

	//could not find free pages
	return NULL;
}

/*Function that gets the next available virtual page
*/
void *get_next_avail_virt(int num_pages) {

	//Use virtual address bitmap to find the next free page
	int curr_page; int num_free = 0;
	for(curr_page = 0; curr_page < virt_page_count; curr_page++){
		if(virt_bit_map[curr_page] == '0'){

			num_free++;
			if(num_free == num_pages){
				return &virt_bit_map[curr_page];
			}
		}
	}

	//could not find free pages
	return NULL;

}

/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *myalloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.
	if ( !init ) SetPhysicalMem();
	//allocate in term of n * pages not n < page_size
	//return first virtual address of the allocated virtual page
	//give out pages not starting at 0x0, only a multiple of 4KB
	//HINT: If the physical memory is not yet initialized, then allocate and initialize.

	//allocate in term of n * pages not n < page_size
	no_pgs = ceil(num_bytes/PGSIZE);
	//return first virtual address of the allocated virtual page
	void* freePages = get_next_avail_virt(no_pages);

	//give out pages not starting at 0x0, only a multiple of 4KB

	//change the bitmap to be in use

	//map a new page

	//mark which physical pages to use

	/* HINT: If the page directory is not initialized, then initialize the
   	page directory. Next, using get_next_avail(), check if there are free pages. If
   	free pages are available, set the bitmaps and map a new page. Note, you will
   	have to mark which physical pages are used. */
    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to 
    getting the values from two matrices, you will perform multiplication and 
    store the result to the "answer array"*/

       
}

//helper function

//return a string of binary from the given hex address
char *hextobin(void * addr) {
	if(addr == NULL) return NULL;
	unsigned int g = (unsigned int) addr;
	char * hexChar = (char *)malloc(sizeof(char) * 32);
	if ( hexChar == NULL ) return NULL;
	itoa(g,hexChar,2);
	return hexChar;
}

//return the address from a given binary string
void * bintohex(char * bin){
	if (bin == NULL) return NULL;
	unsigned int val = 0;
	unsigned int counter = 0;
	size_t length = strlen(bin);
	int i = 0;
	for ( i = 0 ; i < length; i ++){
		if(bin[length - i- 1] == '1'){
			val += (unsigned int)pow(2,counter);
		}
		counter++;
	}
	return (void *) val;
}

