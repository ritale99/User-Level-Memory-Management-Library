#include "my_vm.h"

pthread_mutex_t initialize_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t page_dir_lock;
pthread_mutex_t phys_bit_map_lock;
pthread_mutex_t virt_bit_map_lock;
pthread_mutex_t mem_lock;
pthread_mutex_t tlb_lock;

int init;

//initialize a direct mapped TLB when initializing a page table

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int put_in_TLB(void *va, void *pa) {

	/*Part 2 HINT: Add a virtual to physical page translation to the TLB */
	//After adding new page into translation table entry, also add a translation to the TLB using this
	pthread_mutex_lock(&tlb_lock);
	int indexMaxTime = 0;

	//set the max time to the first time we see
	time_t maxTime = tlb_store.entries[0].time;

	//look for empty spot in the tlb
	int i;
	for(i =0; i< TLB_SIZE; i++){

		//if the spot is not empty
		if((tlb_store.entries[i].va != NULL)){
			//check the max time again
			double time_dif = difftime(maxTime,tlb_store.entries[i].time);

			if(time_dif > 0){
				indexMaxTime = i;
				maxTime = tlb_store.entries[i].time;
			}
			//update indexMaxTime and maxTime
		}

		//check if the spot is empty
		if((tlb_store.entries[i]).va ==  NULL){

			time_t seconds = time(NULL);

			(tlb_store.entries[i]).va = va;
			(tlb_store.entries[i]).pa = pa;
			(tlb_store.entries[i]).time = seconds;

			pthread_mutex_unlock(&tlb_lock);
			//found a spot to insert the translation
			return 1;

		}

	}

	//if reached here, need to evict the min

	tlb_store.entries[indexMaxTime].va =va;
	tlb_store.entries[indexMaxTime].pa = pa;
	tlb_store.entries[indexMaxTime].time = time(NULL);
	pthread_mutex_unlock(&tlb_lock);
	return -1;
}

/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {
	//use the prescence of a translation in the TLB before performing translation in translate()
	/* Part 2: TLB lookup code here */
	pthread_mutex_lock(&tlb_lock);
	//loop through the tlb, and look for the va
	int i = 0;
	for(i=0; i<TLB_SIZE; i++){
		if(tlb_store.entries[i].va == va){
			//found the translation, return the physical address
			//check return type
			pte_t * ret = (pte_t *)(tlb_store.entries[i].pa);
			pthread_mutex_unlock(&tlb_lock);
			return ret;
		}
	}
	pthread_mutex_unlock(&tlb_lock);
	return NULL;
}

