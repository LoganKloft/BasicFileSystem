#ifndef MOUNTUMOUNT
#define MOUNTUMOUNT

#include "type.h"

int my_mount(char *name, char *mount_name)
{
    // // (1) ask for file system and mount point
    // char name[64] = {0};
    // printf("Enter disk: ");
    // fgets(name, 64, stdin);
    // name[strlen(name) - 1] = 0; // remove trailing new line

    // char mount_name[64] = {0};
    // printf("Enter mount point: ");
    // fgets(mount_name, 64, stdin);
    // mount_name[strlen(mount_name) - 1] = 0; // remove trailing new line

    if (strlen(name) == 0 && strlen(mount_name) == 0)
    {
        printf("dev ninodes nblocks bmap imap iblk name mname\n");
        printf("---------------------------------------------\n");
        for (int i = 0; i < NMTABLE; i++)
        {
            MOUNT *mptr = &mountTable[i];
            if (mptr->dev > 0)
            {
                printf("%3d %7d %7d %4d %4d %4d %s %s\n", mptr->dev, mptr->ninodes, mptr->nblocks, mptr->bmap, mptr->imap, mptr->iblk, mptr->name, mptr->mount_name);
            }
        }
        return 0;
    }

    printf("mounting %s at %s\n", name, mount_name);

    // (2) check whether filesys is already mounted
    for (int i = 0; i < NMTABLE; i++)
    {
        if (mountTable[i].dev == 0)
        {
            continue;
        }

        if (strcmp(mountTable[i].name, name) == 0)
        {
            // reject - already mounted
            printf("mount> %s already mounted\n", name);
            return -1;
        }
    }

    // allocate a new mountTable
    MOUNT *mptr = 0;
    for (int i = 0; i < NMTABLE; i++)
    {
        if (mountTable[i].dev == 0)
        {
            mptr = &mountTable[i];
            break;
        }
    }

    // failed to allocate a new mountTable
    if (mptr == 0)
    {
        printf("mount> no more mount tables available\n");
        return -1;
    }

    // (3) open filesys for RW and check magic number for EXT2
    int fd = open(name, O_RDWR);
    if (fd < 0)
    {
        // failed to open name
        printf("mount> failed to open %s for RDWR\n", name);
        return -1;
    }

    printf("new dev: %d\n", fd);

    char buf[BLKSIZE];
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;
    if (sp->s_magic != 0xEF53)
    {
        // not an ext2 filesystem
        printf("mount> %s is not an EXT2 filesystem\n", name);
        close(fd);
        return -1;
    }

    int ninodes = sp->s_inodes_count;
    int nblocks = sp->s_blocks_count;

    // (4) get ino and minode of mount_point
    int ino = getino(mount_name);
    MINODE *mip = iget(dev, ino);
    printf("mip: %d %d %d\n", mip->dev, mip->ino, mip->INODE.i_size);
    // (5) verify mount_point is a DIR and mount_point not busy
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        // not a dir
        printf("mount> %s is not a directory\n", mount_name);
        close(fd);
        return -1;
    }

    for (int i = 0; i < NPROC; i++)
    {
        if (proc[i].cwd->dev == fd && proc[i].cwd->ino == ino)
        {
            // mount_point busy
            printf("mount> P[%d]'s CWD is mount_point\n", proc[i].pid);
            close(fd);
            return -1;
        }
    }

    // (6) record mount information
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    int bmap = gp->bg_block_bitmap;
    int imap = gp->bg_inode_bitmap;
    int iblk = gp->bg_inode_table;

    mptr->dev = fd;
    mptr->ninodes = ninodes;
    mptr->nblocks = nblocks;
    mptr->bmap = bmap;
    mptr->imap = imap;
    mptr->iblk = iblk;
    mptr->mounted_inode = mip;
    strcpy(mptr->name, name);
    strcpy(mptr->mount_name, mount_name);

    // (7) mark mip
    mip->mounted = 1;
    mip->mptr = mptr;

    return 0;
}

int my_umount(char *filesys)
{
    // (1) search for filesys in mountTable
    MOUNT *mptr = 0;
    for (int i = 0; i < NMTABLE; i++)
    {
        if (mountTable[i].dev > 0 && strcmp(mountTable[i].name, filesys) == 0)
        {
            mptr = &mountTable[i];
            break;
        }
    }

    if (mptr == 0)
    {
        // not mounted
        printf("umount> %s is not mounted\n", filesys);
        return 0;
    }

    // (2) check whether any file still active in mounted filesys
    for (int i = 0; i < NMINODE; i++)
    {
        if (minode[i].dev == mptr->dev)
        {
            if (minode[i].refCount > 0)
            {
                // busy
                printf("umount> active file found in %s\n", filesys);
                printf("active file: [%d %d]\n", minode[i].dev, minode[i].ino);
                printf("refCount: %d\n", minode[i].refCount);
                return -1;
            }
        }
    }

    // (3) unmount
    close(mptr->dev);
    mptr->mounted_inode->mounted = 0;
    mptr->dev = 0;
    iput(mptr->mounted_inode);

    return 0;
}

#endif