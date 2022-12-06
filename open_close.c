#ifndef OPENCLOSE
#define OPENCLOSE

#include "type.h"
#include "util.c"
#include "alloc_dalloc.c"

int pfd()
{
    OFT *oftp;

    printf("fd mode ref offset inode\n");
    printf("-- ---- --- ------ -----\n");
    for (int i = 0; i < NFD; i++)
    {
        oftp = running->fd[i];
        if (oftp)
        {
            char* mode_string;
            switch(oftp->mode)
            {
                case 0:
                    mode_string = "READ";
                    break;
                case 1:
                    mode_string = "WRIT";
                    break;
                case 2:
                    mode_string = "RDWR";
                    break;
                case 3:
                    mode_string = "APND";
                    break;
                default:
                    mode_string = "OOPS";
            }
            printf("%2d %s %3d %6d [%d, %d]\n", i, mode_string, oftp->refCount, oftp->offset, oftp->minodePtr->dev, oftp->minodePtr->ino);
        }
    }
    return 1;
}

int truncate(MINODE *mip)
{
    // (1) release mip's data blocks
    INODE *inode = &mip->INODE;

    // release direct blocks
    for (int i = 0; i < 12; i++)
    {
        if (inode->i_block[i])
        {
            bdalloc(mip->dev, inode->i_block[i]);
            inode->i_block[i] = 0;
        }
    }

    // release indirect blocks
    if (inode->i_block[12])
    {
        int buf[256];
        bzero(buf, BLKSIZE);
        get_block(mip->dev, inode->i_block[12], buf);
        for (int i = 0; i < 256; i++)
        {
            if (buf[i])
            {
                bdalloc(mip->dev, buf[i]);
                buf[i] = 0;
            }
        }
        bdalloc(mip->dev, inode->i_block[12]);
        inode->i_block[12] = 0;
    }
    
    // release double indirect blocks
    if (inode->i_block[13])
    {
        int buf[256];
        bzero(buf, BLKSIZE);
        get_block(mip->dev, inode->i_block[13], buf);
        for (int i = 0; i < 256; i++)
        {
            if (buf[i])
            {
                int buf2[256];
                bzero(buf2, BLKSIZE);
                get_block(mip->dev, buf[i], buf2);

                for (int j = 0; j < 256; j++)
                {
                    if (buf2[j])
                    {
                        bdalloc(mip->dev, buf2[j]);
                        buf2[j] = 0;
                    }
                }

                bdalloc(mip->dev, buf[i]);
                buf[i] = 0;
            }
        }
        bdalloc(mip->dev, inode->i_block[13]);
        inode->i_block[13] = 0;
    }

    // (2) update time field
    mip->INODE.i_mtime = time(0L); // update modify time

    // (3) update size and mark as dirty
    mip->INODE.i_size = 0;
    mip->dirty = 1;
}

int open_file(char* pathname, char* mode_string)
{
    // (1) extract mode to open as
    int mode = atoi(mode_string);
    printf("mode: %s %d\n", mode_string, mode);
    if (mode < 0 || mode > 3)
    {
        printf("open> incorrect mode: %d\n", mode);
        return -1;
    }

    // (2) pathname's inumber, minode pointer
    int ino = getino(pathname);
    if (ino == 0)
    {
        printf("open> incorrect pathname: %s\n", pathname);
        return -1;
    }

    MINODE *mip = iget(dev, ino);
    if (mip == 0)
    {
        printf("open> no free minodes left\n");
        return -1;
    }

    // (3) check if regular file and permission OK

    // check if regular
    if (!S_ISREG(mip->INODE.i_mode))
    {
        printf("open> not a regular file\n");
        iput(mip);
        return -1;
    }

    // check if already opened in 1 W | 2 RW | 3 APP
    for (int i = 0; i < NFD; i++)
    {
        if (running->fd[i] == 0) continue; // not an oft
        if (running->fd[i]->minodePtr->ino == ino)
        {
            // found related file, check if open in non-read mode
            if (running->fd[i]->mode > 0)
            {
                iput(mip);
                printf("open> file opened in non-read mode already\n");
                return -1;
            }
        }
    }

    // (4) allocate a free OpenFileTable
    OFT *oftp = oget();
    if (oftp == 0)
    {
        iput(mip);
        printf("open> no free OpenFileTables\n");
        return -1;
    }

    oftp->mode = mode;
    oftp->refCount = 1;
    oftp->minodePtr = mip;

    // (5) set oft offset
    switch(mode)
    {
        case 0:
            oftp->offset = 0;
            break;
        case 1:
            truncate(mip);
            oftp->offset = 0;
            break;
        case 2:
            oftp->offset = 0;
            break;
        case 3:
            oftp->offset = mip->INODE.i_size;
            break;
        default:
            printf("open> invalid mode: %d", mode);
            iput(mip);
            oftp->refCount = 0;
            return -1;
    }

    // (6) set running PROC's lowest open fd to oftp
    int i = 0;
    int flag = 1;
    for (; i < NFD; i++)
    {
        if (running->fd[i] == 0)
        {
            running->fd[i] = oftp;
            flag = 0;
            break;
        }
    }

    if (flag)
    {
        printf("open> all fd's of running process in use\n");
        iput(mip);
        oftp->refCount = 0;
        return -1;
    }

    // (7) update INODE's time field

    mip->INODE.i_atime = time(0L);
    if (oftp->mode > 0)
    {
        mip->INODE.i_mtime = time(0L);
    }
    mip->dirty = 1;

    // (8) return the file descriptor
    pfd();
    return i;
}

