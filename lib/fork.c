// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here
        //check if the page fault is a write.
        if ((err & FEC_WR) == 0)
        panic("pgfault: faulting address [%08x] is not a write\n", addr);

        void *page_aligned_addr = (void *) ROUNDDOWN(addr, PGSIZE);
        uint32_t pgno = (uint32_t) page_aligned_addr / PGSIZE;
        if (!(uvpt[pgno] & PTE_COW))
        panic("pgfault: fault was not on a copy-on-write page\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.i
        if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
        panic("pgfault: sys_page_alloc: %e\n", r);
       
        void *src_addr = (void *) ROUNDDOWN(addr, PGSIZE);
        memmove(PFTEMP, src_addr, PGSIZE);

       if ((r = sys_page_map(0, PFTEMP, 0, src_addr, PTE_P | PTE_U | PTE_W)) < 0)
       panic("pgfault: sys_page_map: %e\n", r);
      
       if((sys_page_unmap(0, PFTEMP))<0)
       panic("pgfault/fork: couldn't unmap the page at PFTEMP");
 
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
/*For each writable or copy-on-write page in its address space below UTOP, the parent calls duppage, which should map the page copy-on-write into the address space of the child and then remap the page copy-on-write in its own address space. duppage sets both PTEs so that the page is not writeable, and to contain PTE_COW in the "avail" field to distinguish copy-on-write pages from genuine read-only pages.*/
static int
duppage(envid_t envid, unsigned pn)
{
     int r;
     uint32_t perm = PTE_P | PTE_COW | PTE_U;
     void * addr = (void *)(pn*PGSIZE);

     // LAB 4: Your code here.
    //If the page passed is marked copy-on-write or writable 
    if (uvpt[pn] & PTE_COW || uvpt[pn] & PTE_W) 
     {
     //then map the page at addr in parent to child at addr in its address space with perm COW
     if ((r = sys_page_map(thisenv->env_id, addr, envid, addr, perm)) < 0)
     panic("duppage: sys_page_map: %e\n", r);

     // remap the page at addr in parent with perm that contains COW
     if ((r = sys_page_map(thisenv->env_id, addr, thisenv->env_id, addr, perm)) < 0)
     panic("duppage: sys_page_map: %e\n", r);

    } 
   else // if the page is read-only , map this addr of the page too in the child 
   if((r = sys_page_map(thisenv->env_id, addr, envid, addr,PTE_P | PTE_U )) < 0)
   panic("duppage: sys_page_map: %e\n", r);

return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	//panic("fork not implemented");
        envid_t child_envid;
        int r;
        set_pgfault_handler(pgfault); // set the pagefault handler to pgfault which is defined above  

        // Allocate a new child environment.
        // The kernel will initialize it with a copy of our register state,
        // so that the child will appear to have called sys_exofork() too -
        // except that in the child, this "fake" call to sys_exofork()
        // will return 0 instead of the envid of the child.
        child_envid = sys_exofork();
        if (child_envid < 0)
                panic("fork: sys_exofork: %e", child_envid);
        if (child_envid == 0) {
                // We're the child.
                // The copied value of the global variable 'thisenv'
                // is no longer valid (it refers to the parent!).
                // Fix it and return 0.
                thisenv = &envs[ENVX(sys_getenvid())];
                return 0;
        }

        // We're the parent.
        // Eagerly copy our mappings into the child.
       uint32_t pgno=0;
       pte_t *pte;
       for (; pgno < PGNUM(UTOP - PGSIZE); pgno++) 
       {  //rounddown to the nearest multiple of NPDENTRIES
       uint32_t pdx = ROUNDDOWN(pgno, NPDENTRIES) / NPDENTRIES; // or use PDX macro with substitution of page numbers
       if ((uvpd[pdx] & PTE_P) == PTE_P && ((uvpt[pgno] & PTE_P) == PTE_P))  
// traverse through the uvpt array of the parents environment to find *pte and make sure there is mapping to every page
       {
       duppage(child_envid, pgno);
       }
      } 
 
        //allocate user exception stack for child
        if(sys_page_alloc(child_envid,(void *) (UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P)<0)    
        panic("fork: unable to allocate user exeception stack for the child"); 
  
        //copy the parent page fault handler entrypoint to child's environment, in this case _pgfault_upcall in lib/pfentry.S
 
       if ((r = sys_env_set_pgfault_upcall(child_envid, _pgfault_upcall)) < 0)
       panic("fork: sys_env_set_pgfault_upcall", r);
 
    
        // Now the child is ready to start running
        if ((r = sys_env_set_status(child_envid, ENV_RUNNABLE)) < 0)
                panic("sys_env_set_status: %e", r);

        return child_envid;
        
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
