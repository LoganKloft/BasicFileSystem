#ifndef ALLOC
#define ALLOC

#include "type.h"
#include "util.c"

int tst_bit(char *buf, int bit)
{
    int i = bit / 8; // byte
    int j = bit % 8; // bit
    return buf[i] & (1 << j);
}

int set_bit(char *buf, int bit)
{
    int i = bit / 8; // byte
    int j = bit % 8; // bit
    buf[i] |= (1 << j);
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];
    
    // dec free blocks count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev)
{
    int i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i = 0; i < ninodes; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            // free block
            set_bit(buf, i);
            put_block(dev, imap, buf);
            decFreeInodes(dev);

            printf("Allocated ino= %d\n", i + 1);
            return i + 1; // bits count from 0, ino from 1.
        }
    }

    return 0; // failed to allocate inode
}

int balloc(int dev)
{
    int i;
    char buf[BLKSIZE];

    // read block_bitmap block
    get_block(dev, bmap, buf);

    for (i = 0; i < nblocks; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            // free block
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            decFreeBlocks(dev);

            printf("Allocated block= %d\n", i);
            return i; // bits count from 0, block from 0 also
        }
    }

    return -1; // failed to allocate inode
}

#endif