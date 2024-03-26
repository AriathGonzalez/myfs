#ifndef __MY_FUSE_IMPL__
#define __MY_FUSE_IMPL__

#include <stdint.h>
#include <time.h>

// Constants and type definitions
#define MAGIC_NUMBER ((uint32_t)0xADDBEEF)
#define NAME_MAX_LEN ((size_t)255)
#define BLOCK_SIZE ((size_t)1024)

typedef size_t fs_offset;  
typedef unsigned int u_int;

/* Struct Declarations (1) */

// Memory block structure
typedef struct data_block {
    size_t remaining; // Remaining space of the data block
    fs_offset next;       // Offset to the next memory block
} data_block_t;

typedef struct list {
  fs_offset head;
} list_t;

// Superblock structure
typedef struct superblock {
    uint32_t magic_number; // Magic number identifying the file system
    fs_offset free_memory;    // Offset to the free memory
    fs_offset root_directory; // Offset to the root directory
    size_t size;           // Total size of the file system
} superblock_t;

// (3) File-specific inode fields
typedef struct inode_file {
    size_t size;      // Size of the file
    fs_offset first_block; // Offset to the first block of the file
} inode_file_t;

// (3) Directory-specific inode fields
typedef struct inode_directory {
    size_t num_children; // Number of children in the directory
    fs_offset children;     // Offset to the children list
} inode_directory_t;

// Inode structure (common fields for both files and directories)
typedef struct inode {
    char name[NAME_MAX_LEN + ((size_t) 1)];  // Name of the file or directory (+ 1 for null terminator)
    struct timespec time[2];    // [0] - last access time; [1] - last modification time
    uint8_t type;                // Type: 1 for file, 2 for directory
    union {
        inode_file_t file;         // File-specific inode fields
        inode_directory_t directory; // Directory-specific inode fields
    } value;
} inode_t;

// (3) File block structure
typedef struct file_block {
    size_t size;      // Size of the file block
    size_t allocated; // Allocated size of the file block
    fs_offset next;      // Offset to the next file block
    fs_offset data;      // Offset to the data of the file block
} file_block_t;

/* END Struct declarations (1) */

/* START memory allocation implementation */

/**
 * @brief (2) Allocate memory for a data block and add it to the list in ascending order.
 * 
 * This function allocates memory for a data block and inserts it into a linked list
 * of allocated memory regions, maintaining ascending order based on memory addresses.
 * 
 * @param fsptr Pointer to the start of the file system.
 * @param ll Pointer to the linked list of allocated memory regions.
 * @param new_block Pointer to the data block to be allocated.
 */
void add_to_free_memory(void *fsptr, list_t *ll, data_block_t *new_block);

/**
 * @brief (2) Extends the available block to accommodate additional size, if possible.
 * 
 * This function extends the available block to accommodate the specified size
 * if it has enough remaining space. If the available block does not have enough
 * space, it extends the original available block as much as possible and updates
 * the size accordingly.
 * 
 * @param fsptr Pointer to the start of the file system.
 * @param before_avail Pointer to the data block before the available block.
 * @param org_avail Pointer to the original available block.
 * @param avail_off Offset of the available block.
 * @param size Pointer to the size to be extended.
 */
void extend_avail_block(void *fsptr, data_block_t *before_avail,
                        data_block_t *org_avail, fs_offset avail_off,
                        size_t *size);

/**
 * @brief (2) Gets a memory allocation block of the specified size.
 * 
 * This function retrieves a memory allocation block of the specified size
 * from the free space managed by the provided list. It first attempts to
 * allocate space from a selected space indicated by selected_block. If the selected
 * space does not have enough space, or if no selected space is specified, it
 * searches for the largest available block in the list and allocates space from it.
 * 
 * @param fsptr Pointer to the start of the file system.
 * @param ll Pointer to the list of free blocks.
 * @param selected_block Pointer to the original selected space block, if any.
 * @param size Pointer to the size of the allocation to be obtained.
 * @return Pointer to the allocated memory block, or NULL if allocation fails.
 */
void *get_memory_block(void *fsptr, list_t *ll, data_block_t *selected_block,
                     size_t *size);    

/**
 * @brief (2) Allocates a memory block of the given size.
 *
 * If the size is zero, returns NULL. Otherwise, calls get_allocation to
 * allocate memory of the specified size.
 *
 * @param fsptr Pointer to the start of the file system.
 * @param selected_ptr Pointer to the preferred memory block. If NULL, defaults to fsptr.
 * @param size Pointer to the size of the memory block to allocate.
 * @return Pointer to the allocated memory block, or NULL if size is zero or memory allocation fails.
 */
void *malloc_impl(void *fsptr, void *selected_ptr, size_t *size);

