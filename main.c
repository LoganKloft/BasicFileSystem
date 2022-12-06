/****************************************************************************
*         Logan Kloft & Manjesh Puram: mount root file system             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"
#include "util.c"
#include "alloc_dalloc.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
// #include "stat.c"
// #include "misc1.c"
#include "open_close.c"
#include "read_cat.c"
#include "write_cp.c"
#include "mount_umount.c"

int quit();

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;
  OFT *oftp;

  printf("init()\n");

  // (1)
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->next = 0;
    p->pid = i;           // pid = 0, 1, ..., NPROC - 1
    p->uid = p->gid = 0;    // uid = gid = 0: SUPER user
    p->ppid = 0;
    p->cwd = 0;             // CWD of process
    p->status = 0;

    // process starts with no opened files
    for (int j = 0; j < NFD; j++) p->fd[j] = 0;
  }

  // (2)
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }

  // (3)
  root = 0;

  // (4) all ofts are free
  for (i = 0; i < NOFT; i++)
  {
    oftp = &oft[i];
    oftp->refCount = 0;
  }

  // (5) set all dev = 0 for mountTables
  for (int i = 0; i < NMTABLE; i++)
  {
    mountTable[i].dev = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mounting root()\n");
  printf("Enter filesystem: ");
  char filesys[64] = {0};
  fgets(filesys, 64, stdin);
  filesys[strlen(filesys) - 1] = 0;

  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(filesys, O_RDWR)) < 0){
    printf("open %s failed\n", filesys);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  int bfree = sp->s_free_blocks_count;
  int ifree = sp->s_free_inodes_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);
  printf("nblocks=%d bfree=%d ninodes=%d ifree=%d\n", nblocks, bfree,ninodes, ifree);

  // update mountTable[0]
  MOUNT *mptr = &mountTable[0];
  mptr->dev = dev;
  mptr->ninodes = ninodes;
  mptr->nblocks = nblocks;
  mptr->bmap = bmap;
  mptr->imap = imap;
  mptr->iblk = iblk;
  strcpy(mptr->name, filesys);
  strcpy(mptr->mount_name, "/");

  root = iget(dev, 2);

  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = 1;
  running->cwd = iget(dev, 2);

  // WRTIE code here to create P1 as a USER process
  printf("creating P1 as USER process\n");
  proc[1].uid = 2;
  proc[1].cwd = iget(dev, 2);
  proc[1].status = 1;
  running->next = &proc[1];

  printf("root refCount = %d\n", root->refCount);
}

void menu()
{
    printf("****************** Menu ******************\n");
    printf("ls     cd      mkdir      creat     rmdir\n");
    printf("link   unlink  symlink    quit      open\n");
    printf("close  lseek   read       cat       write\n");
    printf("cp     mount   umount     pfd       cs\n");
    printf("fork   ps      kill\n");
    printf("================== Usage =================\n");
    printf("mkdir filename\n");
    printf("link  oldfile  newfile");
    printf("mount filesys  dir\n");
    printf("open  filename mode (0|1|2|3 for R|W|RW|AP)\n");
    printf("write fd       text_string\n");
    printf("read  fd       nbytes\n");
    printf("pfd   (display opened file descriptors)\n");
    printf("cs    (switch process)\n");
    printf("fork  (fork child process)\n");
    printf("ps    (show process queue)\n");
    printf("kill  pid (kill a process)\n");
    printf("*****************************************\n");
}

int main(int argc, char *argv[ ])
{
  init();  
  mount_root();
  
  while(1){
    printf("input> ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s", cmd, pathname);
    printf("cmd=%s pathname=%s\n", cmd, pathname);
  
    if (strcmp(cmd, "ls") == 0) ls(pathname);
    else if (strcmp(cmd, "cd") == 0) cd(pathname);
    else if (strcmp(cmd, "pwd") == 0) pwd(running->cwd);
    else if (strcmp(cmd, "mkdir") == 0) my_mkdir(pathname);
    else if (strcmp(cmd, "creat") == 0) my_creat(pathname);
    else if (strcmp(cmd, "rmdir") == 0) my_rmdir(pathname);
    else if (strcmp(cmd, "link") == 0)
    {
      char pathname2[256];
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      my_link(pathname, pathname2);
    }
    else if (strcmp(cmd, "unlink") == 0) my_unlink(pathname);    
    else if (strcmp(cmd, "symlink") == 0) 
    {
      char pathname2[256];
      sscanf(line, "%s %s %s", cmd, pathname, pathname2);
      my_symlink(pathname, pathname2);
    }
    // else if (strcmp(cmd, "stat") == 0) my_stat(pathname);
    // else if (strcmp(cmd, "chmod") == 0)
    // {
    //   char permissions[32];
    //   sscanf(line, "%s %s %s", cmd, permissions, pathname);
    //   my_chmod(permissions, pathname);
    // }
    // else if (strcmp(cmd, "utime") == 0) my_utime(pathname);
    else if (strcmp(cmd, "quit") == 0) quit();
    else if (strcmp(cmd, "open") == 0)
    {
      char mode_string[32];
      sscanf(line, "%s %s %s", cmd, pathname, mode_string);
      open_file(pathname, mode_string);
    }
    else if (strcmp(cmd, "pfd") == 0) pfd();
    else if (strcmp(cmd, "close") == 0) close_file_str(pathname);
    else if (strcmp(cmd, "lseek") == 0)
    {
      char position[32];
      sscanf(line, "%s %s %s", cmd, pathname, position);
      my_lseek_str(pathname, position);
    }
    else if (strcmp(cmd, "write") == 0)
    {
      char content[BLKSIZE];
      sscanf(line, "%s %s %s", cmd, pathname, content);
      my_write_str(pathname, content);
    }
    else if (strcmp(cmd, "read") == 0)
    {
      char amount[BLKSIZE];
      sscanf(line, "%s %s %s", cmd, pathname, amount);
      printf("read %d", read_file(pathname, amount));
    }
    else if (strcmp(cmd, "cat") == 0)
    {
      my_cat(pathname);
    }
    else if (strcmp(cmd, "cp") == 0)
    {
      char dst[BLKSIZE];
      sscanf(line, "%s %s %s", cmd, pathname, dst);
      my_cp(pathname, dst);
    }
    else if (strcmp(cmd, "mount") == 0)
    {
      char mount_name[64] = {0};
      sscanf(line, "%s %s %s", cmd, pathname, mount_name);
      my_mount(pathname, mount_name);
    }
    else if (strcmp(cmd, "umount") == 0) my_umount(pathname);
    else if (strcmp(cmd, "cs") == 0) my_cs();
    else if (strcmp(cmd, "fork") == 0) my_fork();
    else if (strcmp(cmd, "ps") == 0) my_ps();
    else if (strcmp(cmd, "kill") == 0) my_kill(pathname);
    else if (strcmp(cmd, "menu") == 0) menu();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    // if closed dev skip
    if (getmptr(mip->dev) == 0) continue;
    if (mip->refCount && mip->dirty)
      mip->refCount = 1;
      iput(mip);
  }
  printf("see you later, alligator\n");
  exit(0);
}
