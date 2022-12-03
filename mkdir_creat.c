#ifndef MKDIRCREAT
#define MKDIRCREAT

#include "type.h"
#include "util.c"
#include "alloc_dalloc.c"

int enter_name(MINODE *pmip, int ino, char *name)
{
    char buf[BLKSIZE];
    int need_length = 4 * ( (8 + strlen(name) + 3) / 4);

    for (int i = 0; i < 12; i++)
    {
        if (pmip->INODE.i_block[i] == 0)
        {
            // allocate new block
            int blk = balloc(pmip->dev);
            if (blk == 0)
            {
                printf("failed to allocate block in enter_name\n");
                return -1;
            }

            pmip->INODE.i_block[i] = blk;
            pmip->INODE.i_size += BLKSIZE;
            pmip->INODE.i_blocks += 2;
            pmip->dirty = 1;
        }

        // (1) get parent's data block into buf
        get_block(pmip->dev, pmip->INODE.i_block[i], buf);

        // (2) iterate to last DIR entry
        DIR *dp = (DIR *)buf;
        char *cp = buf;
        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        int ideal_length = 4 * ( (8 + dp->name_len + 3) / 4 );
        int remain = dp->rec_len - ideal_length;

        if (remain >= need_length)
        {
            // trim rec_len of last entry
            dp->rec_len = ideal_length;

            // add new last entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
            dp->inode = ino;
            dp->rec_len = remain;
            dp->name_len = strlen(name);
            strncpy(dp->name, name, strlen(name));

            // write data block to disk
            put_block(pmip->dev, pmip->INODE.i_block[i], buf);

            return 1;
        }
    }
}

int kmkdir(MINODE *pmip, char *basename)
{
    // (1) allocate INODE and disk block
    int ino = ialloc(pmip->dev);
    if (ino == 0)
    {
        printf("kmkdir> failed to allocate inode\n");
        return -1;
    }

    int bno = balloc(pmip->dev);
    if (bno == 0)
    {
        printf("kmkdir> failed to allocate block\n");
    }

    // (2) load INODE into MINODE and initialize INODE
    MINODE *mip = iget(pmip->dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED; // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = BLKSIZE; // size in bytes
    ip->i_links_count = 2; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = bno; // new DIR has one data block
    for (int i = 1; i < 15; i++) ip->i_block[i] = 0; // ip->i_block[1] to ip->i_block[14] = 0;
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk

    // (3) add . and ..
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    DIR *dp = (DIR *)buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(mip->dev, bno, buf); // write to blk on disk

    // (4) register (ino, basename) as DIR entry to parent INODE
    enter_name(pmip, ino, basename);
}

int my_mkdir(char *pathname)
{
    // (1) divide pathname into dirname and basename
    char pbuf[128];
    strcpy(pbuf, pathname);
    char *dname = dirname(pathname);
    char *bname = basename(pbuf);

    printf("dirname: %s\n", dname);
    printf("basename: %s\n", bname);

    // (2) check that dirname exists and is a DIR
    int pino = getino(dname);
    if (pino == 0)
    {
        printf("mkdir> dirname: %s does not exist\n", dname);
        return -1;
    }

    MINODE *pmip = iget(dev, pino);
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("mkdir> dirname: %s not a directory\n", dname);
        return -1;
    }

    // (3) check that basename does not exist in dirname
    if (search(pmip, bname))
    {
        printf("mkdir> basename: %s already exists in directory %s\n", bname, dname);
        return -1;
    }

    // (4)
    kmkdir(pmip, bname);

    // (5) increment pmip's link count, mark pmip as dirty
    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
}

int kcreat(MINODE *pmip, char *basename)
{
    // (1) allocate INODE and disk block
    int ino = ialloc(pmip->dev);
    if (ino == 0)
    {
        printf("kcreat> failed to allocate inode\n");
        return -1;
    }

    // (2) load INODE into MINODE and initialize INODE
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4; // regular file type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = 0; // size in bytes
    ip->i_links_count = 1; // links count=1
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 0; // LINUX: Blocks count in 512-byte chunks
    for (int i = 0; i < 15; i++) ip->i_block[i] = 0; // ip->i_block[0] to ip->i_block[14] = 0;
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk

    // (3) register (ino, basename) as DIR entry to parent INODE
    enter_name(pmip, ino, basename);
}

int my_creat(char *pathname)
{
    // (1) divide pathname into dirname and basename
    char pbuf[128];
    strcpy(pbuf, pathname);
    char *dname = dirname(pathname);
    char *bname = basename(pbuf);

    printf("dirname: %s\n", dname);
    printf("basename: %s\n", bname);

    // (2) check that dirname exists and is a DIR
    int pino = getino(dname);
    if (pino == 0)
    {
        printf("creat> dirname: %s does not exist\n", dname);
        return -1;
    }

    MINODE *pmip = iget(dev, pino);
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("creat> dirname: %s not a directory\n", dname);
        return -1;
    }

    // (3) check that basename does not exist in dirname
    if (search(pmip, bname))
    {
        printf("creat> basename: %s already exists in directory %s\n", bname, dname);
        return -1;
    }

    // (4)
    kcreat(pmip, bname);

    // mark pmip as dirty
    pmip->dirty = 1;
    iput(pmip);
}

#endif