/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */

void display_frmtab(){
	int i;
	for(i = 0;i<20; i++){
		kprintf("\nI: %d; PID %d; VPNO: %d; Type: %d", FRAME0+i, frm_tab[i].fr_pid, frm_tab[i].fr_vpno, frm_tab[i].fr_type);
	}
}

SYSCALL pfint()
{
  STATWORD ps;
  unsigned long address;
  int store, pageth;
  disable(ps);
  // Read faulted address
  address = read_cr2();
  virt_addr_t *vaddr = (virt_addr_t*)&address;
  // kprintf("\nPF Address - %u",address);
  // Check if address is valid
  if(bsm_lookup(currpid, address, &store, &pageth) == SYSERR){
    // kprintf("\nCurrpid - %d",currpid);
    // kprintf("\nError - Invalid Address - %u",address);
    kill(currpid);
    restore(ps);
    return SYSERR;
  }
  // Check if table exists
  pd_t *pde = proctab[currpid].pdbr + (vaddr->pd_offset*sizeof(pd_t));
  
  // If page table not present, initialize it
  if(pde->pd_pres == 0){
    int frame_no;
    if(init_pt(currpid, &frame_no) == SYSERR){
      // kprintf("Error - Page table initialization failed");
      restore(ps);
      return SYSERR;
    }
    pde->pd_pres = 1;
    pde->pd_base = FRAME0 + frame_no;
  }
  
  // Get the pte entry
  pt_t *pte = (pde->pd_base*NBPG) + (vaddr->pt_offset*sizeof(pt_t));

  // If page not present
  if(pte->pt_pres == 0){
    int frame_no;
    // Get new frame
    if(get_frm(&frame_no) == SYSERR){
      // kprintf("Error - Could not fetch the page frame");
      restore(ps);
      return SYSERR;
    }
    // Update Inverted Page Table
    update_ipt(frame_no, currpid, address/NBPG, FR_PAGE);
    // Read from BS
    read_bs((char*)((FRAME0 + frame_no)*NBPG), store, pageth);
    pte->pt_pres = 1;
    pte->pt_base = FRAME0 + frame_no;
  }
  else{
    frm_tab[pte->pt_base - FRAME0].fr_refcnt += 1;
  }
  // Restore ps
  restore(ps);
  return OK;
}