/**
 * @brief (2) Reallocates memory block pointed to by orig_ptr to the specified size.
 *
 * If size is 0, frees the memory block pointed to by orig_ptr and returns NULL.
 * If orig_ptr is NULL, reallocates memory equivalent to a call to malloc(size) for size bytes.
 * If the new size is less than the original size and not enough to make a new memory block, 
 * the remaining memory is kept.
 * If the new size is less than the original size and enough to make a new memory block, 
 * a new memory block is created for the remaining memory and added to the free memory list.
 * If the new size is greater than the original size, a new memory block is allocated, 
 * contents of the original memory block are copied to the new memory block, 
 * and the original memory block is freed.
 *
 * @param fsptr Pointer to the start of the file system.
 * @param orig_ptr Pointer to the original memory block.
 * @param size Pointer to the new size of the memory block.
 * @return Pointer to the reallocated memory block, or NULL if size is 0 or memory allocation fails.
 */
void *realloc_impl(void *fsptr, void *orig_ptr, size_t *size);

/**
 * @brief (2) Frees the memory block pointed to by ptr and adds it back to the free memory list.
 *
 * If ptr is NULL, the function does nothing.
 *
 * @param fsptr Pointer to the start of the file system.
 * @param ptr Pointer to the memory block to be freed.
 */
void free_impl(void *fsptr, void *ptr);

/* END memory allocation implementation */

/* Start fuse helper methods */

/// @brief (1) Convert a pointer to an offset relative to the start of the file system
/// @param fsptr 
/// @param pointer 
/// @return 
fs_offset pointer_to_offset (void *fsptr, void *pointer);  

/// @brief (1) Calculate an offset relative to the start of the file system to a pointer
/// @param fsptr 
/// @param my_offset 
/// @return 
void *offset_to_pointer (void *fsptr, fs_offset my_offset);

/// @brief (1)
/// @param fsptr 
/// @return 
void *get_free_memory_pointer(void *fsptr);

/**
 * @brief (1) Updates the access and modification times of the specified inode.
 *
 * If node is NULL, the function does nothing.
 *
 * @param node Pointer to the inode whose times are to be updated.
 * @param set_mod Flag indicating whether to update modification time as well.
 *                If set_mod is non-zero, modification time will be updated.
 *                If set_mod is zero, only access time will be updated.
 */
void update_time(inode_t *node, int set_mod);

/**
 * @brief (1) Mounts the filesystem represented by the provided memory buffer.
 *
 * If the filesystem is being mounted for the first time, initializes the superblock
 * and sets up the root directory.
 *
 * @param fsptr Pointer to the start of the filesystem.
 * @param fssize Size of the filesystem.
 */
void mount_filesystem(void *fsptr, size_t fssize);

/**
 * @brief (4) Retrieves the last token (filename or directory name) from the given path.
 *
 * Finds the last occurrence of '/' in the path and extracts the token following it.
 * Allocates memory for the token and returns a copy of it.
 *
 * @param path The path from which to extract the last token.
 * @param token_len Pointer to store the length of the extracted token.
 * @return Pointer to the extracted token, or NULL if memory allocation fails.
 */
char *get_last_token(const char *path, unsigned long *token_len);

/**
 * @brief (4) Tokenizes a string based on a delimiter character and skips a specified number of tokens.
 *
 * Splits the input path string into tokens using the given delimiter character.
 * Allocates memory for the tokens array and returns it.
 *
 * @param token The delimiter character to use for tokenization.
 * @param path The input path string to tokenize.
 * @param skip_n_tokens The number of tokens to skip from the end of the tokenized array.
 * @return An array of token strings, or NULL if memory allocation fails.
 */
char **tokenize(const char token, const char *path, int skip_n_tokens);

/// @brief (4)
/// @param tokens 
void free_tokens(char **tokens);

/**
 * @brief (4) Retrieves the inode of a child node within a directory.
 *
 * Searches for the specified child node within the directory specified by dict.
 * Returns a pointer to the inode of the child node if found, or NULL if not found.
 * If the child node name is "..", returns a pointer to the parent directory inode.
 *
 * @param fsptr Pointer to the start of the filesystem.
 * @param dict Pointer to the directory inode structure.
 * @param child Name of the child node to search for.
 * @return Pointer to the inode of the child node if found, or NULL if not found.
 */
inode_t *get_node(void *fsptr, inode_directory_t *dict, const char *child);

/**
 * @brief (4) Resolves the path to retrieve the corresponding inode.
 *
 * Traverses the filesystem hierarchy based on the provided path to locate
 * and return the inode corresponding to the specified path.
 * 
 * @param fsptr Pointer to the start of the filesystem.
 * @param path The path to resolve.
 * @param skip_n_tokens Number of leading tokens to skip in the path.
 * @return Pointer to the inode corresponding to the resolved path, or NULL if not found.
 */
