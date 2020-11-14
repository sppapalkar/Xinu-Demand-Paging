/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>


extern int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */
  STATWORD ps;
  disable(ps);
  if(policy != SC && policy != AGING){
    restore(ps);
    return SYSERR;
  }
  page_replace_policy = policy;
  // if(policy == AGING)
  //   ageq = NULL;
  debug_mode = 1;
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}
