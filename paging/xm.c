/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  if(source < 0 || source > 7){
    // kprintf("\nError - Invalid BS ID");
    restore(ps);
    return SYSERR;
  }
  if(npages <= 0 || npages > 256){
    // kprintf("\nError - Invalid NPages");
    restore(ps);
    return SYSERR;
  }
  if(bsm_tab[source].bs_status == BSM_MAPPED){
    // kprintf("\nError - The BS contains a private heap");
    restore(ps);
    return SYSERR;
  }
  if(bsm_tab[source].bs_status == BSM_SHARED && bsm_tab[source].bs_npages<npages){
    // kprintf("\nRequested Map - Store: %d, Npages: %d",source, npages);
    // kprintf("\nError - The size of existing BS is %d pages", bsm_tab[source].bs_npages);
    restore(ps);
    return SYSERR;
  }
  if(bsm_map(currpid, virtpage, source, npages, BSM_SHARED) == SYSERR){
    // kprintf("\nError - Could not create a mapping");
    restore(ps);
    return SYSERR;
  }
  // kprintf("\nMapped: \n");
  // display();
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);
  // display();
  // kprintf("PID: %d",currpid);
  if(grpolicy() != AGING)
    write_dirty(currpid);
  // kprintf("\n\nunmpp called");
  if(bsm_unmap(currpid, virtpage) == SYSERR){
    // kprintf("\nError - Could not remove the mapping");
    restore(ps);
    return SYSERR;
  }

  // display();
  // kprintf("\nUnmapped: \n");
  restore(ps);
  return OK;
}
