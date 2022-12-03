#ifndef SYMLINK 
#define SYMLINK 

#include "type.h"
#include "util.c"
#include "mkdir_creat.c" // my_creat
#include "alloc_dalloc.c" // set_bit and clr_bit

int my_symlink(char *pathname1, char *pathname2)
{
    // (1) check pathname1 exists and pathname2 does not
    int oino = getino(pathname1);
    if (!oino)
    {
        printf("symlink> %s does not exist\n", pathname1);
        return -1;
    }

    int nino = getino(pathname2);
    if (nino)
    {
        printf("symlink> %s already exists\n", pathname2);
    }

    // (2) create new file and change to type LNK type
    my_creat(pathname2);
    nino = getino(pathname2);
    MINODE *mip = iget(dev, nino);

    // clear 4 most significant bits of mode
    int mode = mip->INODE.i_mode;
    for (int i = 12; i < 16; i++)
    {
        mode = mode & ~(1 << i);
    }

    // set most significant bits of mode to 1010
    mode = mode | (1 << 15);
    mode = mode | (1 << 13);

    // set mode
    mip->INODE.i_mode = mode;

    // (3) update newfile
    memcpy(mip->INODE.i_block, pathname1, strlen(pathname1));
    mip->INODE.i_size = strlen(pathname1);
    mip->dirty = 1;
    iput(mip);

    // (4) update parent file of newfile
    char *pname = basename(dirname(pathname2));
    int pino = getino(pname);
    MINODE *pmip = iget(dev, pino);
    pmip->dirty = 1;
    iput(pmip);
}

int my_readlink(char *pathname, char *buffer)
{
    // (1) get pathname's INODE and verify LNK type
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    if (!S_ISLNK(mip->INODE.i_mode))
    {
        printf("readlink> pathname: %s is not a LNK type\n", pathname);
        iput(mip);
        return -1;
    }

    // (2) copy name from INODE's i_block[0] into buffer
    int len = mip->INODE.i_size;
    strncpy(buffer, mip->INODE.i_block, len);
    buffer[len] = 0;
    iput(mip);

    // (3) return file size
    return len;
}

#endif