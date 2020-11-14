/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
bs_map_t bsm_tab[NBS];
SYSCALL init_bsm()
{
    STATWORD ps;
    int i;
    disable(ps);
    for(i = 0; i<NBS; i++){
        // kprintf("Size: %d", sizeof(bs_map_t));
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_pid = 0;
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].bs_vpno = 0;

        bsm_tab[i].pmaphead = (bs_proc_map*)getmem(sizeof(bs_proc_map));
        bsm_tab[i].pmaphead->bs_pid = -1;
        bsm_tab[i].pmaphead->bs_vpno = -1;
        
        bsm_tab[i].pmaptail = (bs_proc_map*)getmem(sizeof(bs_proc_map));
        bsm_tab[i].pmaptail->bs_pid = -1;
        bsm_tab[i].pmaptail->bs_vpno = -1;

        bsm_tab[i].pmaphead->prev = NULL;
        bsm_tab[i].pmaphead->next = bsm_tab[i].pmaptail;
        bsm_tab[i].pmaptail->prev = bsm_tab[i].pmaphead;
        bsm_tab[i].pmaptail->next = NULL;
    }
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    int i;
    disable(ps);
    *avail = -1;
    for(i=0; i<NBS; i++){
        if(bsm_tab[i].bs_status == BSM_UNMAPPED){
            *avail = i;
            break;
        }
    }
    if(*avail == -1){
        restore(ps);
        return SYSERR;
    }
    restore(ps);
    return OK;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);
    if(bsm_tab[i].bs_status == BSM_SHARED && bsm_tab[i].pmaphead->next != bsm_tab[i].pmaptail){
        restore(ps);
        return SYSERR;
    }
    bsm_tab[i].bs_npages = 0;
    bsm_tab[i].bs_pid = 0;
    bsm_tab[i].bs_status = BSM_UNMAPPED;
    bsm_tab[i].bs_sem = 0;
    bsm_tab[i].bs_vpno = 0;
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth){
    STATWORD ps;
    int i;
    long vpno;
    disable(ps);
    *store = -1;
    vpno = vaddr/NBPG;
    
    for(i=0; i<NBS; i++){
        if(bsm_tab[i].bs_status == BSM_MAPPED){
            if(bsm_tab[i].bs_pid == pid && bsm_tab[i].bs_vpno <= vpno && vpno < (bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages)){
                *store = i;
                *pageth = vpno - bsm_tab[i].bs_vpno;
            }
        }
        else if(bsm_tab[i].bs_status == BSM_SHARED){
            bs_proc_map *current = bsm_tab[i].pmaphead->next;
            while(current != bsm_tab[i].pmaptail){
                if(current->bs_pid == pid && current->bs_vpno <= vpno && vpno < (current->bs_vpno + current->npages)){
                    *store = i;
                    *pageth = vpno - current->bs_vpno; 
                    break;
                }
                current = current->next;
            }
        }
        if(*store != -1)
            break;       
    }
    if(*store == -1){
        restore(ps);
        return SYSERR;
    }
    restore(ps);
    return OK;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, bsd_t source, int npages, int map_type){
    STATWORD ps;
    disable(ps);
    if(bsm_tab[source].bs_status == BSM_MAPPED || npages <= 0 || npages > 256){
        // kprintf("\nBSM map not created");
        restore(ps);
        return SYSERR;
    }
    if(bsm_tab[source].bs_status == BSM_UNMAPPED){
        bsm_tab[source].bs_status = map_type;
        bsm_tab[source].bs_pid = pid;
        bsm_tab[source].bs_vpno = vpno;
        bsm_tab[source].bs_npages = npages;
    }
    if(bsm_tab[source].bs_status == BSM_SHARED){
        bs_proc_map *temp;
        bs_proc_map *newentry = (bs_proc_map*) getmem(sizeof(bs_proc_map));

        newentry->bs_pid = pid;
        newentry->bs_vpno = vpno;
        newentry->npages = npages;
        newentry->prev = bsm_tab[source].pmaptail->prev;
        newentry->next = bsm_tab[source].pmaptail;

        temp = bsm_tab[source].pmaptail->prev;
        temp->next = newentry;
        bsm_tab[source].pmaptail->prev = newentry;
    }  
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno){
    STATWORD ps;
    int i; int store, pageth;
    bs_proc_map *current, *temp;
    disable(ps);
    if(bsm_lookup(pid, vpno*4096, &store, &pageth) == SYSERR || bsm_tab[store].bs_status != BSM_SHARED){
        restore(ps);
        return SYSERR;
    }
    
    current = bsm_tab[store].pmaphead->next;
    while(current != bsm_tab[store].pmaptail){
        if(current->bs_pid == pid && current->bs_vpno <= vpno && vpno < (current->bs_vpno + current->npages)){ 
            break;
        }
        current = current->next;
    }
    temp = current->prev;
    temp->next = current->next;
    temp = current->next;
    temp->prev = current->prev;
    freemem(current, sizeof(bs_proc_map));

    if(bsm_tab[store].pmaphead->next == bsm_tab[store].pmaptail){
        free_bsm(store);
    }
    restore(ps);
    return OK;
}

void display(){
    int i;
    bs_proc_map *current;
    for(i = 0; i<NBS; i++){
        kprintf("\nBSM ID: %d", i);
        kprintf("\nStatus: %d, PID: %d, VPNO: %d, NPAGES: %d", bsm_tab[i].bs_status, bsm_tab[i].bs_pid, bsm_tab[i].bs_vpno, bsm_tab[i].bs_npages);
        kprintf("\nProc Map: Head -> ");
        current = bsm_tab[i].pmaphead->next;
        while(current != bsm_tab[i].pmaptail){
            kprintf("<PID: %d, VPNO: %d, NPages: %d> -> ",current->bs_pid, current->bs_vpno, current->npages);
            current = current->next;
        }
        kprintf("Tail \n");
    }
}