//init the tlb values to NULL
void init_tlb(){
	int i;
	for(i = 0; i< TLB_SIZE; i++){
		tlb_store.entries[i].va = NULL;
		tlb_store.entries[i].pa = NULL;
		tlb_store.entries[i].time = NULL;
	}

	misses = 0;
	accesses = 0;
}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate() {
	double miss_rate = 0;

	miss_rate = misses/accesses;

	/*Part 2 Code here to calculate and print the TLB miss rate*/

	printf("number of tlb misses: %lf, number of tlb accesses: %lf", misses, accesses);


	fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

/*
 Function responsible for allocating and setting your physical memory
 */
void SetPhysicalMem() {
	//lock the init
	pthread_mutex_lock(&initialize_lock);
	//if we somehow already initialized the library, return
	if (init) {
		pthread_mutex_unlock(&initialize_lock);
		return;
	}
	//initialize any additional mutex

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
		pthread_mutex_unlock(&initialize_lock);
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
	if (mem == NULL){
		pthread_mutex_unlock(&initialize_lock);
		return;
	}
	//allocate page directory
	page_dir = (pde_t*) malloc(sizeof(pde_t) * phys_page_count);
	if (page_dir == NULL) {
		free(mem);
		pthread_mutex_unlock(&initialize_lock);
		exit(0);
	}
	//create physical bit map
	phys_bit_map = (char*) malloc(sizeof(char) * phys_page_count);
	if (phys_bit_map == NULL) {
		free(mem);
		free(page_dir);
		pthread_mutex_unlock(&initialize_lock);
		exit(0);
	}
	//create virtual bit map
	virt_bit_map = (char*) malloc(sizeof(char) * virt_page_count);
	if (virt_bit_map == NULL) {
		free(phys_bit_map);
		free(page_dir);
		free(mem);
		pthread_mutex_unlock(&initialize_lock);
		exit(0);
	}
	//set all phys and bit to 0
	memset(phys_bit_map, 0, sizeof(char) * phys_page_count);
	memset(virt_bit_map, 0, sizeof(char) * virt_page_count);
	//initialize tlb
	init_tlb();
	init = 1;

	if ( pthread_mutex_init(&page_dir_lock, NULL) != 0 ) exit(0);
	if ( pthread_mutex_init(&phys_bit_map_lock, NULL) != 0 ) exit(0);
	if ( pthread_mutex_init(&virt_bit_map_lock, NULL) != 0 ) exit(0);
	if ( pthread_mutex_init(&mem_lock, NULL) != 0 ) exit(0);
	if ( pthread_mutex_init(&tlb_lock, NULL) != 0 ) exit (0);

	//unlock initialize
	pthread_mutex_unlock(&initialize_lock);
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
	pte_t * ret = NULL;

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

	//lock the page directory
	pthread_mutex_lock(&page_dir_lock);

	//we need to subtract the address? or the inner and offset
	//by the base (the virtual address we received after calling mymalloc)

	//retrieve the page table at the given outer
	//this page table is just a node
	selPgTable_Node = pgdir[outer];
	if(selPgTable_Node == NULL || selPgTable_Node->data== NULL) {
		printf("there was an error attempting to access selPgTable at %d\n", outer);
		pthread_mutex_unlock(&page_dir_lock);
		return NULL;
	}
	//get the base page va
	baseVA = (void*) selPgTable_Node->base_va_addr;
	if (baseVA == NULL) {
		printf("base VA is an error\n");
		pthread_mutex_unlock(&page_dir_lock);
		return NULL;
	}
	//get the 3 decimal value of the bin
	base_node = Get3DecimalOfBin(baseVA);
	if (base_node == NULL) {
		printf("error at 231\n");
		pthread_mutex_unlock(&page_dir_lock);
		return NULL;
	}
	//subtract the given va with the base VA received
	//so we can get the relative differences
	inner = inner - base_node->inner;
	offset = offset - base_node->offset;
	//we are freeing the 3 decimal of bin not the actual entry
	free(base_node);
	//return as pointer to pte_t + offset
	ret = (pte_t *)((pgdir[outer]->data[inner]) + offset);
	//unlock page directory
	pthread_mutex_unlock(&page_dir_lock);
	return ret;
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
	//attempt to lock the page directory
	pthread_mutex_lock(&page_dir_lock);
	selpgtable_node = pgdir[outer];
	//check if page table exist in page directory using outer
	//if no page table exist at outer, then create a new one
	if(selpgtable_node == NULL){
		pte_t_node * pte_node = (pte_t_node *) malloc(sizeof(pte_t_node));
		pte_t * pgtable = NULL;
		unsigned int pgtable_size = pow(2, inner_bit_count);
		if(pte_node == NULL) {
			pthread_mutex_unlock(&page_dir_lock);
			return -1;
		}
		//i am assuming we allocate page table as big as inner_bit_count
		pgtable = (pte_t*) malloc(sizeof(pte_t) * pgtable_size);
		if(pgtable == NULL){
			free(pte_node);
			pthread_mutex_unlock(&page_dir_lock);
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
		//unlock the page directory
		pthread_mutex_unlock(&page_dir_lock);
		return 0;
	}
	selpgtable = selpgtable_node->data;
	//get the 3 decimal value of base va at this pagetable
	node = Get3DecimalOfBin((void*)selpgtable_node->base_va_addr);
	if(node == NULL) {
		pthread_mutex_unlock(&page_dir_lock);
		return -1;
	}
	baseOuter = node->outer; baseInner = node->inner; baseOffset = node->offset;
	free(node);
	inner -= baseInner;
	//check if if page at inner exist
	void* addr = (void*)(selpgtable[inner]);
	//if there is no such mapping, make one
	if((void*)(selpgtable[inner]) == NULL){
		selpgtable[inner] = (unsigned long)pa;
		pthread_mutex_unlock(&page_dir_lock);
		return 0;
	}
	//return since there is nothing to do
	pthread_mutex_unlock(&page_dir_lock);
	return 0;
}
/*Function that gets the next available physical page
 */
next_avail_node* get_next_avail_phys(int num_pages) {

	next_avail_node* node = (next_avail_node*)malloc(sizeof(next_avail_node));
	if(node == NULL) return NULL;
	//lock memoery and physical bit map
	pthread_mutex_lock(&phys_bit_map_lock);
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
				//lock memory if we have found what we are looking for
				pthread_mutex_lock(&mem_lock);
				//return page address of first page
				node->addr = &mem[first_page * PGSIZE];
				node->index = first_page;
				node->next = NULL;
				for(i = 0 ; i < (curr_page - first_page); i++)
					phys_bit_map[first_page + i] = 1;

				//unlock memory and physical bit map mutex
				pthread_mutex_unlock(&phys_bit_map_lock);
				pthread_mutex_unlock(&mem_lock);
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
				//lock memory if we found what we are looking for
				pthread_mutex_unlock(&mem_lock);
				node->addr = &mem[curr_page * PGSIZE];
				node->index = curr_page;
				node->next = NULL;
				phys_bit_map[first_page] = 1;

				//unlock memory and physical bit map mutex
				pthread_mutex_unlock(&phys_bit_map_lock);
				pthread_mutex_unlock(&mem_lock);
				return node;
			}
		}
	}
	//could not find free pages
	pthread_mutex_unlock(&phys_bit_map_lock);
	return NULL;
}

