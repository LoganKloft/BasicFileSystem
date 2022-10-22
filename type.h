/*************** type.h file for LEVEL-1 ****************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// struct ext2_super_block {
//   u32 s_inodes_count; /* Inodes count */
//   u32 s_blocks_count; /* Blocks count */
//   u32 s_r_blocks_count; /* Reserved blocks count */
//   u32 s_free_blocks_count; /* Free blocks count */
//   u32 s_free_inodes_count; /* Free inodes count */
//   u32 s_first_data_block; /* First Data Block */
//   u32 s_log_block_size; /* Block size */
//   u32 s_log_cluster_size; /* Allocation cluster size */
//   u32 s_blocks_per_group; /* # Blocks per group */
//   u32 s_clusters_per_group; /* # Fragments per group */
//   u32 s_inodes_per_group; /* # Inodes per group */
//   u32 s_mtime; /* Mount time */
//   u32 s_wtime; /* Write time */
//   u16 s_mnt_count; /* Mount count */
//   s16 s_max_mnt_count; /* Maximal mount count */
//   u16 s_magic; /* Magic signature */
//   // more non-essential fields
//   u16 s_inode_size; /* size of inode structure */
// }
typedef struct ext2_super_block SUPER;

// struct ext2_group_desc {
//   u32 bg_block_bitmap; // Bmap block number
//   u32 bg_inode_bitmap; // Imap block number
//   u32 bg_inode_table; // Inodes begin block number
//   u16 bg_free_blocks_count; // THESE are OBVIOUS
//   u16 bg_free_inodes_count;
//   u16 bg_used_dirs_count;
//   u16 bg_pad; // ignore these
//   u32 bg_reserved[3];
// };
typedef struct ext2_group_desc  GD;

// struct ext2_inode {
//   u16 i_mode; // 16 bits = |tttt|ugs|rwx|rwx|rwx|
//   u16 i_uid; // owner uid
//   u32 i_size; // file size in bytes
//   u32 i_atime; // time fields in seconds
//   u32 i_ctime; // since 00:00:00,1-1-1970
//   u32 i_mtime;
//   u32 i_dtime;
//   u16 i_gid; // group ID
//   u16 i_links_count; // hard-link count
//   u32 i_blocks; // number of 512-byte sectors
//   u32 i_flags; // IGNORE
//   u32 i_reserved1; // IGNORE
//   u32 i_block[15]; // See details below
//   u32 i_pad[7]; // for inode size = 128 bytes
// };
typedef struct ext2_inode       INODE;

// struct ext2_dir_entry_2{
//   u32 inode; // inode number; count from 1, NOT 0
//   u16 rec_len; // this entry’s length in bytes
//   u8 name_len; // name length in bytes
//   u8 file_type; // not used
//   char name[EXT2_NAME_LEN]; // name: 1-255 chars, no ending NULL
// };
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2

typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;

typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int          uid;      // user ID
  int          gid;
  MINODE      *cwd;      // CWD directory pointer  
}PROC;
