#ifndef __MY_FUSE_IMPL__
#define __MY_FUSE_IMPL__

#include <stdint.h>

// Constants and type definitions
#define MAGIC_NUMBER ((uint32_t)0xADDBEEF)
#define NAME_MAX_LEN ((size_t)255)
#define BLOCK_SIZE ((size_t)1024)

typedef uint64_t offset;  
typedef unsigned int u_int;

/* Struct Declarations (1) */

// Superblock structure
typedef struct superblock {
    uint32_t magic_number; // Magic number identifying the file system
    offset free_memory;    // Offset to the free memory
    offset root_directory; // Offset to the root directory
    size_t size;           // Total size of the file system
} superblock_t;

// Inode structure (common fields for both files and directories)
typedef struct inode {
    char name[NAME_MAX_LEN + 1];  // Name of the file or directory
    struct timespec accessed_time; // Last access time
    struct timespec modified_time; // Last modification time
    uint8_t type;                // Type: 1 for file, 2 for directory
    union {
        inode_file_t file;         // File-specific inode fields
        inode_directory_t directory; // Directory-specific inode fields
    } value;
} inode_t;

// File-specific inode fields
typedef struct inode_file {
    size_t size;      // Size of the file
    offset first_block; // Offset to the first block of the file
} inode_file_t;

// Directory-specific inode fields
typedef struct inode_directory {
    size_t num_children; // Number of children in the directory
    offset children;     // Offset to the children list
} inode_directory_t;

// Memory block structure
typedef struct data_block {
    size_t total_size; // Total size of the memory block
    size_t user_size;  // Size of the memory block for user data
    offset next;       // Offset to the next memory block
} data_block_t;

// File block structure
typedef struct file_block {
    size_t size;      // Size of the file block
    size_t allocated; // Allocated size of the file block
    offset next;      // Offset to the next file block
    offset data;      // Offset to the data of the file block
} file_block_t;

/* END Struct declarations (1) */

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
