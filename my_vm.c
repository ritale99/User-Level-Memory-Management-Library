#include "my_vm.h"

int init;

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int add_TLB(void *va, void *pa) {

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
void print_TLB_missrate() {
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
	if (init)
		return;

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
	offset_bit_count = (unsigned int) ceil(log(PGSIZE) / log(2));
	if (offset_bit_count >= 32) {
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
	if (mem == NULL)
		return;
	//allocate page directory
	page_dir = (pde_t*) malloc(sizeof(pde_t) * phys_page_count);
	if (page_dir == NULL) {
		free(mem);
		exit(0);
	}
	//create physical bit map
	phys_bit_map = (char*) malloc(sizeof(char) * phys_page_count);
	if (phys_bit_map == NULL) {
		free(mem);
		free(page_dir);
		exit(0);
	}
	//create virtual bit map
	virt_bit_map = (char*) malloc(sizeof(char) * virt_page_count);
	if (virt_bit_map == NULL) {
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
	/*void * addr1 = NULL;
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

	pte_t * tempt_page_table = (pte_t *) (malloc(sizeof(pte_t) * PGSIZE));
	tempt_page_table[0] = (pte_t) char_arr;
	tempt_page_table[1] = (pte_t) tempt_int;
	tempt_page_table[2] = (pte_t) char_arr2;

	page_dir[1] = tempt_page_table;*/
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
	if (va == NULL)
		return NULL;

	void* baseVA = NULL;

	pde_t selPgTable_Node = NULL;
	pte_t selPgTable_Entry = NULL;

	int3_node * node = NULL;
	int3_node * base_node = NULL;

	int outer = 0;
	int inner = 0;
	int offset = 0;

	//operate on the given VA
	//get the 3 decimal value of the bin
	node = Get3DecimalOfBin(va);
	if(node == NULL) {
		printf("error at 175\n");
		return NULL;
	}
	outer = node->outer - 1;
	inner = node->inner;
	offset = node->offset;
	//debug
	printf("%d\n%d\n%d\n", offset, inner, outer);
	free(node);

	//check if outer's index value is greater than
	//physical page count
	if (outer >= outer_page_count) {
		printf(
				"Requested page directory %d is greater than the physical page count: %d\n",
				outer, outer_page_count);
		return NULL;
	}
	if (inner >= inner_page_count) {
		printf(
				"Requested inner page table %d is greater than the inner page table count %d\n",
				inner, inner_page_count);
	}
	if (offset >= PGSIZE) {
		printf("Requested offset: %d is greater than the PGSIZE: %d\n", offset,
				PGSIZE);
		return NULL;
	}

	//we need to subtract the address? or the inner and offset
	//by the base (the virtual address we received after calling mymalloc)

	//retrieve the page table at the given outer
	//this page table is just a node
	selPgTable_Node = pgdir[outer];
	if(selPgTable_Node == NULL || selPgTable_Node->data== NULL) {
		printf("there was an error attempting to access selPgTable at %d\n", outer);
		return NULL;
	}
	//get the base page va
	baseVA = (void*) selPgTable_Node->base_va_addr;
	if (baseVA == NULL) {
		printf("base VA is an error\n");
		return NULL;
	}
	//get the 3 decimal value of the bin
	base_node = Get3DecimalOfBin(baseVA);
	if (base_node == NULL) {
		printf("error at 231\n");
		return NULL;
	}
	//subtract the given va with the base VA received
	inner = inner - base_node->inner;
	offset = offset - base_node->offset;

	//since pte_t is a void *
	//pte_t points to the starting address of a physical frame
	//we can add the offset to that starting address to find our
	//intended physical page

	printf("return physical address: %x\n", ((pgdir[outer]->data[inner]) + offset));

	//we are freeing the 3 decimal of bin not the actual entry
	free(base_node);
	//return as pointer to pte_t + offset
	return (pte_t *) ((pgdir[outer]->data[inner]) + offset);
}

/*
 The function takes a page directory address, virtual address, physical address
 as an argument, and sets a page table entry. This function will walk the page
 directory to see if there is an existing mapping for a virtual address. If the
 virtual address is not present, then a new entry will be added
 */
int PageMap(pde_t *pgdir, void *va, void *pa) {

	/*HINT: Similar to Translate(), find the page directory (1st level)
	 and page table (2nd-level) indices. If no mapping exists, set the
	 virtual to physical mapping */
	if (va == NULL || pa == NULL)
		return -1;

	unsigned int outer = 0, inner = 0, offset = 0, baseOuter = 0, baseInner = 0, baseOffset = 0;

	int3_node * node = NULL;

	pte_t_node * selpgtable_node = NULL;
	pte_t * selpgtable = NULL;

	//extract the 3 values
	node = Get3DecimalOfBin(va);
	//return if node could not be created
	if(node == NULL){
		return -1;
	}
	//since we could not allocate on page dir 0th index
	//we needed to increase the outer by 1, thus when we
	//access the value we need to store at outer - 1
	outer = node->outer - 1;
	inner = node->inner;
	offset = node->offset;
	free(node);
	//bound check
	if (outer >= outer_page_count) {
		//debug
		printf("Requested page directory %d is greater than the physical page count: %d\n",
				outer, outer_page_count);
		return -1;
	}
	if (inner >= inner_page_count) {
		//debug
		printf("Requested inner page table %d is greater than the inner page table count %d\n",
				inner, inner_page_count);
		return -1;
	}
	if (offset >= PGSIZE) {
		//debug
		printf("Requested offset: %d is greater than the PGSIZE: %d\n", offset,
		PGSIZE);
		return -1;
	}
	selpgtable_node = pgdir[outer];
	//check if page table exist in page directory using outer
	//if no page table exist at outer, then create a new one
	if(selpgtable_node == NULL){
		pte_t_node * pte_node = (pte_t_node *) malloc(sizeof(pte_t_node));
		pte_t * pgtable = NULL;
		unsigned int pgtable_size = pow(2, inner_bit_count);
		if(pte_node == NULL) return -1;
		//i am assuming we allocate page table as big as inner_bit_count
		pgtable = (pte_t*) malloc(sizeof(pte_t) * pgtable_size);
		if(pgtable == NULL){
			free(pte_node);
			return -1;
		}
		memset(pgtable, NULL, sizeof(pte_t) * pgtable_size);
		//assign pgtable and va to pte_node
		pte_node->data = pgtable;
		pte_node->base_va_addr = (unsigned long)va;
		//set outer entry in pgdir to be pte_node
		pgdir[outer] = pte_node;
		//set the first index of pgtable equal to pa,
		//since va -> pa, and va is base virtual address
		pgtable[0] = (unsigned long)pa;
		return 0;
	}
	selpgtable = selpgtable_node->data;
	//get the 3 decimal value of base va at this pagetable
	node = Get3DecimalOfBin((void*)selpgtable_node->base_va_addr);
	if(node == NULL) return -1;
	baseOuter = node->outer; baseInner = node->inner; baseOffset = node->offset;
	free(node);
	inner -= baseInner;
	//check if if page at inner exist
	void* addr = (void*)(selpgtable[inner]);
	if((void*)(selpgtable[inner]) == NULL){
		selpgtable[inner] = (unsigned long)pa;
		return 0;
	}
	//return since there is nothing to do
/*	void* baseVA = NULL;

	pde_t selPgTable_Node = NULL;
	pte_t selPgTable_Entry = NULL;

	int3_node * node = NULL;
	int3_node * node_base = NULL;

	int outer = 0;
	int inner = 0;
	int offset = 0;

	//retrieve the bit string for the given address
	char * bin = hextobin(va);
	if (bin == NULL) {
		printf("an error occurred on 280\n");
		free(node);
		return -1;
	}
	//get the 3 decimal value of the bin
	node = Get3DecimalOfBin(bin);
	if(node == NULL) {
		printf("error at 239\n");
		free(bin);
		return -1;
	}
	outer = node->outer;
	inner = node->inner;
	offset = node->offset;
	//debug
	printf("%s\n", bin);
	printf("%d\n%d\n%d\n", offset, inner, outer);
	//free bin string
	free(bin);
	//check if outer's index value is greater than
	//physical page count
	if (outer >= outer_page_count) {
		printf(
				"Requested page directory %d is greater than the physical page count: %d\n",
				outer, outer_page_count);
		return -1;
	}
	if (inner >= inner_page_count) {
		printf(
				"Requested inner page table %d is greater than the inner page table count %d\n",
				inner, inner_page_count);
	}
	if (offset >= PGSIZE) {
		printf("Requested offset: %d is greater than the PGSIZE: %d\n", offset,
				PGSIZE);
		return -1;
	}

	//get the page table node at outer index
	selPgTable_Node = pgdir[outer];
	//error checkingn
	if (selPgTable_Node == NULL || selPgTable_Node->data == NULL) {
		//debug
		printf("there was an error attempting to access selPgTable at %d\n",
				outer);
		return -1;
	}

	//this can probably be removed
	if (selPgTable_Node->size == 0) {
		//debug
		printf("there is nothing stored in this page table\n");
		return -1;
	}

	//get the base va given the outer we received from va
	baseVA = (void*) selPgTable_Node->base_va_addr;
	//check null
	if (baseVA == NULL) {
		//debug
		printf("base VA is an error at 324\n");
		return -1;
	}
	bin = hextobin(va);
	if (bin == NULL) {
		//debug
		printf("error at 329\n");
		return -1;
	}
	//get the 3 decimal value of the bin
	node_base = Get3DecimalOfBin(bin);
	if (node_base == NULL) {
		//debug
		printf("error at 335\n");
		free(bin);
		return -1;
	}
	free(bin);
	//subtract the given va with the base VA received
	inner = inner - node_base->inner;
	offset = offset - node_base->offset;

	//check if within bound
	if(inner >= selPgTable_Node->size){
		//deubg
		printf("out of bound\n");
		return -1;
	}

	//are we just doing replacement are do we extend the selected page table?
	if ((void*)((pgdir[outer]->data[inner])) == NULL) {
		//the mapping doesn't exist, enter it
		pgdir[outer]->data[inner] = (pte_t)pa;
		printf("New mapping was set");
		return -1;
	} else {
		printf("The mapping exists");
		return 0;
	}
	*/
	return 0;
}
/*Function that gets the next available physical page
 */
next_avail_node* get_next_avail_phys(int num_pages) {

	//this is looking for free pages, find the starting
	//address of the continuous free pages that matches
	//num_pages
	next_avail_node* node = (next_avail_node*)malloc(sizeof(next_avail_node));
	if(node == NULL) return NULL;

	int curr_page;
	int num_free = 0;
	int first_page = -1;
	int i = 0;
	//tell us if last page was free, so we know
	//if next page, if free, is continuous
	char last_page_free = 0;
	for (curr_page = 0; curr_page < phys_page_count; curr_page++) {
		//toggle last_page_free if curr_page is not free
		if (phys_bit_map[curr_page] == 1) {
			last_page_free = 0;
		}
		//check if current page is free and it is continuous
		if (phys_bit_map[curr_page] == 0 && last_page_free) {
			num_free++;
			if (num_free == num_pages) {
				//return page address of first page
				node->addr = &mem[first_page * PGSIZE];
				node->index = first_page;
				node->next = NULL;
				for(i = 0 ; i < (curr_page - first_page); i++)
					phys_bit_map[first_page + i] = 1;
				return node;
			}
		}
		//check if current page is free and last page was not free
		else if (phys_bit_map[curr_page] == 0 && !last_page_free) {
			//toggle last page free
			last_page_free = 1;
			num_free = 1;
			first_page = curr_page;
			if (num_free == num_pages){
				node->addr = &mem[curr_page * PGSIZE];
				node->index = curr_page;
				node->next = NULL;
				phys_bit_map[first_page] = 1;
				return node;
			}
		}
	}
	//could not find free pages
	return NULL;
}

/*Function that gets the next available virtual page
 */
next_avail_node * get_next_avail_virt(int num_pages) {

	//Use virtual address bitmap to find the next free continuous pages
	//matching num_pages
	next_avail_node* node = (next_avail_node*) malloc(sizeof(next_avail_node));
	if (node == NULL) return NULL;
	//this is looking for free pages, find the starting
	//address of the continuous free pages that matches
	//num_pages
	int curr_page;
	int num_free = 0;
	int first_page = -1;
	int i = 0;
	//tell us if last page was free, so we know
	//if next page, if free, is continuous
	char last_page_free = 0;
	for (curr_page = 0; curr_page < virt_page_count; curr_page++) {
		//toggle last_page_free if curr_page is not free
		if (virt_bit_map[curr_page] == 1) {
			last_page_free = 0;
		}
		//check if current page is free and it is continuous
		if (virt_bit_map[curr_page] == 0 && last_page_free) {
			num_free++;
			if (num_free == num_pages) {
				node->addr = ConvertIndexToVA(first_page + 1);
				if(node->addr == NULL) {
					free(node);
					return NULL;
				}
				node->index = first_page;
				node->next = NULL;
				for( i = 0 ; i < (curr_page - first_page); i++)
					virt_bit_map[first_page+i] = 1;
				//return page address of first page
				return node;
			}
		}
		//check if current page is free and last page was not free
		else if (virt_bit_map[curr_page] == 0 && !last_page_free) {
			//toggle last page free
			last_page_free = 1;
			num_free = 1;
			first_page = curr_page;
			if (num_free == num_pages) {
				node->addr = ConvertIndexToVA(first_page + 1);
				if (node->addr == NULL) {
					free(node);
					return NULL;
				}
				node->index = first_page;
				node->next = NULL;
				virt_bit_map[first_page] = 1;
				return node;
			}
		}
	}

	//could not find free pages
	return NULL;

}

/* Function responsible for allocating pages
 and used by the benchmark
 */
void *myalloc(unsigned int num_bytes) {

	//HINT: If the physical memory is not yet initialized, then allocate and initialize.
	if (!init)
		SetPhysicalMem();
	//allocate in term of n * pages not n < page_size
	//return first virtual address of the allocated virtual page
	//give out pages not starting at 0x0, only a multiple of 4KB
	//HINT: If the physical memory is not yet initialized, then allocate and initialize.

	if(page_dir == NULL || mem == NULL) return NULL;

	//allocate in term of n * pages not n < page_size
	//determine number of page to allocate
	pte_t * pgtable = NULL;

	next_avail_node* ll_phys_node = NULL;
	next_avail_node* pointer_ll_phys_node = NULL;
	next_avail_node* virtNode = NULL;
	next_avail_node* physNode = NULL;
	next_avail_node* node = NULL;

	int3_node* virt3DecimalNode = NULL;
	void * virt_addr = NULL;
	void * ret_virt_addr = NULL;
	unsigned long virt_addr_inner_incremental = pow(2,offset_bit_count);

	int no_pgs = 0, i = 0, index = 0;

	//calculate the number of pages to allocate
	no_pgs = ceil((float)num_bytes / PGSIZE);

	if (no_pgs == 0) return NULL;
	//find consecutive virtual pages satisfying no_pgs
	virtNode = get_next_avail_virt(no_pgs);
	//return if we could not
	if(virtNode == NULL) return NULL;
	virt_addr = virtNode->addr;
	ret_virt_addr = virt_addr;
	//get all the physical pages we need to back up the
	//virtual pages
	for(i = 0; i < no_pgs; i ++){
		//get 1 free physical page from mem as indicated
		//by the value in phys_bit_map
		physNode = get_next_avail_phys(1);
		//check if it was successful
		//if not then return
		if (physNode == NULL) {
			free(pgtable);

			if(ll_phys_node != NULL){
				//release all physical pages we've selected
				FreeSelectedPhys(ll_phys_node);
				//release all virtual page we've selected
				FreeSelectedVirt(virtNode->index, no_pgs);
			}
			freeLL(ll_phys_node);
			freeLL(virtNode);
			return NULL;
		}
		//create a link list so we can reference it later
		if (ll_phys_node == NULL) {
			ll_phys_node = physNode;
			pointer_ll_phys_node = ll_phys_node;
		} else {
			pointer_ll_phys_node->next = physNode;
			pointer_ll_phys_node = physNode;
		}
	}
	//if there exist no list of physical node (ie no physical page)
	//then return
	if(ll_phys_node == NULL){
		FreeSelectedVirt(virtNode->index, no_pgs);
		freeLL(virtNode);
		return NULL;
	}
	//create a mapping for each virtual page to physical page
	//in ll_phys_nod
	node = ll_phys_node;
	virt3DecimalNode = Get3DecimalOfBin(virtNode->addr);
	if(virt3DecimalNode == NULL){
		FreeSelectedPhys(ll_phys_node);
		FreeSelectedVirt(virtNode->index, no_pgs);
		freeLL(ll_phys_node);
		freeLL(virtNode);
		return NULL;
	}
	unsigned int outer = virt3DecimalNode->outer - 1;
	pte_t_node * pteNode = NULL;
	while(node != NULL){
		if (PageMap(page_dir, virt_addr, node->addr) < 0) {
			if (virt_addr != virtNode->addr) {
				//free the page table in pg dir
				FreeSelectedPhys(ll_phys_node);
				FreeSelectedVirt(virtNode->index, no_pgs);
				if(page_dir[outer] != NULL){
					free(page_dir[outer]->data);
					pteNode = page_dir[outer];
					free(pteNode);
					page_dir[outer] = NULL;
				}
			}
			freeLL(ll_phys_node);
			freeLL(virtNode);
			return NULL;
		}
		//increase the virt_addr by 1 inner bit value
		virt_addr = (void*) ((unsigned long)virt_addr + virt_addr_inner_incremental);
		node = node->next;
	}
	//mark the selected physical page as used
	node = ll_phys_node;
	while(node != NULL){
		i = node->index;
		if (i >= 0 && i < phys_page_count) {
			phys_bit_map[i] = 1;
		}
		node = node->next;
	}
	free(ll_phys_node);
	freeLL(virtNode);

	printf("allocated\n");

	return ret_virt_addr;
	/* HINT: If the page directory is not initialized, then initialize the
	 page directory. Next, using get_next_avail(), check if there are free pages. If
	 free pages are available, set the bitmaps and map a new page. Note, you will
	 have to mark which physical pages are used. */
	//printf("allocated\n");
	//return virt_addr;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
 */
void myfree(void *va, int size) {

	//assumed va is the base address, and size equal to the amount of thing
	//that was allocated in mymalloc

	//Free the page table entries starting from this virtual address (va)
	//Also mark the pages free in the bitmap
	//Only free if the memory from "va" to va+size is valid

	//error check
	//if(page_dir == NULL || mem == NULL || va == NULL || size <= 0 || size >= MAX_MEMSIZE ) return;
	pte_t physAddr = 0;
	pte_t_node * pgtablenode = NULL;
	pte_t * pgtable = NULL;
	int3_node * node = Get3DecimalOfBin(va);
	unsigned int outer, inner, offset, no_pgs, i, index;
	if(node == NULL) {
		return;
	}
	outer = node->outer - 1;
	inner = node->inner;
	offset = node->offset;
	no_pgs = ceil((float)size / PGSIZE);
	free(node);
	//bound check
	if(outer >= outer_page_count) return;
	else if(inner + no_pgs > inner_page_count - 1) return;
	else if(offset > PGSIZE - 1) return;

	//retrieve page node from outer
	pgtablenode = page_dir[outer];
	if(pgtablenode == NULL) return;
	if(pgtablenode->data == NULL) {
		page_dir[outer] = NULL;
		free(pgtablenode);
		return;
	}
	//check if va is equal to base va
	if(va != (void*)pgtablenode->base_va_addr){
		return;
	}
	//mark all selected virtual pages as not selected
	FreeSelectedVirt(outer, no_pgs);
	//mark all selected physical pages as not selected
	pgtable = pgtablenode->data;
	for(i = 0; i < no_pgs; i++){
		//get the differences since we need to zero it
		physAddr = pgtable[i] - (unsigned long)mem;
		//divide by pgsize to get the page we are at
		//based on the differences above
		index = physAddr / PGSIZE;
		//if index is out of bound just return
		if(index >= phys_page_count){
			return;
		}
		//set physical page to free
		phys_bit_map[index] = 0;
	}
	free(pgtable);
	free(pgtablenode);
	//disconnected the pagetable outer in page_ddir
	page_dir[outer] = NULL;
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
void FreeSelectedVirt(int index, int size){
	int i = index;
	for (i = index; i < virt_page_count && i < index + size; i++) {
		virt_bit_map[i] = 0;
	}
}
void FreeSelectedPhys(next_avail_node * node){
	int i = 0;
	while (node != NULL) {
		i = node->index;
		if (i >= 0 && i < phys_page_count) {
			phys_bit_map[i] = 0;
		}
		node = node->next;
	}
}

void freeLL (next_avail_node * head){
	if(head == NULL) return;
	next_avail_node * node = head;
	next_avail_node * prev = NULL;
	while(node != NULL){
		prev = node;
		node = node->next;
		free(prev);
	}
}
int3_node * Get3DecimalOfBin(void * address){
	//error checking
	if (address == NULL) {
		printf("an error occurred on 612\n");
		return NULL;
	}
	int3_node * node = (int3_node *)malloc(sizeof(int3_node));
	if(node == NULL) return NULL;

	unsigned long address_long = (unsigned long)address;

	unsigned long tempt = 0;
	unsigned int rightShift = inner_bit_count + offset_bit_count;
	unsigned int outer = (unsigned int) (address_long >> rightShift);
	tempt = (unsigned long) (address_long >> (rightShift - inner_bit_count));
	unsigned int inner = (unsigned int) ((unsigned long) (outer
			<< inner_bit_count) ^ tempt);
	unsigned int offset = (unsigned int) ((unsigned long) (tempt
			<< offset_bit_count) ^ address_long);

	node->outer = outer;
	node->inner = inner;
	node->offset = offset;
	/*//counter
	int i = 0;
	//counter to keep track of which bit we are on
	int counter = 0;
	//length of the bin
	size_t length;

	//get the length of the binary string
	length = strlen(bin);
	//retrieve the value for offset, inner, and outer
	for (i = length - 1; i >= 0; i--) {
		//reset counter for every time we pass a bit count
		if ((length - 1 - i) == offset_bit_count
				|| (length - 1 - i - offset_bit_count) == inner_bit_count
				|| (length - 1 - i - offset_bit_count - inner_bit_count)
						== outer_bit_count) {
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
	node->outer = outer;
	node->inner = inner;
	node->offset = offset;*/
	return node;
}
//return the address from a given binary string
unsigned long bintohex(char * bin) {
	if (bin == NULL)
		return;
	unsigned long val = 0;
	unsigned long counter = 0;
	size_t length = strlen(bin);
	int i = 0;
	for (i = 0; i < length; i++) {
		if (bin[length - i - 1] == '1') {
			val += (unsigned long) pow(2, counter);
		}
		counter++;
	}
	return val;
}

//convert the given index into a virtual address
//index is increased by one
void* ConvertIndexToVA(unsigned int index){
	//void* ret = (void*)malloc(sizeof(void*));
	//if(ret == NULL) return NULL;
	int length = 0;
	unsigned long final_addr = 0;
	final_addr = (unsigned long)(index<<(inner_bit_count+offset_bit_count));
	/*
	char * outerBinStr = (char*)malloc(sizeof(char)*outer_bit_count);
	if(outerBinStr == NULL) {
		//free(ret);
		return NULL;
	}
	char * finalStr = (char*)malloc(sizeof(char)*33);
	memset(finalStr, '0', sizeof(char)*32);
	finalStr[32] = '\0';

	inner = 0;
	outer = index + 1;

	itoa(outer, outerBinStr, 2);
	c = strlen(outerBinStr) - 1;
	for(i = 32 - offset_bit_count - inner_bit_count - 1; i >= 0 ; i--){
		if (c < 0) {
			break;
		}
		finalStr[i] = outerBinStr[c];
		c--;
	}
	//free(innerBinStr);
	free(outerBinStr);
	final_addr = bintohex(finalStr);
	free(finalStr);*/
	return (void*)final_addr;
}
