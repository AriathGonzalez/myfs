/*

  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Copyright 2022-24

  University of Texas at El Paso, Department of Computer Science.

  Contributors: Christoph Lauter 
                ... and
                ...

  and based on 

  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "implementation.h"
#include <linux/time.h>


/* The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.

   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.

   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.

   CAUTION:

   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.

   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.

   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.

   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.

   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!

   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:

   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.

   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.

   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.

   * And of course: your code MUST NOT SEGFAULT!

   It is reasonable to proceed in the following order:

   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinitialized at mount
         time if you initialized it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 

   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.

   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).
          
   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.

   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.

   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with 

         touch foo

         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.

   (7)   Design and implement __myfs_mkdir_implem. Test as above.

   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:

         truncate -s 1024 foo

   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.

   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).

   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.

   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:

         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo

         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.

   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.

   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.

   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.

   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.

   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).

   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.

*/

/* Helper types and functions */

/* YOUR HELPER FUNCTIONS GO HERE */

static inline offset pointer_to_offset (void *pointer, void *fsptr) {   
      if (pointer < fsptr){
            return 0;   // Pointer is before the start of the file system
      }
      return (offset) (pointer - fsptr);  // Calculate offset
}

static inline void *offset_to_pointer (void *pointer, offset my_offset){
      if (my_offset == 0){
            return NULL;      // Offset is 0, return NULL pointer
      }
      return (void *) (pointer + my_offset);    // Calculate pointer
}

superblock_t *mount_filesystem (void *fsptr, size_t fssize){
      superblock_t *sb = (superblock_t *) fsptr;      // superblock pointer
      data_block_t *db; // data block pointer
      size_t s;   

      if (fssize < sizeof(superblock_t)){
            return NULL;      // File system size is too small to contain superblock
      }

      // Check if magic number is set, indicating an already mounted file system
      if (sb->magic_number != MAGIC_NUMBER){
            // Will be the size of file system - size of super block
            s = (fssize - (sizeof(superblock_t)));    // Calculate available size for data blocks

            // Initialize file system
            if (sb->magic_number != ((uint32_t) 0)){
                  // File the rest ofo the file system with 0s
                  memset((fsptr + sizeof(superblock_t)), 0, s);
            }
            sb->magic_number = MAGIC_NUMBER;
            sb->size = s;

            if (s == ((size_t) 0)){
                  sb->free_memory = ((offset) 0);     // No space available, set free memory to 0
            }
            else {
                  // Calculate pointer to first data block (will be the start of file system + size of superblock)
                  db = (data_block_t *) offset_to_pointer(fsptr, sizeof(superblock_t));   
                  db->total_size = s;     // Set total size of data block
                  db->next = (offset) 0;  // Set the next pointer of the data block to 0
                  // Set free memory offset relative to the start of the file system
                  sb->free_memory = pointer_to_offset(fsptr, db); // We just lost memory with data block
            }
            sb->root_directory = (offset) 0;    // Initialize root directory offset
      }
      return sb;  // Return mounted superblock
}

size_t free_memory_size (superblock_t *sb){
      size_t total_free_size;
      data_block_t *db;

      total_free_size = (size_t) 0; 

      /*
      Go through each data block in linked list, adding up their size to get
      total free size.
      Initialization: Starting from first data block in linked list.
      Condition:  while not null
      Update: Point db to the next data block in linked list using offset
      */
      for (db = (data_block_t *) offset_to_pointer(sb, sb->free_memory);
           db != NULL; 
           db = (data_block_t *) (offset_to_pointer(sb, db->next))){
                  total_free_size += db->total_size;
      }
      
      return total_free_size;
}

data_block_t *get_memory_block(superblock_t *sb, size_t size){
      data_block_t *curr, *prev, *next;

      for (curr = (data_block_t *) offset_to_pointer(sb, sb->free_memory), prev = NULL;
           curr != NULL && curr->next != (offset) 0;
           prev = curr, curr = (data_block_t *) offset_to_pointer(sb, curr->next)){
            if (curr->total_size > size){
                  break;      // Found a block that can accomodate size
            }
      }
      
      // No suitable block found, return NULL
      if (curr->total_size < size){
            return NULL;
      }

      // If allocated space is larger than requested size,
      // split the block and adjust pointers
      if (curr->total_size - size > (size_t) 0){
            next = (data_block_t *) offset_to_pointer(curr, size);
            next->total_size = curr->total_size - size;
            next->next = curr->next;      // Inherit the rest of the free memory
      } 
      else {
            next = (data_block_t *) offset_to_pointer(sb, curr->next);
      }

      // If the current block is the first block in the free memory pool,
      // update the superblock's free memory pointer
      if (curr == (data_block_t *) offset_to_pointer(sb, sb->free_memory)){
            sb->free_memory = pointer_to_offset(next, sb);
      }
      else {
            prev->next = pointer_to_offset(next, sb);
      }
      curr->total_size = size;
      curr->next = (offset) 0;

      return curr;
}

