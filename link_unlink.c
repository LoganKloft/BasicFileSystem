#ifndef LINKUNLINK
#define LINKUNLINK

#include "type.h"
#include "util.c"
#include "mkdir_creat.c"
#include "rmdir.c"

int my_link(char* old_file, char* new_file)
{
    int old_file_inode, new_file_inode;
    MINODE *old_mip, *new_mip;
    // (1). // verify old_file exists and is not a DIR;

    // If the old file location is told to me root then assign dev as the root dev
    if (old_file[0] == '/')
    {
        dev = root->dev;
    }
    else // Else, change the dev to the current working directory
    {
        dev = running->cwd->dev;
    }

    old_file_inode = getino(old_file);

    if (old_file_inode == -1)
    {
        printf("Old file doesn't exist\n");
        return -1;
    }

    old_mip = iget(dev, old_file_inode);

    if ((old_mip->INODE.i_mode & 0xF000) == 0x4000)
    {
        printf("Cannot hard link to a directory\n");
        iput(old_mip);
        return -1;
    }
    // check omip->INODE file type (must not be DIR).
    // (2). // new_file must not exist yet:
    int newfileIno = getino(new_file);

    if (newfileIno != 0)
    {
        printf("New file already exists or has been linked already\n");
        return -1;
    }

    // (3). creat new_file with the same inode number of old_file:
    char parent[256], child[256];

    strcpy(parent, dirname(new_file));
    strcpy(child, basename(new_file));

    new_file_inode = getino(parent);

    new_mip = iget(dev, new_file_inode);
    // // creat entry in new parent DIR with same inode number of old_file
    enter_name(new_mip, old_file_inode, child);

    // (4). 
    old_mip->INODE.i_links_count++; // inc INODE’s links_count by 1
    old_mip->dirty = 1; // for write back by iput(omip)
    new_mip->dirty = 1; // for write back by iput(omip)

    iput(old_mip);
    iput(new_mip);
}

int my_unlink(char* filename)
{
    // ino = target i number, pino = parent i number
    int ino, pino;
    MINODE *mip, *pmip;

    // (1). get filenmae’s minode:
    // If the old file location is told to me root then assign dev as the root dev
    if (filename[0] == '/')
    {
        dev = root->dev;
    }
    else // Else, change the dev to the current working directory
    {
        dev = running->cwd->dev;
    }

    ino = getino(filename);

    if (ino == -1)
    {
        printf("File doesn't exist\n");
        return -1;
    }

    mip = iget(dev, ino);

    // check for permission to unlink file
    if (running->uid != mip->INODE.i_uid)
    {
        printf("unlink> UID %d is not the owner of %s\n", running->uid, filename);
        iput(mip);
        return -1;
    }

    // check it’s a REG or symbolic LNK file; can not be a DIR
    if ((mip->INODE.i_mode & 0xF000) == 0x4000)
    {
        printf("Cannot unlink a directory\n");
        iput(mip);
        return -1;
    }

    // (2). // remove name entry from parent DIR’s data block:
    char parent[256], child[256];

    strcpy(parent, dirname(filename));
    strcpy(child, basename(filename));

    pino = getino(parent);
    pmip = iget(dev, pino);

    rm_child(pmip, child);

    pmip->dirty = 1;

    iput(pmip);

    // (3). // decrement INODE’s link_count by 1
    mip->INODE.i_links_count--;

    // (4). 
    if (mip->INODE.i_links_count > 0)
        mip->dirty = 1; // for write INODE back to disk

    // (5). 
    else { // if links_count = 0: remove filename
        if (mip->INODE.i_block[0] != 0 && !S_ISLNK(mip->INODE.i_mode))
        {
            // deallocate all data blocks in INODE;
            bdalloc(mip->dev, mip->INODE.i_block[0]);
        }

        // deallocate INODE;
        idalloc(mip->dev, ino);
    }

    iput(mip); // release mip
}

#endif