/*Function that gets the next available virtual page
 */
next_avail_node * get_next_avail_virt(int num_pages) {

	//Use virtual address bitmap to find the next free continuous pages
	//matching num_pages
	next_avail_node* node = (next_avail_node*) malloc(sizeof(next_avail_node));
	if (node == NULL) return NULL;
	pthread_mutex_lock(&virt_bit_map_lock);
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
					pthread_mutex_unlock(&virt_bit_map_lock);
					return NULL;
				}
				node->index = first_page;
				node->next = NULL;
				for( i = 0 ; i < (curr_page - first_page); i++)
					virt_bit_map[first_page+i] = 1;
				//return page address of first page
				pthread_mutex_unlock(&virt_bit_map_lock);
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
					pthread_mutex_unlock(&virt_bit_map_lock);
					return NULL;
				}
				node->index = first_page;
				node->next = NULL;
				virt_bit_map[first_page] = 1;
				pthread_mutex_unlock(&virt_bit_map_lock);
				return node;
			}
		}
	}
	//could not find free pages
	pthread_mutex_unlock(&virt_bit_map_lock);
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
				//release all virtual page we've selected
				FreeSelectedVirt(virtNode->index, no_pgs);
				//release all physical pages we've selected
				FreeSelectedPhys(ll_phys_node);
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
		FreeSelectedVirt(virtNode->index, no_pgs);
		FreeSelectedPhys(ll_phys_node);
		freeLL(ll_phys_node);
		freeLL(virtNode);
		return NULL;
	}
	unsigned int outer = virt3DecimalNode->outer - 1;
	pte_t_node * pteNode = NULL;
	while(node != NULL){
		//if we could not map the virtual address to the
		//physical address then remove everything and return
		if (PageMap(page_dir, virt_addr, node->addr) < 0) {
			if (virt_addr != virtNode->addr) {
				//free the page table in pg dir
				FreeSelectedVirt(virtNode->index, no_pgs);
				FreeSelectedPhys(ll_phys_node);
				//lock the page directory
				pthread_mutex_lock(&page_dir_lock);
				if(page_dir[outer] != NULL){
					free(page_dir[outer]->data);
					pteNode = page_dir[outer];
					free(pteNode);
					page_dir[outer] = NULL;
				}
				pthread_mutex_unlock(&page_dir_lock);
			}
			//also add the page into the TLB
			put_in_TLB(virt_addr, node->addr);
			//free link list
			freeLL(ll_phys_node);
			freeLL(virtNode);
			return NULL;
		}
		//increase the virt_addr by 1 inner bit value
		virt_addr = (void*) ((unsigned long)virt_addr + virt_addr_inner_incremental);
		node = node->next;
	}
	//mark the selected physical page as used
	/*node = ll_phys_node;
	while(node != NULL){
		i = node->index;
		if (i >= 0 && i < phys_page_count) {
			phys_bit_map[i] = 1;
		}
		node = node->next;
	}*/
	free(ll_phys_node);
	freeLL(virtNode);

	return ret_virt_addr;
	/* HINT: If the page directory is not initialized, then initialize the
	 page directory. Next, using get_next_avail(), check if there are free pages. If
	 free pages are available, set the bitmaps and map a new page. Note, you will
	 have to mark which physical pages are used. */
}

/* Responsible for releasing one or more memory pages using virtual address (va)
 */
