/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include<paging.h>
/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */

void release_proc_frames(int pid){
	int i = 0;
	for(i = 0; i<NFRAMES; i++){
		if(frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == pid){
			free_frm(i);
		}
	}
}

void remove_bsm_mappings(int pid){
	int i;
	bs_proc_map *current, *temp;
	for(i=0; i<NBS; i++){
		if(bsm_tab[i].bs_status == BSM_SHARED){
			current = (bs_proc_map *) bsm_tab[i].pmaphead->next;
			while(current != bsm_tab[i].pmaptail){
                if(current->bs_pid == pid){
                    temp = current->prev;
    				temp->next = current->next;
    				temp = current->next;
    				temp->prev = current->prev; 
    				// freemem(current, sizeof(bs_proc_map));
                }
                current = current->next;
            }
		}
	}

}

SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	// Write all frames back to BS
	// kprintf("\nKILL DONE");
	if(pid!=49 && pid!=0){
		write_dirty(pid);
		remove_bsm_mappings(pid);
		if(proctab[pid].store >= 0 && proctab[pid].store < 8){
			release_bs(proctab[pid].store);
		}
		release_proc_frames(pid);
	}
	
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
