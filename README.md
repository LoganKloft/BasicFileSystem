
               CS360 Project Level-1 DEMO RECORD 


Team Member Names IDs Manjesh Reddy Puram (11716685), Logan Kloft (11728076)
                     
                            Check List

     Commands              Expected Results           Observed Results
--------------------   -------------------------   ------------------------
1. startup (with an EMPTY diskiamge)
   ls:                  Show contents of / DIR      Only can see ., .., lost+found all as directories 10

2. mkdir dir1; ls:      Show /dir1 exists           Only can see ., .., lost+found, dir1 all as directories 10

   mkdir dir2; ls:      Show /dir1, /dir2 exist     Only can see ., .., lost+found, dir1, dir2 all as directories 10

3. mkdir dir1/dir3 
   ls dir1:             Show dir3 in dir1/          Only can see ., .., dir3 all as directories 10

4. creat file1          Show /file1 exists          Only can see ., .., lost+found, dir1, dir2 all as directories and file1 as a file 10

5. rmdir dir1           REJECT (dir1 not empty)     Only can see ., .., lost+found, dir1, dir2 all as directories and file1 as a file 10

6. rmdir dir2; ls:      Show dir2 is removed        Only can see ., .., lost+found, dir1 all as directories and file1 as a file 10

7. link file1 A;ls:     file1,A same ino,  LINK=2   Only can see ., .., lost+found, dir1 all as directories and file1, A both sharing the same INode and links are 2 and are files 10

8. unlink A; ls:        A deleted, file1's LINK=1   Only can see ., .., lost+found, dir1 all as directories and file1 link is 1 and is a file 10

9. symlink file1 B;ls:  ls must show   B->file1     Only can see ., .., lost+found, dir1 all as directories and file1, B with the B showing B -> file1 and are files 10

10.unlink file1; ls:    file1 deleted, B->file1     Only can see ., .., lost+found, dir1 all as directories and B and is a file 10

                                                                     Total = 100