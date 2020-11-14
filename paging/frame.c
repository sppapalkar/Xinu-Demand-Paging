/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
int currfrm = 0;
fr_map_t frm_tab[NFRAMES];
SYSCALL init_frm()
{
  STATWORD ps;
  int i;
  // Disable interrupts
  disable(ps);
  // Initialize frame table
  for(i = 0; i<NFRAMES; i++){
    frm_tab[i].fr_dirty = 0;
    frm_tab[i].fr_pid = 0;
    frm_tab[i].fr_refcnt = 0;
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_type = 0;
    frm_tab[i].fr_vpno = 0; 
  }
  // Restore interrupts
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD ps;
  int i;
  // Disable interrupt
  disable(ps);
  // Search for free frame
  for(i = 0; i<NFRAMES; i++){
    if(frm_tab[i].fr_status == FRM_UNMAPPED){
      // Freeframe found return
      frm_tab[i].fr_status = FRM_MAPPED;
      *avail = i;
      // kprintf("Free frame: %d",i);
      if(grpolicy() == AGING)
        insert_ageq(i);
      
      restore(ps);
      return OK;
    }
  }
  if(grpolicy() == SC){
    *avail = swap_sc();
  }
  else if(grpolicy()==AGING){
    *avail = swap_aging();
  }
  if(debug_mode == 1){
    kprintf("\n\tReplaced Frame: %d\n", *avail+FRAME0);
  }
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  STATWORD ps;
  // Disable interrupt
  disable(ps);
  frm_tab[i].fr_dirty = 0;
  frm_tab[i].fr_pid = 0;
  frm_tab[i].fr_refcnt = 0;
  frm_tab[i].fr_status = FRM_UNMAPPED;
  frm_tab[i].fr_type = 0;
  frm_tab[i].fr_vpno = 0;
  
  if(grpolicy() == AGING)
    remove_ageq(i);
  
  restore(ps);
  return OK;
}

void insert_ageq(int frame){
  age_node *current, *newnode;
  newnode = (age_node*) getmem(sizeof(age_node));
  newnode->frame_no = frame;
  newnode->next = NULL;
  if(ageq == NULL){
    ageq = newnode;
  }
  else{
    current = ageq;
    while(current->next != NULL){
      current = current->next;
    }
    current->next = newnode;
  }
}

void remove_ageq(int frame){
  age_node *current, *prev;
  current = ageq;
  // kprintf("\nRemove: %d", frame);
  if(ageq == NULL)
    return;
  if(ageq->frame_no == frame){
    ageq = ageq->next;
  }
  while(current!=NULL && current->frame_no != frame){
    prev = current;
    current = current->next;
  }
  prev->next = current->next;
  freemem(current, sizeof(age_node));
}

int check_ref(int pid, int vpno, int clear){
  unsigned long address;
  address = vpno*NBPG;
  virt_addr_t *vaddr = (virt_addr_t*)&address;
  pd_t *pde = proctab[pid].pdbr + (vaddr->pd_offset*sizeof(pd_t));
  pt_t *pte = (pde->pd_base*NBPG) + (vaddr->pt_offset*sizeof(pt_t));
  
  if(pte->pt_acc == 0){
    return 0;
  }
  if(clear == 1)
    pte->pt_acc = 0;
  return 1;
}

int swap_aging(){
  int i, store, pageth, minframe;
  int minage = 2147483647;
  int frame = -1;
  age_node *current;
  current = ageq;
  while(current != NULL){
    frame = current->frame_no;
    if(frm_tab[frame].fr_status == FRM_MAPPED && frm_tab[frame].fr_type == FR_PAGE){
      frm_tab[frame].age = frm_tab[frame].age/2;
      // kprintf("\nFrameNo: %d; Age: %d",1024+frame, frm_tab[frame].age);
      if(check_ref(frm_tab[frame].fr_pid, frm_tab[frame].fr_vpno, 0) == 1){
        frm_tab[frame].age += 128;
        if(frm_tab[frame].age > 255)
          frm_tab[frame].age = 255;
      }
      // kprintf("\nNo: %d; Age: %d",frame, frm_tab[frame].age);
      if(frm_tab[frame].age < minage){
        minage = frm_tab[frame].age;
        minframe = frame;
      }
    }
    // kprintf("\nFrameNo: %d; Age: %d",1024+frame, frm_tab[frame].age);
    current = current->next;
  }
  if(bsm_lookup(frm_tab[minframe].fr_pid, frm_tab[minframe].fr_vpno*NBPG, &store, &pageth) == OK){
    write_bs(((minframe+FRAME0)*NBPG), store, pageth);
  }
  invalidate_pte(frm_tab[minframe].fr_pid, frm_tab[minframe].fr_vpno);
  free_frm(minframe);
  insert_ageq(minframe);
  return minframe;
}

int swap_sc(){
  int store; 
  int pageth;
  int frame = -1;
  while(1){
    if(frm_tab[currfrm].fr_type == FR_PAGE && check_ref(frm_tab[currfrm].fr_pid, frm_tab[currfrm].fr_vpno, 1) == 0){
      if(bsm_lookup(frm_tab[currfrm].fr_pid, frm_tab[currfrm].fr_vpno*NBPG, &store, &pageth) == OK){
        write_bs(((currfrm+FRAME0)*NBPG), store, pageth);
      }
      else{
        // kprintf("\nBSM Lookup failed");
      }
        frame = currfrm;
        invalidate_pte(frm_tab[currfrm].fr_pid, frm_tab[currfrm].fr_vpno);
        update_ipt(currfrm, 0, 0, FRM_UNMAPPED);
        break;
    }
    currfrm = (currfrm+1)%NFRAMES;  
  }
  return frame;
}

