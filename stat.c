#ifndef STAT 
#define STAT 

int my_stat(char* pathname)
{
    printf("*********  stat **********\n");
    // Use CWD
    if (!pathname || !strlen(pathname)) 
        return ls_dir(running->cwd);

    // Use Path
    int ino = getino(pathname);

    // Path does not exist
    if (!ino)
    {
        printf("ls> pathname: %s does not exist\n", pathname);
        return -1;
    }

    MINODE *mip = iget(dev, ino);

    if (!mip)
    {
        printf("ls> ino: %d does not exist in memory", ino);
        return -1;
    }

    // // directory
    // if ((mip->INODE.i_mode & 0xF000) == 0x4000) return ls_dir(mip);

    // // file
    // ls_file(mip, basename(pathname));

    char ftime[26];
    INODE *inode = &(mip->INODE);

    printf("File: %s\n", pathname);
    printf("Size: %d \t\tBlocks: %d \tIO Block: %d\t", inode->i_size, inode->i_blocks, 4096);
    if ((mip->INODE.i_mode & 0xF000) == 0x4000)
    {
        printf("directory\n");
    }
    else if ((inode->i_mode & 0xF000)== 0xA000)
    {
        printf("Symbolic Link\n");
    }
    else
    {
        if (inode->i_size == 0)
        {
            printf("regular empty file\n");
        }
        else
        {
            printf("regular file\n");
        }
    }

    printf("Inode: %d\t\tLinks: %d\n", mip->ino, inode->i_links_count);

    short j = inode->i_mode;
    j = (j << 6);
    j = (j >> 6);
    printf("Access: (0%o/", j);
    if ((inode->i_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c",'-');
    if ((inode->i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c",'d');
    if ((inode->i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c",'l');
    for (int i=8; i >= 0; i--){
    if (inode->i_mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]); // or print -
    }
    printf(")\t");

    printf("Uid: (%d)\tGid: (%d)\n", inode->i_uid, inode->i_gid);

    // print time
    time_t t = inode->i_atime;
    strcpy(ftime, ctime(&t)); // print time in calendar form
    ftime[strlen(ftime)-1] = 0; // kill \n at end

    printf("Access: %s\n", ftime);

    // print time
    t = inode->i_mtime;
    strcpy(ftime, ctime(&t)); // print time in calendar form
    ftime[strlen(ftime)-1] = 0; // kill \n at end
    
    printf("Modify: %s\n", ftime);

    // print time
    t = inode->i_ctime;
    strcpy(ftime, ctime(&t)); // print time in calendar form
    ftime[strlen(ftime)-1] = 0; // kill \n at end

    printf("Change: %s\n", ftime);

    printf("\n");

    iput(mip);

    return 1;
}

#endif