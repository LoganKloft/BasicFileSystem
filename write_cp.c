#ifndef WRITECP
#define WRITECP

#include "mkdir_creat.c"
#include "read_cat.c"
#include "open_close.c"

int my_write_str(char *fd_string, char *content)
{
    // (1) convert fd to integer
    int fd = atoi(fd_string);

    // (2) verify fd is opened and for correct mode
    if (fd < 0 || fd >= NFD)
    {
        printf("write> invalid fd\n");
        return 0;
    }

    OFT *oftp = running->fd[fd];
    if (!oftp)
    {
        printf("write> fd not an instance\n");
        return 0;
    }

    if (oftp->mode < 1 || oftp->mode > 3)
    {
        printf("write> fd not correct mode\n");
        return 0;
    }

    // (3) make my_write call
    char buf[BLKSIZE];
    strcpy(buf, content);

    int nbytes = strlen(buf);

    return my_write(fd, buf, nbytes);
}

int my_write(int fd, char buf[ ], int nbytes)
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    INODE *ip = &mip->INODE;

    int total_wrote = 0;
    int first = 0;
    while (nbytes > 0)
    {
        // compute logical block
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;

        // get block
        int blk = -1;
        if (lbk < 12)
        {
            // direct block
            if (ip->i_block[lbk] == 0)
            {
                // allocate new block
                ip->i_block[lbk] = balloc(mip->dev);
            }
            blk = ip->i_block[lbk];
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            // indirect block
            int ibuf[256];
            if (ip->i_block[12] == 0)
            {
                ip->i_block[12] = balloc(mip->dev);
                get_block(mip->dev, ip->i_block[12], ibuf);
                bzero(ibuf, BLKSIZE);
                put_block(mip->dev, ip->i_block[12], ibuf);
            }
            get_block(mip->dev, ip->i_block[12], ibuf);
            blk = ibuf[lbk - 12];
            if (blk == 0)
            {
                blk = ibuf[lbk - 12] = balloc(mip->dev);
                put_block(mip->dev, ip->i_block[12], ibuf);
            }
        }
        else {
            // double indirect block

            // check if [13] is allocated
            int ibuf[256];
            if (ip->i_block[13] == 0)
            {
                // if not, allocate new block
                ip->i_block[13] = balloc(mip->dev);
                // load block
                get_block(mip->dev, ip->i_block[13], ibuf);
                // bzero block
                bzero(ibuf, BLKSIZE);
                // changed: [13], save changed blocks
                put_block(mip->dev, ip->i_block[13], ibuf);
            }

            // get indirect block
            get_block(mip->dev, ip->i_block[13], ibuf);
            // get block of indirect block
            int first_index = (lbk - 256 - 12) / 256;
            first = ibuf[first_index];
            // check if allocated
            int iibuf[256];
            if (first == 0)
            {
                // if not, allocate new block
                first = balloc(mip->dev);
                ibuf[first_index] = first;
                // load block
                get_block(mip->dev, first, iibuf);
                // bzero block
                bzero(iibuf, BLKSIZE);
                // changed: [13] and allocated block, save changed blocks
                put_block(mip->dev, first, iibuf);
                put_block(mip->dev, ip->i_block[13], ibuf);
            }
            
            // get indirect block of indirect block
            get_block(mip->dev, first, iibuf);
            int second_index = (lbk - 256 - 12) % 256;
            int second = iibuf[second_index];
            // check if allocated
            int iiibuf[256]; // 
            if (second == 0)
            {
                // if not, allocate new block
                second = balloc(mip->dev);
                iibuf[second_index] = second;
                // load block
                get_block(mip->dev, second, iiibuf);
                // bzero block
                bzero(iiibuf, BLKSIZE);
                // changed: [1st indirect block] and allocated block, save changed blocks
                put_block(mip->dev, second, iiibuf);
                put_block(mip->dev, first, iibuf);
            }

            // get blk
            blk = second;
        }

        // write to block the lesser of remain and nbytes
        // printf("first: %d blk: %d\n", first, blk);
        char wbuf[BLKSIZE];
        bzero(wbuf, BLKSIZE);
        get_block(mip->dev, blk, wbuf);
        char *cp = wbuf + startByte;
        int remain = BLKSIZE - startByte;
        int write_amount = remain < nbytes ? remain : nbytes;
        memcpy(cp, buf, write_amount);
        oftp->offset += write_amount;
        nbytes -= write_amount;
        total_wrote += write_amount;
        if (oftp->offset > ip->i_size)
        {
            ip->i_size = oftp->offset;
        }
        put_block(mip->dev, blk, wbuf);
    }

    mip->dirty = 1;
    // printf("wrote %d char into file descriptor fd=%d\n", total_wrote, fd);
    return total_wrote;
}

int my_cp(char* source, char* destination)
{
    printf("Attempting to copy %s into %s\n", source, destination);

    int n = 0;
    int fd = open_file(source, "0");

    // check if destination exists
    int ino = getino(destination);
    if (ino == 0)
    {
        // does not exist, create file
        my_creat(destination);
    }

    int gd = open_file(destination, "1");

    if (fd == -1 || gd == -1)
    {
        if (fd == -1)
        {
            printf("Source file wasn't opened properly and now aborting from command.\n");
            close_file(fd);
        }
        if (gd == -1)
        {
            printf("Destination file wasn't opened properly and now aborting from command.\n");
            close_file(gd);
        }
        return -1;
    }

    char mybuff[BLKSIZE] = {0};

    memset(mybuff, '\0', BLKSIZE);

    while(n = my_read(fd, mybuff, BLKSIZE)){
        mybuff[n] = 0;
        my_write(gd, mybuff, n);  // notice the n in write()
        memset(mybuff, '\0', BLKSIZE);
    }

    close_file(fd);
    close_file(gd);
}

#endif