void add_to_free_memory (superblock_t *sb, offset my_offset){
      data_block_t *db, *curr, *prev;
      db = (data_block_t *) offset_to_pointer(sb, my_offset);     // Get pointer to data block

      // Iterate through linked list of data blocks
      for (curr = (data_block_t *) offset_to_pointer(sb, sb->free_memory), prev = NULL;
           curr != NULL;
           prev = curr, curr = (data_block_t *) offset_to_pointer(sb, curr->next)){
            
            // Check if current data block should be placed before the new block
            if ((void *) db < (void *) curr){
                  break;
            }
      }
      
      // Place the new block in between prev and curr block
      if (curr != NULL){
            db->next = pointer_to_offset(curr, sb);
      }
      else {
            db->next = (offset) 0;
      }

      // Update the next pointer of the previous block or superblock
      if (prev == NULL){
            sb->free_memory = my_offset;
      }
      else {
            prev->next = my_offset;
      }

      // Merge adjacent data blocks if they are contiguous (coalesce)
      if (curr != NULL && ((void *) ((void *) db + db->total_size)) == ((void *) curr)) {
            db->total_size += curr->total_size;
            db->next = curr->next;
      }

      if (prev != NULL && ((void *) ((void *) prev + prev->total_size)) == ((void *) db)) {
            prev->total_size += db->total_size;
            prev->next = db->next;
      }
}

void free_impl (superblock_t *sb, offset my_offset){
      void *pointer;    // Pointer to adjust the offset of the data block

      // Adjust the offset to account for the metadata (data_block_t struct)
      // Decrementing a data block
      pointer = (((void *) offset_to_pointer(sb, my_offset)) - ((size_t) sizeof(data_block_t))); 
      
      // Convert the adjusted pointer back to an offset
      offset new_offset = pointer_to_offset(pointer, sb);

      // Add the data block to the free memory pool
      add_to_free_memory(sb, new_offset);
}

offset allocate_memory (superblock_t *sb, size_t size){
      size_t s;   // Adjusted size of the data block
      void *pointer;    // Pointer to the allocated memory block

      // If size is 0, no allocation
      if (size == ((size_t) 0)){
            return (offset) 0;
      }

      // Calculate the adjusted size of the data block considering metadata
      s = (size + (size_t) sizeof(data_block_t));

      // Check for overflow
      if (s < size){
            return (offset) 0;
      }

      // Get a data block from free memory pool
      pointer = ((void *) get_memory_block(sb, s));

      // If data block allocation successful, return its offset
      if (pointer != NULL){
            return pointer_to_offset((pointer + (size_t) sizeof(data_block_t)), sb);
      }

      // Memory block allocations fails
      return (offset) 0;
}

offset reallocate_memory (superblock_t *sb, offset my_offset, size_t size){
      data_block_t *old_db;   // Pointer to the metadata to old data block
      offset new_offset;      // Offset of the new data block
      void *old_pointer, *new_db;   // Pointers to old and new data blocks
      size_t s;

      // Check for NULL superblock
      if (sb == NULL){
            return (offset) 0;
      }
      // Check for NULL offset
      if (my_offset == (offset) 0){
            return (offset) 0;
      }
      // If new size is 0, free the data block and return 0
      if (size == (size_t) 0){
            free_impl(sb, my_offset);
            return (offset) 0;
      }

      // Allocate memory for new data block w/ specified size
      new_offset = allocate_memory(sb, size);
      if (new_offset == (offset) 0){
            return (offset) 0;      // Allocation failed
      }

      // Get pointer to old data block and its metadata
      old_pointer = offset_to_pointer(sb, my_offset);
      old_db = (data_block_t *) (old_pointer - (size_t) sizeof(data_block_t));

      // Determine the size to copy from old to new data block
      s = old_db->total_size;
      if (size < s){
            s = size;
      }

      new_db = offset_to_pointer(sb, new_offset);     // Get pointer to new data block
      memcpy(new_db, old_pointer, s);     // Copy from old to new data block
      free_impl(sb, my_offset);     // Free old data block

      // Return offset of reallocated memory block
      return new_offset;
}

