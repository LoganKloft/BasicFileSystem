/************* cd_ls_pwd.c file **************/
#ifndef CDLDPWD
#define CDLWPWD

#include "type.h"
#include "util.c"
#include "symlink.c"

int cd(char* pathname)
{
  // (1)
  int ino = 0;
  if (!pathname || !strlen(pathname))
  {
    ino = 2;
    dev = root->dev;
  }
  else 
  {
    ino = getino(pathname);
  }

  if (!ino)
  {
    printf("cd> pathname: %s does not exist\n", pathname);
    return -1;
  }

  // check permissions
  if (!my_access(pathname, 'x'))
  {
    printf("cd> UID %d does not have access to %s\n", running->uid, pathname);
    return -1;
  }

  // (2)
  MINODE *mip = iget(dev, ino);
  // printf("cd: dev %d -> %d ino %d -> %d\n", running->cwd->dev, mip->dev, running->cwd->ino, mip->ino);

  // (3)
  if ((mip->INODE.i_mode & 0xF000) != 0x4000)
  {
    printf("cd> pathname: %s not a directory\n", pathname);
    iput(mip);
    return -1;
  }

  // (4)
  iput(running->cwd);

  // (5)
  running->cwd = mip;
}

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
int ls_file(MINODE *mip, char *name)
{
  int i = 0;
  char ftime[26];
  INODE *inode = &(mip->INODE);
  
  if ((inode->i_mode & 0xF000) == 0x8000) // if (S_ISREG())
    printf("%c",'-');
  if ((inode->i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    printf("%c",'d');
  if ((inode->i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    printf("%c",'l');
  for (i=8; i >= 0; i--){
    if (inode->i_mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]); // or print -
  }

  printf("%4d ",inode->i_links_count); // hard link count
  printf("%4d ",inode->i_gid); // gid
  printf("%4d ",inode->i_uid); // uid
  printf("%8d ",inode->i_size); // file size

  // print time
  time_t t = inode->i_ctime;
  strcpy(ftime, ctime(&t)); // print time in calendar form
  ftime[strlen(ftime)-1] = 0; // kill \n at end
  printf("%s ",ftime);

  // print name
  printf("%7s", name); // print file name

  if ((inode->i_mode & 0xF000)== 0xA000){
    // not using my_readlink because pollutes output
    char smlnk[61];
    int len = inode->i_size;
    strncpy(smlnk, inode->i_block, len);
    smlnk[len] = 0;
    printf(" -> %s", smlnk);
  }

  char buff[50];

  sprintf(buff, "[%d %d]", mip->dev, mip->ino);

  printf("%7s", buff);
  printf(" %d", mip->refCount);
  printf("\n");

  return 1;
}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  int dev = mip->dev;

  get_block(mip->dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE){
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    // display entry
    int ino = dp->inode;
    MINODE *mip = iget(dev, ino);
    ls_file(mip, temp);
    iput(mip);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

int ls(char* pathname)
{
  // Use CWD
  if (!pathname || !strlen(pathname)) 
  {
    printf("No argument for ls\n");
      dev = running->cwd->dev;
      return ls_dir(running->cwd);
  }

  // Use Path
  int ino = getino(pathname);

  // Path does not exist
  if (!ino)
  {
    printf("ls> pathname: %s does not exist\n", pathname);
    return -1;
  }

  MINODE *mip = iget(dev, ino);

  if (!mip)
  {
    printf("ls> ino: %d does not exist in memory", ino);
    iput(mip);
    return -1;
  }

  // directory
  if ((mip->INODE.i_mode & 0xF000) == 0x4000) ls_dir(mip); // directory
  else ls_file(mip, basename(pathname)); // file
  iput(mip);
}

char* rpwd(MINODE *wd)
{
  char my_name[256];
  // (1)
  if (wd == root) return;

  // (2)
  u32 my_ino;
  u32 parent_ino = findino(wd, &my_ino);

  // (3)
  MINODE *pip = iget(dev, parent_ino);

  // (4)
  findmyname(pip, my_ino, my_name);

  // (5)
  rpwd(pip);
  iput(pip); // I think

  // (6)
  printf("/%s", my_name);
}

char* pwd(MINODE *wd)
{
  int save_dev = dev;
  if (wd == root) printf("/");
  else rpwd(wd); putchar('\n');
  dev = save_dev;
}

#endif