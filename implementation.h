#ifndef __MY_FUSE_IMPL__
#define __MY_FUSE_IMPL__

#include <stdint.h>
#include <time.h>

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
    char name[NAME_MAX_LEN + 1];  // Name of the file or directory (+ 1 for null terminator)
    struct timespec time[2];    // [0] - last access time; [1] - last modification time
    uint8_t type;                // Type: 1 for file, 2 for directory
    union {
        inode_file_t file;         // File-specific inode fields
        inode_directory_t directory; // Directory-specific inode fields
    } value;
} inode_t;

// (3) File-specific inode fields
typedef struct inode_file {
    size_t size;      // Size of the file
    offset first_block; // Offset to the first block of the file
} inode_file_t;

// (3) Directory-specific inode fields
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

// (3) File block structure
typedef struct file_block {
    size_t size;      // Size of the file block
    size_t allocated; // Allocated size of the file block
    offset next;      // Offset to the next file block
    offset data;      // Offset to the data of the file block
} file_block_t;

/* END Struct declarations (1) */

/* Start fuse helper methods */

/// @brief (1) Convert a pointer to an offset relative to the start of the file system
/// @param pointer 
/// @param fsptr 
/// @return 
static inline offset pointer_to_offset (void *pointer, void *fsptr);  

/// @brief (1) Calculate an offset relative to the start of the file system to a pointer
/// @param pointer 
/// @param my_offset 
/// @return 
static inline void *offset_to_pointer (void *pointer, offset my_offset);

/// @brief (1) Mount a file system given a pointer to its start and its size
/// @param fsptr 
/// @param fssize 
/// @return 
superblock_t *mount_filesystem (void *fsptr, size_t fssize);

/// @brief (2) Calculates the total size of free memory in the file system
/// @param sb 
/// @return 
size_t free_memory_size (superblock_t *sb);

/// @brief (2) Retrieves a memory block of the specified size from the free memory pool
/// @param sb 
/// @param size 
/// @return 
data_block_t *get_memory_block(superblock_t *sb, size_t size);

/// @brief (2) Adds a data block to the free memory pool
/// @param sb 
/// @param my_offset 
void add_to_free_memory (superblock_t *sb, offset my_offset);

/// @brief (2) Frees a data block and adds it to the free memory pool
/// @param sb 
/// @param my_offset 
void free_impl (superblock_t *sb, offset my_offset);

/// @brief (2) Allocates a memory block of the specified dsize from the free memory pool
/// @param sb 
/// @param size 
/// @return 
offset allocate_memory (superblock_t *sb, size_t size);

/// @brief (2) Reallocates a data block to the specified size
/// @param sb 
/// @param my_offset 
/// @param size 
/// @return 
offset reallocate_memory (superblock_t *sb, offset my_offset, size_t size);

/// @brief (2) Resolves a path to its corresponding inode in the file system
/// @param sb 
/// @param path 
/// @return 
inode_t *path_resolve (superblock_t *sb, const char *path);

/* END fuse helper methods */

/* START fuse functions declarations */

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param uid 
/// @param gid 
/// @param path 
/// @param stbuf 
/// @return 
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr, uid_t uid,
                          gid_t gid, const char *path, struct stat *stbuf);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @param namesptr 
/// @return 
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @return 
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @return 
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *path);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @return 
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @return 
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param from 
/// @param to 
/// @return 
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @param offset 
/// @return 
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @return 
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @param buf 
/// @param size 
/// @param offset 
/// @return 
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset);
/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @param buf 
/// @param size 
/// @param offset 
/// @return 
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size,
                        off_t offset);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param path 
/// @param ts 
/// @return 
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]);

/// @brief 
/// @param fsptr 
/// @param fssize 
/// @param errnoptr 
/// @param stbuf 
/// @return 
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs *stbuf);

// END of fuse function declarations

#endif
