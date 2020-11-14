#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {
  STATWORD ps;
  disable(ps);
  if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
    restore(ps);
    return OK;
  }
  if(bsm_tab[bs_id].bs_status == BSM_SHARED && bsm_tab[bs_id].pmaphead->next != bsm_tab[bs_id].pmaptail){
    // kprintf("\nError - Cannot release a shared BS mapped to other processes");
    restore(ps);
    return SYSERR;
  }
  if(bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_pid != currpid){
    // kprintf("\nError - BS belongs to other process");
    restore(ps);
    return SYSERR;
  }
  free_bsm(bs_id);
  restore(ps);
  return OK;
}

