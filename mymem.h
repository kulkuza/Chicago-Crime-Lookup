/*mymem.h*/

//
// wrapper functions for malloc and free
//
// Michael Mei
// Linux Mint 18(Sarah) with gcc
// U. of Illinois, Chicago
// CS251, Fall 2016
// HW #9
//

void *mymalloc(unsigned int size);
void myfree(void *ptr);
void mymem_stats();