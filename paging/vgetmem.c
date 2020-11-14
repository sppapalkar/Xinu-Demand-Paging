/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;
	struct	mblock	*curr, *prev, *leftover;
	unsigned base, limit;
	disable(ps);
	base = (unsigned)(BACKING_STORE_BASE + (proctab[currpid].store*BACKING_STORE_UNIT_SIZE));
	limit = (unsigned)(base + (proctab[currpid].vhpnpages*NBPG));
	
	if(nbytes <= 0){
		// kprintf("\nError - Inavlid size");
		restore(ps);
		return( (WORD *)SYSERR);
	}

	nbytes = (unsigned int) roundmb(nbytes);
	if(nbytes > proctab[currpid].vmemlist->mlen){
		// kprintf("\nError - Insufficient Space");
		restore(ps);
		return( (WORD *)SYSERR);
	}

	prev = proctab[currpid].vmemlist;
	curr = proctab[currpid].vmemlist->mnext;
	while(curr != NULL){
		if(curr->mlen == nbytes){
			prev->mnext = curr->mnext;
			proctab[currpid].vmemlist->mlen -= nbytes;
			restore(ps);
			return (WORD*)((4096+(((unsigned)curr - base)/NBPG))*NBPG);
		}
		else if(curr->mlen > nbytes){
			leftover = (struct mblock*) ((unsigned)curr + nbytes);
			prev->mnext = leftover;
			leftover->mnext = curr->mnext;
			leftover->mlen = curr->mlen - nbytes;
			proctab[currpid].vmemlist->mlen -= nbytes;
			restore(ps);
			return (WORD*)((4096+(((unsigned)curr - base)/NBPG))*NBPG);
			// return ((WORD *)curr);
		}
		prev = curr;
		curr = curr->mnext;
	}
	// kprintf("\nError-No free space");
	restore(ps);
	return( SYSERR );
}