size_t get_max_free_size (superblock_t *sb){
      data_block_t *db;
      size_t max_free_size;

      max_free_size = (size_t) 0;

      for (db = (data_block_t *) offset_to_pointer(sb, sb->free_memory);
           db != NULL;
           db = (data_block_t *) (offset_to_pointer(sb, db->next))){

            if (db->total_size > max_free_size){
                  max_free_size = db->total_size;
            }
      }
      return max_free_size;
}

inode_t *path_resolve (superblock_t *sb, const char *path){
      char *index, *path_copy, *name, file_name[NAME_MAX_LEN];
      inode_t *node, *child;
      size_t size, i;

      // If root directory is not initialized, create it
      if (sb->root_directory == (offset) 0){
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            // Allocate memory for the root directory inode
            sb->root_directory = allocate_memory(sb, ((size_t) sizeof(inode_t)));

            // Obtain pointer to root inode
            inode_t *root = (inode_t *) offset_to_pointer(sb, sb->root_directory);

            // Initialize root inode properties
            root->name[0] = '/';
            root->name[1] = '\0';
            root->type = 2;
            root->time[0] = ts;     // Last access time
            root->time[1] = ts;     // Last modified time
            root->value.directory.num_children = (size_t) 0;
            root->value.directory.children = (offset) 0;
      }

      // Get pointer to node
      node = (inode_t *) offset_to_pointer(sb, sb->root_directory);

      // Check if path is root
      if (strcmp("/\0", path) == 0){
            return node;
      }

      // Copy path to a new buffer
      path_copy = (char *) malloc((strlen(path) + 1) * sizeof(char));
      if (path_copy == NULL){
            return NULL;
      }
      strcpy(path_copy, path);

      // Extract individual directory names from the path
      name = path_copy + 1;   // pointer to 1 char after path (skipping '/')
      index = strchr(name, '/');    // Get 1st occurrence of '/' in string pointed to by name

      // Traverse the directory structure to resolve the path
      while (strlen(name) != 0){
            child = NULL;
            if (index != NULL){
                  // Size is the number of characters in current directory name
                  size = (size_t)  (((void *) index) - ((void *) name));
            }
            else {
                  size = (size_t) strlen(name);
            }
            strncpy(file_name, name, size);     // Extract current directory name    
            file_name[size] = '\0';

            // Iterate through children of the current node to find the matching child
            for (i = 0; i < node->value.directory.num_children; i++){
                  child = (inode_t *) offset_to_pointer(sb, (node->value.directory.children + i * ((size_t) sizeof(inode_t))));
                  if (strcmp(child->name, file_name) == 0){
                        node = child;
                        break;
                  }
            }

            memset(file_name, 0, size);   // Clear buffer

            // If child not found or node is not in directory, return NULL
            if (node != child){
                  free(path_copy);
                  return NULL;
            }
            if (index == NULL){
                  break;
            }
            name = index + 1; // Start at beginning of next directory
            index = strchr(name, '/');    // another '/'
      }
      free(path_copy);
      return node;
}

/* End of helper functions */

int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr,
                          uid_t uid, gid_t gid,
                          const char *path, struct stat *stbuf) {
  superblock_t *sb;
  inode_t *node;
  char *file_name;
  int subdirectory_count;
  size_t i;

  sb = mount_filesystem(fsptr, fssize);

  // Check if mount successful
  if (sb == NULL){
      *errnoptr = EFAULT;
      return -1;
  }

  // Resolve path to get node
  node = path_resolve(sb, path);

  // Check if path resolution successful
  if (node == NULL){
      *errnoptr = ENOENT;
      return -1;
  }
  
  // Extract file name from the path
  file_name = strchr(path, '/') + 1;

  // Check if file name length exceeds max allowed length
  if (strlen(file_name) >= NAME_MAX_LEN){
      *errnoptr = ENAMETOOLONG;
      return -1;
  }

  // Initialize stbuf w/ 0s
  memset(stbuf, 0, sizeof(struct stat));

  // Fill in stbuf fields w/ appropriate values
  stbuf->st_uid = uid;
  stbuf->st_gid = gid;
  stbuf->st_atime = (time_t) node->time[0].tv_sec;
  stbuf->st_mtime = (time_t) node->time[1].tv_sec;

  // Determine the type of node inode (directory or file) and set st_mode and st_nlink accordingly
  if (node->type == 2){ // Directory
      stbuf->st_mode = __S_IFDIR | 0755;
      inode_t *children = (inode_t *) offset_to_pointer(sb, node->value.directory.children);

      subdirectory_count = 0;

      // Count the number of subdirectories (excluding files)
      for (i = (size_t) 0; i < node->value.directory.num_children; i++){
            if (children[i].type == 2){
                  subdirectory_count++;
            }
      }
      stbuf->st_nlink = subdirectory_count;
  }
  // File
  else if (node->type == 1){
      stbuf->st_mode = __S_IFREG | 0755;
      stbuf->st_size = node->value.file.size;
      stbuf->st_nlink = 1;
  }
  return 0;
}

