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
#include "stat.c"
#include "misc1.c"

int quit();

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  // (1)
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i+1;           // pid = 1, 2, ..., NPROC
    p->uid = p->gid = 0;    // uid = gid = 0: SUPER user
    p->cwd = 0;             // CWD of process
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
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");

  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
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

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  root = iget(dev, 2);

  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);

  // WRTIE code here to create P1 as a USER process
  printf("creating P1 as USER process\n");
  proc[1].cwd = iget(dev, 2);

  printf("root refCount = %d\n", root->refCount);
}

int main(int argc, char *argv[ ])
{
  init();  
  mount_root();
  
  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readlink|stat|chmod|utim|quit] ");
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
    // else if (strcmp(cmd, "link") == 0) my_link(pathname);
    // else if (strcmp(cmd, "unlink") == 0) my_unlink(pathname);
    // else if (strcmp(cmd, "symlink") == 0) my_symlink(pathname);
    // else if (strcmp(cmd, "readlink") == 0) my_readlink(pathname);
    // else if (strcmp(cmd, "stat") == 0) my_stat(pathname);
    else if (strcmp(cmd, "chmod") == 0)
    {
      char permissions[32];
      sscanf(line, "%s %s %s", cmd, permissions, pathname);
      my_chmod(permissions, pathname);
    }
    else if (strcmp(cmd, "utime") == 0) my_utime(pathname);
    else if (strcmp(cmd, "quit") == 0) quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dirty)
      mip->refCount = 1;
      iput(mip);
  }
  printf("see you later, alligator\n");
  exit(0);
}