void myfree(void *va, int size) {
	//Free the page table entries starting from this virtual address (va)
	//Also mark the pages free in the bitmap
	//Only free if the memory from "va" to va+size is valid

	//error check
	if(page_dir == NULL || mem == NULL || va == NULL || size <= 0 || size >= MAX_MEMSIZE ) return;
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

	//lock page directory
	pthread_mutex_lock(&page_dir_lock);
	//retrieve page node from outer
	pgtablenode = page_dir[outer];
	if(pgtablenode == NULL) return;
	if(pgtablenode->data == NULL) {
		page_dir[outer] = NULL;
		free(pgtablenode);
		pthread_mutex_unlock(&page_dir_lock);
		return;
	}
	//check if va is equal to base va
	if(va != (void*)pgtablenode->base_va_addr){
		pthread_mutex_unlock(&page_dir_lock);
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
			pthread_mutex_unlock(&page_dir_lock);
			return;
		}
		//set physical page to free
		phys_bit_map[index] = 0;
	}
	free(pgtable);
	free(pgtablenode);
	//disconnected the pagetable outer in page_ddir
	page_dir[outer] = NULL;
	pthread_mutex_unlock(&page_dir_lock);
}

/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 */
void PutVal(void *va, void *val, int size) {

	/* HINT: Using the virtual address and Translate(), find the physical page. Copy
	 the contents of "val" to a physical page. NOTE: The "size" value can be larger
	 than one page. Therefore, you may have to find multiple pages using Translate()
	 function.*/
	if(va == NULL || page_dir == NULL || size <= 0) return;
	//get the physical address
	void * pa = NULL; void * o_va = va;
	pte_t_node * va_base_node = NULL;
	int3_node * va_3_int_node = NULL;
	unsigned int no_pgs = ceil((float)size / PGSIZE);
	unsigned long delta_size = 0;
	unsigned int ith_pgs;
	unsigned int o_size = size;
	unsigned int outer = 0, inner = 0, offset = 0;

	//lock the memory
	pthread_mutex_lock(&mem_lock);

	//loop through each pages we need to put value in
	for(ith_pgs = 0; ith_pgs < no_pgs; ith_pgs++ ){
		//get the physical pages

		//check the TLB first
		pa = (void*)check_TLB((void*) va);
		accesses++;

		//if we could not find result in TLB
		if(pa == NULL){
			pa = (void*) Translate(page_dir, (void*)(va));

			if(pa != NULL){
				put_in_TLB(va,pa);
			}

			misses++;
		}
		//return if we could not
		if(pa == NULL) {
			pthread_mutex_unlock(&mem_lock);
			return;
		}
		if(ith_pgs == 0){
			unsigned int _offset = 0;
			unsigned long diff = 0;
			//get the 3 decimal values of va
			va_3_int_node = Get3DecimalOfBin(va);
			if(va_3_int_node == NULL) {
				pthread_mutex_unlock(&mem_lock);
				return;
			}
			_offset = va_3_int_node->offset;
			free(va_3_int_node);
			//check if offset is out of bound
			if(_offset >= PGSIZE) {
				pthread_mutex_unlock(&mem_lock);
				return;
			}
			//calculate the delta size based on offset
			if(size+_offset < PGSIZE)
				delta_size = size;
			else
				delta_size = PGSIZE - _offset;
			memcpy(pa, val, (size_t) delta_size);
		}else if(ith_pgs > 0 && ith_pgs < no_pgs){
			delta_size = PGSIZE;
			memcpy(pa, (val + (o_size - size)), (size_t) delta_size);
		}else{
			delta_size = size;
			memcpy(pa, (val + (o_size - size)), (size_t) delta_size);
		}
		//increment our va by delta size
		va = (void*) ((unsigned long) va + delta_size);
		//decrease the size we have to deeal with
		size -= delta_size;
	}
	va = o_va;
	pthread_mutex_unlock(&mem_lock);
}

