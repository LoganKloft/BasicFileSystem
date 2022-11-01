#ifndef MISC1 
#define MISC1

#include "type.h"
#include "util.c"
#include "alloc.c"

int my_chmod(char *permissions, char *pathname)
{
    // (1) get the in-memory INODE of a file
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);

    // (2) modify permissions of INODE
    
    short p = mip->INODE.i_mode;
    // clear first 9 bits of p
    for (int i = 0; i < 9; i++)
    {
        p = p & ~(1 << i);
    }

    // set first 9 bits of p
    int dec = (int)strtol(permissions, 0, 8);
    int i = 0;
    while(dec > 0 && i < 9)
    {
        int bit = dec % 2;
        if (bit == 1) p = p | (1 << i);
        dec /= 2;
        i++;
    }

    // update i_mode
    mip->INODE.i_mode = p;

    // (3) INODE modified => mark as dirty
    mip->dirty = 1;

    // (4) write back
    iput(mip);
}

int my_utime(char *pathname)
{
    // (1) get the in-memory INODE of a file
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);

    // (2) modify permissions of INODE
    mip->INODE.i_atime = time(0L);

    // (3) INODE modified => mark as dirty
    mip->dirty = 1;

    // (4) write back
    iput(mip);
}

#endif