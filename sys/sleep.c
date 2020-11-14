/* sleep.c - sleep */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>
#include <paging.h>
/*------------------------------------------------------------------------
 * sleep  --  delay the calling process n seconds
 *------------------------------------------------------------------------
 */

void clear_acc(){
	int i = 0;
	unsigned long address;
  	// kprintf("\nPID: %d",currpid);
	for(i = 0; i<1024; i++){
		// kprintf("\nI: %d; Type: %d, PID: %d; VPNO: %d",1024+i, frm_tab[i].fr_type, frm_tab[i].fr_pid, frm_tab[i].fr_vpno);
		if(frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_type == FR_PAGE && frm_tab[i].fr_pid == currpid){
			address = frm_tab[i].fr_vpno*NBPG;
			virt_addr_t *vaddr = (virt_addr_t*)&address;
			pd_t *pde = proctab[currpid].pdbr + (vaddr->pd_offset*sizeof(pd_t));
  			pt_t *pte = (pde->pd_base*NBPG) + (vaddr->pt_offset*sizeof(pt_t));
			pte->pt_acc = 0;
			// kprintf("\nPDE: %u; PTE: %u", (unsigned)pde, (unsigned)pte);
		}
	}
}

SYSCALL	sleep(int n)
{
	STATWORD ps;    
	if (n<0 || clkruns==0)
		return(SYSERR);
	// kprintf("\nIn sleep");
	if(grpolicy() == AGING)
		clear_acc();
	
	if (n == 0) {
	        disable(ps);
		resched();
		restore(ps);
		return(OK);
	}
	while (n >= 1000) {
		sleep10(10000);
		n -= 1000;
	}
	if (n > 0)
		sleep10(10*n);
	return(OK);
}
