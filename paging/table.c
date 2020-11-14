/* frame.c - manage page tables */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

unsigned int gpt[4];

SYSCALL update_ipt(int frame_no, int pid, int vpno, int fr_type){
    STATWORD ps;
    disable(ps);
    frm_tab[frame_no].fr_status = FRM_MAPPED;
    frm_tab[frame_no].fr_pid = pid;
    frm_tab[frame_no].fr_vpno = vpno;
    frm_tab[frame_no].fr_refcnt = 1;
    frm_tab[frame_no].fr_type = fr_type;
    frm_tab[frame_no].fr_dirty = 0;
    frm_tab[frame_no].age = 0;
    restore(ps);
    return(OK);
}

SYSCALL init_pgdir(int pid){
    STATWORD ps;
    int frame_no, i;
    pd_t *pgdir;
    disable(ps);
    if(get_frm(&frame_no) == SYSERR){
        // kprintf("\nError - Unable to get free frame");
        restore(ps);
        return SYSERR;
    }
    update_ipt(frame_no, pid, -1, FR_DIR);
    pgdir = (pd_t *)((FRAME0 + frame_no)*NBPG);
    for(i = 0; i<1024; i++){
        (pgdir+i)->pd_acc = 0;
        (pgdir+i)->pd_avail = 0;
        (pgdir+i)->pd_fmb = 0;
        (pgdir+i)->pd_global = 0;
        (pgdir+i)->pd_mbz = 0;
        (pgdir+i)->pd_pcd = 0;
        (pgdir+i)->pd_pwt = 0;
        (pgdir+i)->pd_user = 0;
        (pgdir+i)->pd_write = 1;
        if(i < 4){
            (pgdir+i)->pd_pres = 1;
            (pgdir+i)->pd_base = gpt[i]; 
        }
    }
    proctab[pid].pdbr = pgdir;
    restore(ps);
    // test_pdt(0);
    return(OK);
}

SYSCALL init_pt(int pid, int *frame_no){
    STATWORD ps;
    int i;
    pt_t *pt_addr;
    disable(ps);
    if(get_frm(frame_no) == SYSERR){
        // kprintf("\nError - Unable to get free frame");
        restore(ps);
        return SYSERR;
    }
    update_ipt(*frame_no, pid, -1, FR_TBL);
    pt_addr = (pt_t*) ((FRAME0 + *frame_no)*NBPG);
    for(i = 0; i<1024; i++){
        (pt_addr + i)->pt_acc = 0;
        (pt_addr + i)->pt_avail = 0;
        (pt_addr + i)->pt_base = 0;
        (pt_addr + i)->pt_dirty = 0;
        (pt_addr + i)->pt_global = 0;
        (pt_addr + i)->pt_mbz = 0;
        (pt_addr + i)->pt_pcd = 0;
        (pt_addr + i)->pt_pres = 0;
        (pt_addr + i)->pt_pwt = 0;
        (pt_addr + i)->pt_user = 0;
        (pt_addr + i)->pt_write = 1;
    }
    restore(ps);
    return(OK);
}

SYSCALL invalidate_pte(int pid, int vpno){
    STATWORD ps;
    unsigned long address;
    disable(ps);
    address = vpno*NBPG;
    // kprintf("\nInvalidate Address - %u; pid- %d",address,pid);
    virt_addr_t *vaddr = (virt_addr_t*)&address;
    pd_t *pde = (pd_t*)(proctab[pid].pdbr + (vaddr->pd_offset*sizeof(pd_t)));
    pt_t *pte = (pt_t*)((pde->pd_base*NBPG) + (vaddr->pt_offset*sizeof(pt_t)));

    pte->pt_acc=0;
    pte->pt_pres = 0;
    pte->pt_base = 0;

    restore(ps);
    return OK;
}

int init_gpt(){
    int frame_no, i, j;
    pt_t *gpt_addr;
    for(i = 0; i<4; i++){
        if(get_frm(&frame_no) == SYSERR){
            return SYSERR;
        }
        update_ipt(frame_no, -1, -1, FR_TBL);
        gpt[i] = FRAME0 + frame_no;
        gpt_addr = (pt_t*)(gpt[i]*NBPG);
        for(j=0;j<1024;j++){
            (gpt_addr + j)->pt_acc = 0;
            (gpt_addr + j)->pt_avail = 0;
            (gpt_addr + j)->pt_base = ((i*1024)+j);
            (gpt_addr + j)->pt_dirty = 0;
            (gpt_addr + j)->pt_global = 0;
            (gpt_addr + j)->pt_mbz = 0;
            (gpt_addr + j)->pt_pcd = 0;
            (gpt_addr + j)->pt_pres = 1;
            (gpt_addr + j)->pt_pwt = 0;
            (gpt_addr + j)->pt_user = 0;
            (gpt_addr + j)->pt_write = 1;
        }
    }
    // test_gpt();
    return OK;
}