#ifndef READCAT
#define READCAT

#include "open_close_lseek.c"

int read_file()
{
    // ask for fd
    char fd_s[32];
    printf("Enter an fd for read: ");
    fgets(fd_s, 32, stdin);
    fd_s[strlen(fd_s) - 1] = 0; // remove trailing new line
    int fd = atoi(fd_s);

    // ask for nbytes
    char bytes_s[32];
    printf("Enter number of bytes to read: ");
    fgets(bytes_s, 32, stdin);
    bytes_s[strlen(bytes_s) - 1] = 0; // remove trailing new line
    int nbytes = atoi(bytes_s);

    // verify fd is opened for RD or RW
    if (fd < 0 || fd >= NFD)
    {
        printf("read_file> invalid fd\n");
        return 0;
    }

    OFT *oftp = running->fd[fd];
    if (!oftp)
    {
        printf("read_file> fd not an instance\n");
        return 0;
    }

    if (oftp->mode == 0 || oftp->mode == 2)
    {
    }
    else
    {
        printf("read_file> fd not correct mode\n");
        return 0;
    }

    // create buf of size nbytes + 1 for storing nullbyte
    char buf[BLKSIZE];
    // make read call
    int result = my_read(fd, buf, nbytes);
    buf[nbytes] = 0;
    printf("result: %s\n", buf);
    return result;
}

int my_read(int fd, char buf[ ], int nbytes)
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    INODE *ip = &mip->INODE;

    int total_read = 0;
    int available = ip->i_size - oftp->offset;
    char *cq = buf;

    while (nbytes && available)
    {
        // compute logical block
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;

        // get block
        int blk = -1;
        if (lbk < 12)
        {
            blk = ip->i_block[lbk];
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            // indirect block
            int ibuf[256];
            get_block(mip->dev, ip->i_block[12], ibuf);
            blk = ibuf[lbk - 12];
        }
        else {
            // double indirect block
            int ibuf[256];
            get_block(mip->dev, ip->i_block[13], ibuf);

            // get second indirect block
            int second_lbk = (lbk - 256 - 12) / 256;
            
            int iibuf[256];
            get_block(mip->dev, ibuf[second_lbk], iibuf);
            
            blk = iibuf[(lbk - 256 - 12) % 256];
        }

        // read blk
        printf("lbk: %d blk: %d offset: %d\n", lbk, blk, oftp->offset);
        char rbuf[BLKSIZE];
        bzero(rbuf, BLKSIZE);
        get_block(mip->dev, blk, rbuf);
        char *cp = rbuf + startByte;
        int remain = BLKSIZE - startByte;
        // read the lesser of available and and nbytes
        int read_amount = available < nbytes ? available : nbytes;
        // then ensure that the amount is less than remain
        read_amount = read_amount < remain ? read_amount : remain;
        // copy rbuf contents to buf
        memcpy(cq, cp, read_amount);
        // update buf offset, nbytes, total_read, available, and offset
        cq += read_amount;
        nbytes -= read_amount;
        total_read += read_amount;
        available -= read_amount;
        oftp->offset += read_amount;
    }

    printf("read %d char from file descriptor fd=%d\n", total_read, fd);
    return total_read;
}

int my_cat(char *pathname)
{
    // intialize
    char buf[BLKSIZE], dummy = 0;
    int n;

    //
    int fd = open_file(pathname, "0");
    while (n = my_read(fd, buf, BLKSIZE))
    {
        buf[n] = 0;
        printf("%s", buf);
    }
    close_file(fd);
}

#endif