int close_file(int fd)
{
    // (1) verify fd is within range
    if (fd < 0 || fd >= NFD)
    {
        printf("close> not a valid file descriptor\n");
        return -1;
    }

    // (2) verify fd points to an OFT instance
    OFT *oftp = running->fd[fd];
    if (!oftp)
    {
        printf("close> file descriptor does not point to an instance\n");
        return -1;
    }

    // (3) remove fd from list and iput if last refCount
    running->fd[fd] = 0;
    oftp->refCount--;
    if (oftp->refCount > 0) 
    {
        return 0;
    }

    // last user of this OFT entry
    MINODE *mip = oftp->minodePtr;
    iput(mip);

    pfd();
    return 0;
}

int close_file_str(char* fd_string)
{
    int fd = atoi(fd_string);
    return close_file(fd);
}

int my_lseek(int fd, int position)
{
    // check boundaries of fd and position
    if (fd < 0 || fd >= NFD)
    {
        printf("lseek> not a valid file descriptor\n");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    if (!oftp)
    {
        printf("lseek> file descriptor not in use\n");
        return -1;
    }

    if (position < 0 || position >= oftp->minodePtr->INODE.i_size)
    {
        printf("lseek> not a valid position\n");
        return -1;
    }

    // save original position and update new position
    int original = oftp->offset;
    oftp->offset = position;
    
    // return the original (saved) position
    return original;
}

int my_lseek_str(char* fd_string, char* position_string)
{
    // convert parameters to their integer version
    int fd = atoi(fd_string);
    int position = atoi(position_string);

    return my_lseek(fd, position);
}

// does not verify mode (can duplicate R, RW, APP)
int dup(int fd)
{
    if (fd < 0 || fd >= NFD)
    {
        printf("dup> fd not valid\n");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    if (!oftp)
    {
        printf("dup> fd does not point to an instance\n");
        return -1;
    }

    int i = 0;
    for (; i < NFD; i++)
    {
        if (!running->fd[i])
        {
            running->fd[i] = oftp;
        }
    }

    if (i == NFD)
    {
        printf("dup> no free file descriptors\n");
        return -1;
    }

    oftp->refCount++;
    return i;
}

// does not verify modes (can duplicate R, RW, APP)
int dup2(int fd, int gd)
{
    if (fd < 0 || fd >= NFD)
    {
        printf("dup> fd not valid\n");
        return -1;
    }

    if (gd < 0 || gd >= NFD)
    {
        printf("dup> gd not valid\n");
        return -1;
    }

    OFT *foftp = running->fd[fd];
    if (!foftp)
    {
        printf("dup> fd does not point to an instance\n");
        return -1;
    }

    if (gd == fd)
    {
        return gd;
    }

    OFT *goftp = running->fd[gd];
    if (goftp)
    {
        close_file(gd);
    }

    running->fd[gd] = running->fd[fd];
    foftp->refCount++;

    return gd;
}

#endif