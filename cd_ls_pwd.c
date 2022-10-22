/************* cd_ls_pwd.c file **************/
int cd()
{
  printf("cd: under construction READ textbook!!!!\n");

  // READ Chapter 11.7.3 HOW TO chdir
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
  u32 t = inode->i_ctime;
  strcpy(ftime, ctime(&t)); // print time in calendar form
  ftime[strlen(ftime)-1] = 0; // kill \n at end
  printf("%s ",ftime);

  // print name
  printf("%s", name); // print file name

  // print -> linkname if symbolic file
  // if ((inode->i_mode & 0xF000)== 0xA000){
  //   // use readlink() to read linkname
  //   char linkname[256];
  //   readlink(name, linkname, 256);
  //   printf(" -> %s", linkname); // print linked name
  // }
  printf("\n");

  return 1;
}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE){
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    // display entry
    int ino = dp->inode;
    MINODE *mip = iget(dev, ino);
    ls_file(mip, temp);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

int ls(char* pathname)
{
  // Use CWD
  if (!pathname || !strlen(pathname)) return ls_dir(running->cwd);

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
    return -1;
  }

  // directory
  if ((mip->INODE.i_mode & 0xF000) == 0x4000) return ls_dir(mip);

  // file
  ls_file(mip, basename(pathname));
}

char *pwd(MINODE *wd)
{
  printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
}



