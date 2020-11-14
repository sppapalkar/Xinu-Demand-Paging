/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	int pid, store;
	struct mblock *block;
	disable(ps);
	if(hsize > 256){
		// kprintf("\nHeap size cannot exceed 256 pages");
		restore(ps);
		return SYSERR;
	}
	pid = create(procaddr, ssize, priority, name, nargs, args);
	if(pid == -1){
		// kprintf("\nError - Unable to create process");
		restore(ps);
		return SYSERR;
	}
	if(get_bsm(&store) == -1){
		// kprintf("\nError - No BS available");
		restore(ps);
		return SYSERR;
	}
	if(bsm_map(pid, 4096, store, hsize, BSM_MAPPED) == -1){
		// kprintf("\nError - Mapping Unsuccessful");
		restore(ps);
		return SYSERR;
	}
	// display();
	proctab[pid].store = store;
	proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;

	block = (struct mblock*)(BACKING_STORE_BASE + store*BACKING_STORE_UNIT_SIZE);
	block->mlen = truncew(hsize*NBPG);
	block->mnext = NULL;
	
	proctab[pid].vmemlist = (struct mblock*) getmem(sizeof(struct mblock));
	proctab[pid].vmemlist->mlen = block->mlen;
	proctab[pid].vmemlist->mnext = block;

	// kprintf("\nMemlist: %u; Node 1 - <Addr: %u, Len: %d>", (unsigned)(proctab[pid].vmemlist), (unsigned)proctab[pid].vmemlist->mnext, block->mlen);
	restore(ps);
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
