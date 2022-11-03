#ifndef RMDIR 
#define RMDIR 

#include "type.h"
#include "util.c"

int rm_child(MINODE *pmip, char *name)
{
    // (1) search parent INODE's data block(s) for the entry of name
    char buf[BLKSIZE];
    get_block(pmip->dev, pmip->INODE.i_block[0], buf);
    DIR *dp = (DIR *)buf;
    int prev_rec_len = dp->rec_len;
    int total_len = dp->rec_len;
    while(strncmp(dp->name, name, strlen(name)))
    {
        prev_rec_len = dp->rec_len;
        dp = (char *)dp + dp->rec_len;
        total_len += dp->rec_len;
    }
    
    // (2) delete name entry from parent directory
    if (total_len == 1024)
    {
        // (2.1) last entry
        int last_rec_len = dp->rec_len;
        dp = (char *)dp - prev_rec_len;
        dp->rec_len += last_rec_len;
    }
    else
    {
        // (2.2) intermediate entry
        int shift_rec_len = dp->rec_len;
        dp = (char *)dp + dp->rec_len;
        while(dp < buf + BLKSIZE)
        {
            memcpy((char *)dp - shift_rec_len, dp, dp->rec_len);
            dp = (char *)dp + dp->rec_len;
        }
    }

    // (3) update block of pmip
    put_block(pmip->dev, pmip->INODE.i_block[0], buf);
}

int my_rmdir(char *pathname)
{
    char buf[BLKSIZE];

    // (1) get in-memory INODE of pathname
    int ino = getino(pathname);
    if (ino == 0)
    {
        printf("rmdir> pathname: %s does not exist\n", pathname);
        return -1;
    }

    MINODE *mip = iget(dev, ino);

    // (2) verify MINODE is not busy and INODE is a DIR
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("rmdir> pathname: %s is not a directory\n", pathname);
        return -1;
    }

    if (mip->refCount > 1)
    {
        printf("rmdir> pathname: %s is busy with %d references\n", pathname, mip->refCount);
        return -1;
    }

    get_block(mip->dev, mip->INODE.i_block[0], buf);
    int count = 0;
    char temp[256];
    DIR *dp = (DIR *)buf;
    printf("  ino   rlen  nlen  name\n");
    while (dp < buf + BLKSIZE)
    {
        strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
        temp[dp->name_len] = 0;                // temp is a STRING
        printf("%4d  %4d  %4d    %s\n", 
            dp->inode, dp->rec_len, dp->name_len, temp);
        count++;
        dp = (char*)dp + dp->rec_len;
    }
    if (count > 2)
    {
        printf("rmdir> %s has %d entries\n", pathname, count);
        return -1;
    }

    // (3) get parent's ino and INODE
    int pino = findino(mip, &ino);
    MINODE *pmip = iget(mip->dev, pino);

    // (4) get name from parent DIR
    char name[256];
    findmyname(pmip, ino, name);

    // (5) remove name from parent DIR
    rm_child(pmip, name);

    // (6) decrement parent links_count and mark parent MINODE as dirty
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    // (7) deallocate pathname's data blocks and INODE
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);
}

#endif