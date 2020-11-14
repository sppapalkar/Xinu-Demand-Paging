#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {
  /* requests a new mapping of npages with ID map_id */
    STATWORD ps;
    disable(ps);
    if(bs_id < 0 || bs_id > 7){
      // kprintf("\nError - Invalid BS ID");
      restore(ps);
      return SYSERR;
    }
    if(npages <= 0 || npages > 256){
      // kprintf("\nError - Invalid NPages");
      restore(ps);
      return SYSERR;
    }
    if(bsm_tab[bs_id].bs_status == BSM_MAPPED){
      // kprintf("\nError - BS is already in use for private heap");
      restore(ps);
      return SYSERR;
    }
    if(bsm_tab[bs_id].bs_status == BSM_SHARED){
      npages = bsm_tab[bs_id].bs_npages;
    }
    restore(ps);
    return npages;
}