/* Implements an emulation of the readdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   If path can be followed and describes a directory that exists and
   is accessable, the names of the subdirectories and files 
   contained in that directory are output into *namesptr. The . and ..
   directories must not be included in that listing.

   If it needs to output file and subdirectory names, the function
   starts by allocating (with calloc) an array of pointers to
   characters of the right size (n entries for n names). Sets
   *namesptr to that pointer. It then goes over all entries
   in that array and allocates, for each of them an array of
   characters of the right size (to hold the i-th name, together 
   with the appropriate '\0' terminator). It puts the pointer
   into that i-th array entry and fills the allocated array
   of characters with the appropriate name. The calling function
   will call free on each of the entries of *namesptr and 
   on *namesptr.

   The function returns the number of names that have been 
   put into namesptr. 

   If no name needs to be reported because the directory does
   not contain any file or subdirectory besides . and .., 0 is 
   returned and no allocation takes place.

   On failure, -1 is returned and the *errnoptr is set to 
   the appropriate error code. 

   The error codes are documented in man 2 readdir.

   In the case memory allocation with malloc/calloc fails, failure is
   indicated by returning -1 and setting *errnoptr to EINVAL.

*/
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the mknod system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the creation of regular files.

   If a file gets created, it is of size zero and has default
   ownership and mode bits.

   The call creates the file indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mknod.

*/
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the unlink system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the deletion of regular files.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 unlink.

*/
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the rmdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call deletes the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The function call must fail when the directory indicated by path is
   not empty (if there are files or subdirectories other than . and ..).

   The error codes are documented in man 2 rmdir.

*/
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call creates the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mkdir.

*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the rename system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call moves the file or directory indicated by from to to.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   Caution: the function does more than what is hinted to by its name.
   In cases the from and to paths differ, the file is moved out of 
   the from path and added to the to path.

   The error codes are documented in man 2 rename.

*/
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the truncate system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call changes the size of the file indicated by path to offset
   bytes.

   When the file becomes smaller due to the call, the extending bytes are
   removed. When it becomes larger, zeros are appended.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 truncate.

*/
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the open system call on the filesystem 
   of size fssize pointed to by fsptr, without actually performing the opening
   of the file (no file descriptor is returned).

   The call just checks if the file (or directory) indicated by path
   can be accessed, i.e. if the path can be followed to an existing
   object for which the access rights are granted.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The two only interesting error codes are 

   * EFAULT: the filesystem is in a bad state, we can't do anything

   * ENOENT: the file that we are supposed to open doesn't exist (or a
             subpath).

   It is possible to restrict ourselves to only these two error
   conditions. It is also possible to implement more detailed error
   condition answers.

   The error codes are documented in man 2 open.

*/
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the read system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes from the file indicated by 
   path into the buffer, starting to read at offset. See the man page
   for read for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes read into the buffer is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 read.

*/
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the write system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes to the file indicated by 
   path into the buffer, starting to write at offset. See the man page
   for write for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes written into the file is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 write.

*/
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the utimensat system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call changes the access and modification times of the file
   or directory indicated by path to the values in ts.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 utimensat.

*/
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the statfs system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call gets information of the filesystem usage and puts in 
   into stbuf.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 statfs.

   Essentially, only the following fields of struct statvfs need to be
   supported:

   f_bsize   fill with what you call a block (typically 1024 bytes)
   f_blocks  fill with the total number of blocks in the filesystem
   f_bfree   fill with the free number of blocks in the filesystem
   f_bavail  fill with same value as f_bfree
   f_namemax fill with your maximum file/directory name, if your
             filesystem has such a maximum

*/
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs* stbuf) {
  /* STUB */
  return -1;
}
