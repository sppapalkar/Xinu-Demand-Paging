/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>
extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	STATWORD ps;
	struct	mblock	*curr, *prev, *addr;
	unsigned top, base, limit;
	int store, pageth;
	disable(ps);
	if(bsm_lookup(currpid, block, &store, &pageth) == SYSERR){
		// kprintf("\nError - Mapping not found");
		restore(ps);
		return SYSERR;
	}
	addr = (unsigned)(BACKING_STORE_BASE + (unsigned)(store*BACKING_STORE_UNIT_SIZE) + (unsigned)(pageth*NBPG)); 
	base = (unsigned)(BACKING_STORE_BASE + (proctab[currpid].store*BACKING_STORE_UNIT_SIZE));
	limit = (unsigned)(base + (proctab[currpid].vhpnpages*NBPG));

	if(size <= 0 || addr < base || addr >= limit){
		// kprintf("\nError - Invalid free parameters");
		restore(ps);
		return(SYSERR);
	}

	prev = proctab[currpid].vmemlist;
	curr = proctab[currpid].vmemlist->mnext;
	size = (unsigned) roundmb(size);
	
	while(curr != NULL && curr<addr){
		prev = curr;
		curr = curr->mnext;
	}
	top = prev->mlen + (unsigned)prev;
	if((prev != proctab[currpid].vmemlist && top > (unsigned)addr) || (curr != NULL && ((unsigned)addr +size) > (unsigned) curr)){
		// kprintf("\nError - Invalid block");
		restore(ps);
		return(SYSERR);
	}
	if(prev != proctab[currpid].vmemlist && top==(unsigned)addr){
		prev->mlen += size;
	}
	else{
		addr->mlen = size;
		addr->mnext = curr;
		prev->mnext = addr;
		prev = addr;
	}
	if((unsigned) (prev->mlen + (unsigned)prev) == (unsigned)curr){
		prev->mlen += curr->mlen;
		prev->mnext = curr->mnext;
	}
	proctab[currpid].vmemlist->mlen += size;
	restore(ps);
	return(OK);
}
