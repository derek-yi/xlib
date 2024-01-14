//vma_test.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>

static int pid;
module_param(pid, int, 0644);

static void printit(struct task_struct *tsk)
{
  struct mm_struct *mm;
  struct vm_area_struct *vma;
  int j = 0;
  unsigned long start, end, length;

  mm = tsk->mm;
  pr_info("mm_struct addr = 0x%p\n", mm);
  vma = mm->mmap;

  /* 使用 mmap_sem 读写信号量进行保护 */
  down_read(&mm->mmap_sem);
  pr_info("vmas: vma start end length\n");

  while (vma) {
      j++;
      start = vma->vm_start;
      end = vma->vm_end;
      length = end - start;
      pr_info("%6d: %16p %12lx %12lx %8ld\n",
          j, vma, start, end, length);
      vma = vma->vm_next;
  }
  up_read(&mm->mmap_sem);
}

static int __init vma_init(void)
{
  struct task_struct *tsk;
  /* 如果插入模块时未定义 pid 号，则使用当前 pid */
  if (pid == 0) {
      tsk = current;
      pid = current->pid;
      pr_info("using current process\n");
  } else {
      tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
  }
  if (!tsk)
      return -1;
  pr_info(" Examining vma's for pid=%d, command=%s\n", pid, tsk->comm);
  printit(tsk);
  return 0;
}

static void __exit vma_exit(void)
{
  pr_info("Module exit\n");
}

module_init(vma_init);
module_exit(vma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mr Yu");
MODULE_DESCRIPTION("vma test");


/*
root:/data # cat /proc/4766/maps
00400000-0047c000 r-xp 00000000 103:23 6918                              /data/vma
0048b000-0048e000 rw-p 0007b000 103:23 6918                              /data/vma
0048e000-0048f000 rw-p 00000000 00:00 0
38382000-383a4000 rw-p 00000000 00:00 0                                  [heap]
78941af000-78941fb000 rw-p 00000000 00:00 0
78941fb000-78941fc000 r--p 00000000 00:00 0                              [vvar]
78941fc000-78941fd000 r-xp 00000000 00:00 0                              [vdso]
7fc0ed3000-7fc0f9d000 rw-p 00000000 00:00 0                              [stack]
*/
 