/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

	/* HINT: put the values pointed to by "va" inside the physical memory at given
	 "val" address. Assume you can access "val" directly by derefencing them.
	 If you are implementing TLB,  always check first the presence of translation
	 in TLB before proceeding forward */

	//check tlb and get the physical page based on va
	//since va+size can be multiple pages
	if(va == NULL || val == NULL || page_dir == NULL || size <= 0 || mem == NULL) return;
	//physical address
	void * pa = NULL;
	int3_node * va_3_int_node = NULL;
	unsigned int no_pgs = ceil((float) size / PGSIZE);
	unsigned long delta_size = 0;
	unsigned int ith_pgs;
	unsigned int o_size = size;
	unsigned int outer = 0, inner = 0, offset = 0;

	pthread_mutex_lock(&mem_lock);

	//loop through each pages we need to put value in
	for (ith_pgs = 0; ith_pgs < no_pgs; ith_pgs++) {
		//get the physical pages


		//check the TLB first
		pa = (void*)check_TLB((void*) va);
		accesses++;

		//if we did not find a result in our TLB
		if(pa == NULL){
			pa = (void*) Translate(page_dir, (void*) (va));

			if(pa != NULL){
				put_in_TLB(va,pa);
			}

			misses++;
		}

		//return if we could not
		if (pa == NULL){
			pthread_mutex_unlock(&mem_lock);
			return;
		}
		if (ith_pgs == 0) {
			unsigned int _offset = 0;
			unsigned long diff = 0;
			//get the 3 decimal values of va
			va_3_int_node = Get3DecimalOfBin(va);
			if (va_3_int_node == NULL) {
				return;
			}
			_offset = va_3_int_node->offset;
			free(va_3_int_node);
			//check if offset is out of bound
			if (_offset >= PGSIZE){
				pthread_mutex_unlock(&mem_lock);
				return;
			}
			//calculate the delta size based on offset
			if(size+_offset < PGSIZE)
				delta_size = size;
			else
				delta_size = PGSIZE - _offset;
			memcpy(val, pa, (size_t) delta_size);
		} else if (ith_pgs > 0 && ith_pgs < no_pgs) {
			delta_size = PGSIZE;
			memcpy((val + (o_size - size)), pa, (size_t) delta_size);
		} else {
			delta_size = size;
			memcpy((val + (o_size - size)), pa, (size_t) delta_size);
		}
		//increment our va by delta size
		va = (void*) ((unsigned long) va + delta_size);
		//decrease the size we have to deal with
		size -= delta_size;
	}
	pthread_mutex_unlock(&mem_lock);
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

	//matrices are square, column = row, and size is column
	if(mat1 == NULL || mat2 == NULL || answer == NULL || page_dir == NULL || size <= 0) return;
	unsigned long addr_mat1 = 0, addr_mat2 = 0, addr_answer = 0;
	int mat1_i_value = 0, mat2_i_value = 0;
	int i, j, d, sub_answer = 0;
	int sizeof_int = sizeof(int);
	//loop through row of mat1
	for(i = 0; i < size; i++){
		//loop through each col of mat2
		for(d = 0; d < size; d++){
			sub_answer = 0;
			addr_mat1 = 0;
			addr_mat2 = 0;
			//loop through each row of mat2
			for(j = 0; j< size; j++){
				//get address of mat1 + offset
				addr_mat1 = (unsigned long) mat1 + (i * size * sizeof_int + j * sizeof_int);
				//get address of mat2 + offset
				addr_mat2 = (unsigned long) mat2 + (j * size * sizeof_int + d * sizeof_int);
				//get value at addr_mat1 and store at mat1_i_value
				GetVal((void*) addr_mat1, &mat1_i_value, sizeof_int);
				//get value at addr_mat2 and store at mat2_i_value
				GetVal((void*) addr_mat2, &mat2_i_value, sizeof_int);
				//add the product of the two to the sub_answer
				sub_answer += mat1_i_value * mat2_i_value;
			}
			//get address of answer + offset
			addr_answer = (unsigned long) answer + (i * size * sizeof_int + d * sizeof_int);
			//put the sub_answer value into the addr_answer
			PutVal((void*) addr_answer, &sub_answer, sizeof_int);
		}
	}
}

//helper function
void FreeSelectedVirt(int index, int size){
	pthread_mutex_lock(&virt_bit_map_lock);
	int i = index;
	for (i = index; i < virt_page_count && i < index + size; i++) {
		virt_bit_map[i] = 0;
	}
	pthread_mutex_unlock(&virt_bit_map_lock);
}
void FreeSelectedPhys(next_avail_node * node){
	pthread_mutex_lock(&phys_bit_map_lock);
	int i = 0;
	while (node != NULL) {
		i = node->index;
		if (i >= 0 && i < phys_page_count) {
			phys_bit_map[i] = 0;
		}
		node = node->next;
	}
	pthread_mutex_unlock(&phys_bit_map_lock);
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

