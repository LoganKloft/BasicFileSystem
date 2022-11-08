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
    old_file_inode = getino(old_file);

    if (old_file_inode == 1)
    {
        printf("Old file doesn't exist\n");
        return -1;
    }

    old_mip = iget(dev, old_file_inode);

    if ((old_mip->INODE.i_mode & 0xF000) == 0x4000)
    {
        printf("Cannot hard link to a directory\n");
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

// THIS FUNCTION STILL HASN'T BEEN FIXED
int my_unlink(char* filename)
{
    // int ino;
    // MINODE *mip;

    // // (1). get filenmae’s minode:
    // ino = getino(filename);
    // mip = iget(dev, ino);
    // // check it’s a REG or symbolic LNK file; can not be a DIR
    // // (2). // remove name entry from parent DIR’s data block:
    // char parent[256], child[256];

    // strcpy(parent, dirname(filename));
    // strcpy(child, basename(filename));

    // pino = getino(parent);
    // pimp = iget(dev, pino);
    // rm_child(pmip, ino, child);
    // pmip->dirty = 1;
    // iput(pmip);
    // // (3). // decrement INODE’s link_count by 1
    // mip->INODE.i_links_count--;
    // // (4). if (mip->INODE.i_links_count > 0)
    // mip->dirty = 1; // for write INODE back to disk
    // // (5). 
    // else{ // if links_count = 0: remove filename
    // // deallocate all data blocks in INODE;
    // // deallocate INODE;
    // }
    // iput(mip); // release mip
}

#endif