inode_t *resolve_path(void *fsptr, const char *path, int skip_n_tokens);

/**
 * @brief (6) Creates a new inode (file or directory) at the specified path.
 *
 * Creates a new inode (file or directory) at the specified path within the filesystem.
 * 
 * @param fsptr Pointer to the start of the filesystem.
 * @param path The path where the new inode will be created.
 * @param errnoptr Pointer to store error number in case of failure.
 * @param isfile Indicates whether the inode to be created is a file (1) or directory (0).
 * @return Pointer to the newly created inode on success, NULL on failure.
 */
inode_t *make_inode(void *fsptr, const char *path, int *errnoptr, int isfile);

/* END fuse helper methods */

/* START fuse functions declarations */

/**
 * @brief (4) Implements an emulation of the stat system call on the filesystem 
 *        of size fssize pointed to by fsptr.
 *
 * If path can be followed and describes a file or directory 
 * that exists and is accessible, the access information is 
 * put into stbuf.
 *
 * On success, 0 is returned. On failure, -1 is returned and 
 * the appropriate error code is put into *errnoptr.
 *
 * @param fsptr Pointer to the start of the filesystem.
 * @param fssize Size of the filesystem.
 * @param errnoptr Pointer to store the error code in case of failure.
 * @param uid User ID.
 * @param gid Group ID.
 * @param path Path to the file or directory.
 * @param stbuf Pointer to a struct stat to store access information.
 * @return 0 on success, -1 on failure.
 * 
 * man 2 stat documents all possible error codes and gives more detail
 * on what fields of stbuf need to be filled in. Essentially, only the
 * following fields need to be supported:
 * - st_uid: the value passed in argument
 * - st_gid: the value passed in argument
 * - st_mode: (as fixed values S_IFDIR | 0755 for directories,
 *             S_IFREG | 0755 for files)
 * - st_nlink: (as many as there are subdirectories (not files) for directories
 *              (including . and ..),
 *              1 for files)
 * - st_size: (supported only for files, where it is the real file size)
 * - st_atim
 * - st_mtim
 */
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr, uid_t uid,
                          gid_t gid, const char *path, struct stat *stbuf);

/**
 * @brief (5) Implements an emulation of the readdir system call on the filesystem 
 *        of size fssize pointed to by fsptr. 
 *
 * If path can be followed and describes a directory that exists and
 * is accessible, the names of the subdirectories and files 
 * contained in that directory are output into *namesptr. The . and ..
 * directories must not be included in that listing.
 *
 * If it needs to output file and subdirectory names, the function
 * starts by allocating (with calloc) an array of pointers to
 * characters of the right size (n entries for n names). Sets
 * *namesptr to that pointer. It then goes over all entries
 * in that array and allocates, for each of them an array of
 * characters of the right size (to hold the i-th name, together 
 * with the appropriate '\0' terminator). It puts the pointer
 * into that i-th array entry and fills the allocated array
 * of characters with the appropriate name. The calling function
 * will call free on each of the entries of *namesptr and 
 * on *namesptr.
 *
 * The function returns the number of names that have been 
 * put into namesptr. 
 *
 * If no name needs to be reported because the directory does
 * not contain any file or subdirectory besides . and .., 0 is 
 * returned and no allocation takes place.
 *
 * On failure, -1 is returned and the *errnoptr is set to 
 * the appropriate error code. 
 *
 * The error codes are documented in man 2 readdir.
 *
 * In the case memory allocation with malloc/calloc fails, failure is
 * indicated by returning -1 and setting *errnoptr to EINVAL.
 *
 * @param fsptr Pointer to the start of the filesystem
 * @param fssize Size of the filesystem
 * @param errnoptr Pointer to store error code in case of failure
 * @param path Path of the directory to read
 * @param namesptr Pointer to store the array of directory and file names
 * @return The number of names read on success, 0 if no entries, -1 on failure
 */
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr);

/**
 * @brief (6) Implements an emulation of the mknod system call for regular files
 *        on the filesystem of size fssize pointed to by fsptr.
 *
 * This function is called only for the creation of regular files.
 * If a file gets created, it is of size zero and has default
 * ownership and mode bits.
 * The call creates the file indicated by path.
 *
 * On success, 0 is returned.
 * On failure, -1 is returned and *errnoptr is set appropriately.
 * The error codes are documented in man 2 mknod.
 *
 * @param fsptr Pointer to the start of the filesystem.
 * @param fssize Size of the filesystem.
 * @param errnoptr Pointer to store the error code in case of failure.
 * @param path Path to the file to be created.
 * @return 0 on success, -1 on failure.
 */ 
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
