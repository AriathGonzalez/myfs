#ifndef __MY_FUSE_IMPL__
#define __MY_FUSE_IMPL__

#include <stdint.h>

/*
+--------------------------------------------------+
|               Disk Partition Layout              |
+--------------------------------------------------+
|   Superblock (1 block)                           |
|   Size: 1024 Bytes                               |
|   Address: 0x0000                                |
+--------------------------------------------------+
|   Inode Bitmap (2 blocks)                        |
|   Size: 2,048 Bytes                              |
|   Address: 0x0400                                |
+--------------------------------------------------+
|   Block Bitmap (13 blocks)                       |
|   Size: 13,312 Bytes                             |
|   Address: 0x0C00                                |
+--------------------------------------------------+
|   Inode Table (2500 blocks)                      |
|   Size: 2,560,000 Bytes                          |
|   Address: 0x4000                                |
+--------------------------------------------------+
|   Data Region (Remaining)                        |
|   Address: 0x2800000                             |
+--------------------------------------------------+
*/

/* Definitions and type declarations */

// Magic number
#define MAGIC_NUMBER ((uint32_t)0xaddbeef)

// Maximum length of a file name
#define MAX_FILE_NAME_LENGTH 255

// Maximum number of inodes
#define MAX_FILE_NUM 10000  

// Maximum number of simultaneously open files
#define MAX_OPEN_FILES 10

// Start block for inode bitmap
#define INODE_MAP_START 1  

// Start block for block bitmap
#define BLOCK_MAP_START 3 

// Start block for inode table
#define INODE_TABLE_START 16 

// Start block for block table
#define BLOCK_TABLE_START 2516 

// Block size in bytes
#define BLOCK_SIZE 1024

// File and directory types
#define FILE_TYPE 1
#define DIRECTORY_TYPE 2

// Maximum directory levels
#define MAX_LEVEL 10  

// File descriptor array for open files
int fd[MAX_OPEN_FILES]; 

// Current working directory name
char cwd_name[100]; 

// Current working directory's inode number
int cwd_inode;  

/* Memory Allocation STRUCTS */

// Super block structure
typedef struct super_block {
  uint32_t magic_number; // Magic number to identify filesystem type
  uint32_t block_num;    // Total number of data blocks
  uint32_t inode_num;    // Total number of inode blocks
  uint32_t free_block;   // Total number of free data blocks
  uint32_t free_inode;   // Total number of free inode blocks
} super_block;

// Bitmap structure for block allocation
typedef struct bmap {
  unsigned char map[BLOCK_SIZE]; // Bitmap array
} bmap;

// Inode structure
typedef struct inode {
    uint8_t type;                // Type: 1 for file, 2 for directory
    uint32_t num;                // Inode number
    uint64_t size;               // Size of file in bytes
    uint32_t uid;                // User ID
    uint32_t gid;                // Group ID
    char mode[11];               // File permissions
    char name[MAX_FILE_NAME_LENGTH];  // File name
    uint32_t blocks[10];         // Direct blocks (10 KB each)
    uint32_t ind_blocks[30];     // Indirect blocks (7.5 MB)
    uint8_t unused[15];          // Padding to align struct size
} inode;

// Structure for indirect blocks
typedef struct ind_block {
  int blocks[256]; // Pointers to data blocks
} ind_block;

// Directory entry structure
typedef struct directory_entry {
  int inode;       // Inode number
  int type;        // Type: file or directory
  int length;      // Length of file name
  char name[20];   // File or directory name
} directory_entry;

// Directory block structure
typedef struct directory {
  struct directory_entry directory_entries[32]; // Directory entries
} directory;

/* Start fuse helper methods */

void handler(void *fsptr, size_t fssize);


/* END fuse helper methods */

/* START fuse functions declarations */

int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr, uid_t uid,
                          gid_t gid, const char *path, struct stat *stbuf);
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr);
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path);
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *path);
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path);
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path);
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to);
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset);
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path);
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset);
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size,
                        off_t offset);
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]);
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs *stbuf);
                         
// END of fuse function declarations

#endif
