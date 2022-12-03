/*********** util.c file ****************/
#ifndef UTIL
#define UTIL

#include "type.h"

MOUNT* getmptr(int dev)
{
   for (int i = 0; i < NMTABLE; i++)
   {
      if (mountTable[i].dev == dev)
      {
         return &mountTable[i];
      }
   }
   return 0;
}

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;
  int iblk = getmptr(dev)->iblk;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]    
       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

      //  printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);    // buf[ ] contains this INODE
       ip = (INODE *)buf + offset;  // this INODE in buf[ ] 
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       mip->dirty = 0;
       mip->mounted = 0;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)  // iput(): release a minode
{
   if (getmptr(mip->dev) == 0) return; // dev not open
 int i, block, offset;
 int iblk = getmptr(mip->dev)->iblk;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write INODE back to disk */
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/

   // get INODE of ino into buf[ ]    
   int ino = mip->ino;
   block    = (ino-1)/8 + iblk;
   offset = (ino-1) % 8;
   printf("[%d %d] block %d offset %d\n", mip->dev, mip->ino, block, offset);
   //printf("iput: ino=%d block=%d offset=%d\n", ino, block, offset);

   get_block(mip->dev, block, buf);    // buf[ ] contains this INODE
   ip = (INODE *)buf + offset;  // this INODE in buf[ ]

   // copy mip->INODE to buf
   *ip = mip->INODE;

   // put INODE into disk
   put_block(mip->dev, block, buf);

   mip->refCount = 0;
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
     temp[dp->name_len] = 0;                // temp is a STRING
     printf("%4d  %4d  %4d    %s\n", 
	    dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

     if (strcmp(temp, name)==0){            // compare name with temp !!!
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }

     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

int getino(char *pathname) // return ino of pathname   
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
  {
      dev = root->dev;
      return 2;
  }
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
  {
      mip = root;
      dev = mip->dev;
  }
  else
  {
      mip = running->cwd;
      dev = mip->dev;
  }

  mip->refCount++;         // because we iput(mip) later
  
  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

      if (!S_ISDIR(mip->INODE.i_mode))
      {
         printf("getino: %s is not a dir\n", name[i]);
         iput(mip);
         return 0;
      }

      if (strcmp(name[i], "..") == 0)
      {
         // check for upward traversal
         if (mip->dev > 3 && mip->ino == 2)
         {
            printf("Upward traversal\n");
            MOUNT *mptr = getmptr(mip->dev);
            iput(mip);
            printf("cross mounting point: dev=%d newdev=%d\n", dev, mptr->mounted_inode->dev);
            mip = mptr->mounted_inode;
            dev = mip->dev;
            ino = mip->ino;

            ino = search(mip, name[i]);
            mip=iget(dev, ino);
            printf("After upward [%d %d]\n", dev, ino);
            continue;
         }
      }
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      iput(mip);
      printf("getting mip of ino %d from dev %d\n", ino, dev);
      mip = iget(dev, ino);
      printf("[%d %d] mounted? %d\n", mip->dev, mip->ino, mip->mounted);

      if (mip->mounted == 1)
      {
         // check for downward traversal
         if (!strcmp(name[i], ".") == 0 && !strcmp(name[i], "..") == 0)
         {
            printf("Downward traversal\n");
            // go to mount table of mip
            MOUNT *mptr = mip->mptr;
            printf("[%d %d] is mounted on; cross mounting point newdev=%d\n", mip->dev, mip->ino, mptr->dev);

            // change global dev
            dev = mptr->dev;

            // iput current mip and set to root of mounted filesystem
            iput(mip);
            mip = iget(dev, 2);
            ino = mip->ino;
            printf("After downward: dev: %d, ino: %d\n", dev, ino);
         }
      }
   }

   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %d in MINODE = [%d, %d]\n",myino,parent->dev,parent->ino);
   ip = &(parent->INODE);

   /*** search for ino in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(parent->dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
     temp[dp->name_len] = 0;                // temp is a STRING
     printf("%4d  %4d  %4d    %s\n", 
	    dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

     if (dp->inode == myino){            // compare dp->inode with myino !!
        printf("found %d : name = %s\n", dp->inode, temp);
        strcpy(myname, temp);
        return dp->inode;
     }

     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
   // special case: mip is root (ino == 2) && dev > 3
   // then mip is of the mountTable
   if (mip->ino == 2 && mip->dev > 3)
   {
      MOUNT *mptr = getmptr(mip->dev);
      mip = mptr->mounted_inode;
      dev = mip->dev;
   }

  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.
  char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   ip = &(mip->INODE);

   /*** search for ino in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(mip->dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
     temp[dp->name_len] = 0;                // temp is a STRING
     printf("%4d  %4d  %4d    %s\n", 
	    dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

     if (!strcmp(temp, ".")){            // compare dp->inode with myino !!
         printf("found %s : ino = %d\n", temp, dp->inode);
         *myino = dp->inode;
     }

     if (!strcmp(temp, "..")) {
         printf("found %s : ino = %d\n", temp, dp->inode);
         return dp->inode;
     }

     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

#endif