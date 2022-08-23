#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
  int i;
  int shmid = -1;
  uint va = 0;
  acquire(&(shm_table.lock));

  pde_t* pgdir = myproc()->pgdir;

  for (i = 0; i< 64; i++) {
    if(shm_table.shm_pages[i].id == id){
      shmid = i;
      break;
    }
  }

  if(shmid > -1){
    shm_table.shm_pages[shmid].refcnt += 1;   
    va = PGROUNDUP(myproc()->sz) + PGSIZE;
    mappages(pgdir, (char*)va, PGSIZE, V2P(shm_table.shm_pages[shmid].frame), PTE_W|PTE_U);
    *pointer=(char *)va;
  }
  else{
    for (i = 0; i< 64; i++) {
      if(shm_table.shm_pages[i].id == 0){
        va = PGROUNDUP(myproc()->sz) + PGSIZE;
        myproc()->sz = va;
        shm_table.shm_pages[i].id = id;
        shm_table.shm_pages[i].frame = kalloc();
        shm_table.shm_pages[i].refcnt +=1;

        memset(shm_table.shm_pages[i].frame, 0, PGSIZE);

        mappages(pgdir, (char*)va, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
        *pointer=(char *)va;
        break;
      }  
    }
  }
  release(&(shm_table.lock));
  
return va; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
  int i;
  acquire(&(shm_table.lock));

  for (i = 0; i< 64; i++) {
    if(shm_table.shm_pages[i].id == id){
      shm_table.shm_pages[i].refcnt -= 1;

      if(shm_table.shm_pages[i].refcnt == 0){
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
      }
    }
  }

  release(&(shm_table.lock));

return 0; //added to remove compiler warning -- you should decide what to return
}
