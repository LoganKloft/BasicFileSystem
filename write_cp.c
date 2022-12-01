#ifndef WRITECP
#define WRITECP

#include "mkdir_creat.c"
#include "read_cat.c"
#include "open_close_lseek.c"

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
    printf("content: %s nbytes: %d fd: %d\n", buf, nbytes, fd);

    return my_write(fd, buf, nbytes);
}

int my_write(int fd, char buf[ ], int nbytes)
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    INODE *ip = &mip->INODE;

    int total_wrote = 0;
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
            int ibuf[256];
            if (ip->i_block[13] == 0)
            {
                ip->i_block[13] = balloc(mip->dev);
                get_block(mip->dev, ip->i_block[13], ibuf);
                bzero(ibuf, BLKSIZE);
                put_block(mip->dev, ip->i_block[13], ibuf);
            }
            get_block(mip->dev, ip->i_block[13], ibuf);

            // get second indirect block
            int second_lbk = (lbk - 256 - 12) / 256;
            
            int iibuf[256];
            if (ibuf[second_lbk] == 0)
            {
                ibuf[second_lbk] = balloc(mip->dev);
                get_block(mip->dev, ibuf[second_lbk], iibuf);
                bzero(iibuf, BLKSIZE);
                put_block(mip->dev, ip->i_block[13], ibuf);
                put_block(mip->dev, ibuf[second_lbk], iibuf);
            }
            get_block(mip->dev, ibuf[second_lbk], iibuf);
            
            blk = iibuf[(lbk - 256 - 12) % 256];
            if (blk == 0)
            {
                blk = iibuf[(lbk - 256 - 12) % 256] = balloc(mip->dev);
                char iiibuf[BLKSIZE];
                get_block(mip->dev, iibuf[(lbk - 256 - 12) % 256], iiibuf);
                bzero(iiibuf, BLKSIZE);
                put_block(mip->dev, ibuf[second_lbk], iibuf);
                put_block(mip->dev, iibuf[(lbk - 256 - 12) % 256], iiibuf);
            }
        }

        // write to block the lesser of remain and nbytes
        printf("blk: %d\n", blk);
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
    printf("wrote %d char into file descriptor fd=%d\n", total_wrote, fd);
    return nbytes;
}

int my_cp(char *src, char *dst)
{
    int fd = open_file(src, "0");

    // check if dst exists
    int ino = getino(dst);
    if (ino == 0)
    {
        // does not exist, create file
        my_creat(dst);
    }

    int gd = open_file(dst, "1");

    int n = 0;
    char buf[BLKSIZE];
    while(n = my_read(fd, buf, BLKSIZE))
    {
        my_write(gd, buf, n);
    }

    close_file(fd);
    close_file(gd);
}

#endif