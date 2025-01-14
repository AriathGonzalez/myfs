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

/* (2) START memory allocation implementation */

void add_to_free_memory(void *fsptr, list_t *ll, data_block_t *new_block) {
    data_block_t *current_block;
    fs_offset current_offset = ll->head;
    fs_offset new_block_offset = pointer_to_offset(fsptr, new_block);

    // Check if the new allocation comes before the first space in the linked list
    if (current_offset > new_block_offset) {
        // Update the head of the linked list
        ll->head = new_block_offset;

        // Check if we can merge the new allocation with the first space in the linked list
        if ((new_block_offset + sizeof(size_t) + new_block->remaining) == current_offset) {
            // Merge the spaces
            current_block = offset_to_pointer(fsptr, current_offset);
            new_block->remaining += sizeof(size_t) + current_block->remaining;
            new_block->next = current_block->next;
        } else {
            // Couldn't merge, so just add it as the first space and update pointers
            new_block->next = current_offset;
        }
    } else {
        // Find the position to insert the new allocation
        current_block = offset_to_pointer(fsptr, current_offset);
        
        // Get the last block that is in lower memory than the new allocation
        while ((current_block->next != 0) && (current_block->next < new_block_offset)) {
            current_block = offset_to_pointer(fsptr, current_block->next);
        }
        
        current_offset = pointer_to_offset(fsptr, current_block);

        // Check if we can merge the new allocation with the next space
        fs_offset next_block_offset = current_block->next;
        if (next_block_offset != 0) {
            if ((new_block_offset + sizeof(size_t) + new_block->remaining) == next_block_offset) {
                data_block_t *next_block = offset_to_pointer(fsptr, next_block_offset);
                new_block->remaining += sizeof(size_t) + next_block->remaining;
                new_block->next = next_block->next;
            } else {
                new_block->next = next_block_offset;
            }
        } else {
            new_block->next = 0;
        }

        // Check if we can merge the new allocation with the previous space
        if ((current_offset + sizeof(size_t) + current_block->remaining) == new_block_offset) {
            current_block->remaining += sizeof(size_t) + new_block->remaining;
            current_block->next = new_block->next;
        } else {
            current_block->next = new_block_offset;
        }
    }
}

void extend_avail_block(void *fsptr, data_block_t *before_avail,
                        data_block_t *org_avail, fs_offset avail_off,
                        size_t *size) {
    data_block_t *avail = offset_to_pointer(fsptr, avail_off);
    data_block_t *temp;

    // Check if all size bytes can be accommodated in the available block
    if (avail->remaining >= *size) {
        // Check if an AllocateFrom object can be created with the remaining space
        if (avail->remaining > *size + sizeof(data_block_t)) {
            // Create a new AllocateFrom object
            temp = ((void *)avail) + *size;
            temp->remaining = avail->remaining - *size;
            temp->next = avail->next;

            // Update original available block with final total size
            org_avail->remaining += *size;

            // Update pointers to add temp into the list of free blocks
            before_avail->next = avail_off + *size;
        }
        // Not enough space to create a new AllocateFrom object
        else {
            // Add the entire remaining space of the available block to the original one
            org_avail->remaining += avail->remaining;

            // Update pointers to skip the available block
            before_avail->next = avail->next;
        }
        // Reset size as all space is allocated from the available block
        *size = ((fs_offset)0);
    }
    // Not enough space in the available block, allocate as much as possible
    else {
        // Add the entire remaining space of the available block to the original one
        org_avail->remaining += avail->remaining;

        // Update pointers to skip the available block
        before_avail->next = avail->next;

        // Update size to reflect the amount of space allocated from the available block
        *size -= avail->remaining;
    }
}

void *get_memory_block(void *fsptr, list_t *ll, data_block_t *selected_block,
                     size_t *size) {
    fs_offset selected_block_offset = ((fs_offset)0);  // Offset for the selected space
    fs_offset before_current_offset;  // Offset before the current space
    data_block_t *before_current_block;  // Pointer to the block before the current space
    fs_offset current_offset;  // Offset of the current space
    data_block_t *current_block;  // Pointer to the current space
    data_block_t *before_largest_block = NULL;  // Pointer to the block before the largest block
    fs_offset largest_offset;  // Offset of the largest block
    data_block_t *largest_block;  // Pointer to the largest block
    size_t largest_size;  // Size of the largest block
    data_block_t *allocated_block = NULL;  // Pointer to the allocated memory block

    before_current_offset = ll->head;

    // If no space is available, return NULL
    if (!before_current_offset) {
        return NULL;
    }

    // Ensure the minimum size is sizeof(data_block_t)
    if (*size < sizeof(data_block_t)) {
        *size = sizeof(data_block_t);
    }

    // Check if a selected space is specified
    if (((void *)selected_block) != fsptr)  {
        // Calculate the offset for the selected space
        selected_block_offset = pointer_to_offset(fsptr, selected_block) + sizeof(size_t) + selected_block->remaining;
        // If the selected space is at the head of the list, extend it
        if (selected_block_offset == before_current_offset) {
            extend_avail_block(fsptr, ((void *)ll) - sizeof(size_t), selected_block, selected_block_offset, size);
            if (*size == ((size_t)0)) {
                return NULL;
            }
        }
    }

    // Initialize the largest block to the first available block
    before_current_offset = ll->head;
    before_current_block = offset_to_pointer(fsptr, before_current_offset);
    largest_offset = before_current_offset;
    largest_block = before_current_block;
    largest_size = before_current_block->remaining;
    current_offset = before_current_block->next;

    // Iterate through the list to find the largest available block
    while (current_offset != ((fs_offset)0)) {
        current_block = offset_to_pointer(fsptr, current_offset);
        if ((selected_block_offset == current_offset) || (current_block->remaining > largest_size)) {
            if (selected_block_offset == current_offset) {
                extend_avail_block(fsptr, before_current_block, selected_block, selected_block_offset, size);
                if (*size == ((fs_offset)0)) {
                    break;
                }
            } else {
                before_largest_block = before_current_block;
                largest_offset = current_offset;
                largest_block = current_block;
                largest_size = current_block->remaining;
            }
        }
        before_current_offset = current_offset;
        before_current_block = current_block;
        current_offset = current_block->next;
    }

    // Allocate memory from the largest block
    if ((*size != ((fs_offset)0)) && (largest_block != NULL)) {
        allocated_block = largest_block;
        if (largest_block->remaining >= *size) {
            if (largest_block->remaining > *size + sizeof(data_block_t)) {
                data_block_t *temp_block = ((void *)largest_block) + sizeof(size_t) + *size;
                temp_block->remaining = largest_block->remaining - *size - sizeof(size_t);
                temp_block->next = largest_block->next;
                if (before_largest_block == NULL) {
                    ll->head = largest_offset + *size + sizeof(size_t);
                } else {
                    before_largest_block->next = largest_offset + *size + sizeof(size_t);
                }
                allocated_block->remaining = *size;
            } else {
                if (before_largest_block != NULL) {
                    before_largest_block->next = largest_block->next;
                } else {
                    ll->head = ((fs_offset)0);  // The system is out of memory
                }
            }
            *size = ((fs_offset)0);
        } else {
            if (before_largest_block == NULL) {
                // The system has used all available memory and still not enough
                return NULL;
            } else {
                before_largest_block->next = largest_block->next;
                *size -= largest_block->remaining;
            }
        }
    }

    // Return the allocated memory block
    if (allocated_block == NULL) {
        return NULL;
    }
    return ((void *)allocated_block) + sizeof(size_t);  // Adjust pointer to skip the size_t header
}

void *malloc_impl(void *fsptr, void *selected_ptr, size_t *size) {
    // If size is zero, return NULL
    if (*size == ((size_t)0)) {
        return NULL;
    }

    // If selected pointer is NULL, set it to the beginning of the file system
    if (selected_ptr == NULL) {
        selected_ptr = fsptr + sizeof(size_t);
    }

    // Call get_memory_block to allocate memory block
    return get_memory_block(fsptr, get_free_memory_pointer(fsptr),
                            selected_ptr - sizeof(size_t), size);
}

void *realloc_impl(void *fsptr, void *orig_ptr, size_t *size) {
    // If size is 0, free the original pointer and return NULL
    if (*size == ((size_t)0)) {
        free_impl(fsptr, orig_ptr);
        return NULL;
    }

    list_t *ll = get_free_memory_pointer(fsptr);

    // If orig_ptr is NULL, allocate memory equivalent to a call to malloc(size)
    if (orig_ptr == fsptr) {
        // Allocate memory with malloc for size bytes
        return get_memory_block(fsptr, ll, fsptr, size);
    }

    // Calculate the data block pointer
    data_block_t *alloc = (data_block_t *)((void *)orig_ptr - sizeof(size_t));
    data_block_t *temp;
    void *new_ptr = NULL;

    // If the new size is less than the original size but not enough to create a new memory block
    if ((alloc->remaining >= *size) && (alloc->remaining < (*size + sizeof(data_block_t)))) {
        // Keep the original memory block
        new_ptr = orig_ptr;
    }
    // If the new size is less than the original size and enough to create a new memory block
    else if (alloc->remaining > *size) {
        // Create a new memory block for the remaining memory
        temp = (data_block_t *)(orig_ptr + *size);
        temp->remaining = alloc->remaining - *size - sizeof(size_t);
        temp->next = 0;  // Offset of zero
        add_to_free_memory(fsptr, ll, temp);
        // Update the remaining space
        alloc->remaining = *size;
        // Keep the original memory block
        new_ptr = orig_ptr;
    }
    // If the new size is greater than the original size
    else {
        // Allocate a new memory block
        new_ptr = get_memory_block(fsptr, ll, fsptr, size);
        // Check if memory allocation failed
        if (*size == 0) {
            return NULL;
        }
        // Copy contents of the original memory block to the new memory block
        memcpy(new_ptr, orig_ptr, alloc->remaining);
        // Free the original memory block
        add_to_free_memory(fsptr, ll, alloc);
    }

    return new_ptr;
}

void free_impl(void *fsptr, void *ptr) {
    // If ptr is NULL, do nothing
    if (ptr == NULL) {
        return;
    }

    // Adjust ptr by size_t to start on the size of pointer
    add_to_free_memory(fsptr, get_free_memory_pointer(fsptr), ptr - sizeof(size_t));
}

/* (2) END memory allocation implementation*/

/* YOUR HELPER FUNCTIONS GO HERE */

fs_offset pointer_to_offset (void *fsptr, void *pointer) {   
      if (fsptr > pointer){
            return 0;   // Pointer is before the start of the file system
      }
      return (fs_offset) (pointer - fsptr);  // Calculate offset
}

void *offset_to_pointer (void *fsptr, fs_offset my_offset){
      void *pointer = fsptr + my_offset;

      // Check for overflow
      if (pointer < fsptr){
            return NULL;      
      }
      return pointer;
}

void *get_free_memory_pointer(void *fsptr) {
  return &((superblock_t *)fsptr)->free_memory;
}

void update_time(inode_t *node, int set_mod) {
    // If node is NULL, do nothing
    if (node == NULL) {
        return;
    }

    // Get current time
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        // Update access time
        node->time[0] = ts;

        // Update modification time if set_mod is non-zero
        if (set_mod) {
            node->time[1] = ts;
        }
    }
}

void mount_filesystem(void *fsptr, size_t fssize) {
    superblock_t *sb = (superblock_t *)fsptr;

    // If this is the first mount, initialize the superblock
    if (sb->magic_number != MAGIC_NUMBER) {
        // Set general stats
        sb->magic_number = MAGIC_NUMBER;
        sb->size = fssize;

        // Save space for the root directory
        sb->root_directory = sizeof(superblock_t); // Store only the offset
        inode_t *root = offset_to_pointer(fsptr, sb->root_directory);

        // Set up the root directory
        memset(root->name, '\0', NAME_MAX_LEN + ((size_t)1)); // Fill name with null characters
        memcpy(root->name, "/", strlen("/")); // Copy name "/" into node->name
        update_time(root, 1); // Update access and modification times
        root->type = 2; // Set node type to directory
        inode_directory_t *parent_directory = &root->value.directory;
        parent_directory->num_children = ((size_t)1); // Set number of children (including "..")

        // Set up root's children
        size_t *children_size = offset_to_pointer(fsptr, sb->root_directory + sizeof(inode_t));
        *children_size = 4 * sizeof(fs_offset); // Set size of children array header
        parent_directory->children = pointer_to_offset(fsptr, (void *)children_size + sizeof(size_t));
        fs_offset *ptr = offset_to_pointer(fsptr, parent_directory->children);
        *ptr = 0; // Set parent of root to null

        // Set the pointer to the first free block
        sb->free_memory = parent_directory->children + *children_size;
        list_t *ll = get_free_memory_pointer(fsptr);
        data_block_t *fb = offset_to_pointer(fsptr, ll->head);

        // Initialize remaining free space
        fb->remaining = fssize - sb->free_memory - sizeof(size_t);
        memset((void *)fb + sizeof(size_t), 0, fb->remaining); // Fill free space with zeros
    }
}

char *get_last_token(const char *path, unsigned long *token_len) {
    unsigned long path_len = strlen(path);
    unsigned long last_slash_index;

    // Find the last '/' in the path
    for (last_slash_index = path_len - 1; last_slash_index > 0; last_slash_index--) {
        if (path[last_slash_index] == '/') {
            break;
        }
    }

    // Adjust index to start after the last '/'
    last_slash_index++;

    // Calculate the length of the last token
    *token_len = path_len - last_slash_index;

    // Allocate memory for the last token
    void *ptr = malloc((*token_len + 1) * sizeof(char));

    // Check that malloc was successful
    if (ptr == NULL){
      return NULL;
    }

    char *last_token = (char *)ptr;
    // Copy the last token from the path
    // strncpy(last_token, &path[last_slash_index], *token_len);
    strcpy(last_token, &path[last_slash_index]);
    last_token[*token_len] = '\0';  // Add null terminator

    return last_token; 
}

char **tokenize(const char token, const char *path, int skip_n_tokens) {
    int num_tokens = 0;

    // Count the number of tokens in the path
    for (const char *c = path; *c != '\0'; c++) {
        if (*c == token) {
            num_tokens++;
        }
    }

    // Adjust the number of tokens to skip
    num_tokens -= skip_n_tokens;

    // Allocate memory for the tokens array
    char **tokens = (char **)malloc(((size_t)(num_tokens + 1)) * sizeof(char *));
    if (tokens == NULL) {
        return NULL; // Memory allocation failed
    }

    const char *start = &path[1]; // Skip the first character which is '/'
    const char *end = start;
    char *token_ptr;

    // Populate the tokens array
    for (int i = 0; i < num_tokens; i++) {
        while ((*end != token) && (*end != '\0')) {
            end++;
        }

        // Allocate memory for the token string
        token_ptr = (char *)malloc(((size_t)(end - start) + ((u_int)1)) * sizeof(char));
        if (token_ptr == NULL) {
            // Memory allocation failed, free previously allocated memory and return NULL
            for (int j = 0; j < i; j++) {
                free(tokens[j]);
            }
            free(tokens);
            return NULL;
        }

        // Copy the token string
        memcpy(token_ptr, start, ((size_t)(end - start)));
        token_ptr[end - start] = '\0'; // Null-terminate the token string
        tokens[i] = token_ptr;
        start = ++end;
    }

    // Null-terminate the tokens array
    tokens[num_tokens] = NULL;

    return tokens;
}

void free_tokens(char **tokens) {
  for (char **p = tokens; *p; p++) free(*p);
  free(tokens);
}

inode_t *get_node(void *fsptr, inode_directory_t *directory, const char *child) {
    size_t num_children = directory->num_children;
    fs_offset *children_offsets = offset_to_pointer(fsptr, directory->children);
    inode_t *node = NULL;

    // Check if the child node is the parent directory
    if (strcmp(child, "..") == 0) {
        // Return the inode of the parent directory
        return ((inode_t *)offset_to_pointer(fsptr, children_offsets[0]));
    }

    // Iterate through the child nodes to find the specified one
    for (size_t i = ((size_t)1); i < num_children; i++) {
        node = ((inode_t *)offset_to_pointer(fsptr, children_offsets[i]));
        if (strcmp(node->name, child) == 0) {
            // Child node found, return its inode
            return node;
        }
    }

    // Child node not found
    return NULL;
}

inode_t *resolve_path(void *fsptr, const char *path, int skip_n_tokens) {
    // Check if the path starts at the root directory
    if (*path != '/') {
        return NULL;
    }

    // Get the inode of the root directory
    inode_t *node = offset_to_pointer(fsptr, ((superblock_t *)fsptr)->root_directory);

    // If the path is just "/", return the root directory inode
    if (path[1] == '\0') {
        return node;
    }

    // Break the path into tokens
    char **tokens = tokenize('/', path, skip_n_tokens);

    // Traverse the path tokens
    for (char **token = tokens; *token; token++) {
        // Files cannot have children
        if (node->type == 1) {
            free_tokens(tokens);
            return NULL;
        }

        // If the token is ".", stay on the same directory
        if (strcmp(*token, ".") != 0) {
            node = get_node(fsptr, &node->value.directory, *token);
            // Check if the child node was successfully retrieved
            if (node == NULL) {
                free_tokens(tokens);
                return NULL;
            }
        }
    }

    free_tokens(tokens);

    return node;
}

inode_t *make_node(void *fsptr, const char *path, int *errnoptr, int is_file) {
    // Call path solver without the last node name because that is the file name
    // if valid path name is given
    inode_t *parent_node = resolve_path(fsptr, path, 1);

    // Check that the file parent exist
    if (parent_node == NULL) {
        *errnoptr = ENOENT;  // No such file or directory
        return NULL;
    }

    // Check that the node returned is a directory
    if (parent_node->type != 2) {
        *errnoptr = ENOTDIR;  // Not a directory
        return NULL;
    }

    // Get directory from node
    inode_directory_t *parent_directory = &parent_node->value.directory;

    // Get last token which has the filename
    unsigned long len;
    char *new_node_name = get_last_token(path, &len);

    // Check that the parent doesn't contain a node with the same name as the one
    // we are about to create
    if (get_node(fsptr, parent_directory, new_node_name) != NULL) {
        *errnoptr = EEXIST;  // File already exists
        return NULL;
    }

    if (len == 0) {
        *errnoptr = ENOENT;  // No such file or directory
        return NULL;
    }

    if (len > NAME_MAX_LEN) {
        *errnoptr = ENAMETOOLONG;  // File name too long
        return NULL;
    }

    // Access children block
    fs_offset *children = offset_to_pointer(fsptr, parent_directory->children);
    data_block_t *block = (((void *)children) - sizeof(size_t));

    // Make the node and put it in the directory child list
    // First check if the directory list has free places to add nodes to
    size_t max_children = (block->remaining) / sizeof(fs_offset);
    size_t ask_size;
    if (max_children == parent_directory->num_children) {
        ask_size = block->remaining * 2;
        // Make more space for another children
        void *new_children = realloc_impl(fsptr, children, &ask_size);

        // realloc_impl() always returns a new pointer if ask_size == 0, otherwise
        // we don't have enough space in memory
        if (ask_size != 0) {
            *errnoptr = ENOSPC;  // No space left on device
            return NULL;
        }

        // Update offset to access the children
        parent_directory->children = pointer_to_offset(fsptr, new_children);
        children = ((fs_offset *)new_children);
    }

    // Allocate memory for new node
    ask_size = sizeof(inode_t);
    inode_t *new_node = (inode_t *)malloc_impl(fsptr, NULL, &ask_size);
    if ((ask_size != 0) || (new_node == NULL)) {
        free_impl(fsptr, new_node);
        *errnoptr = ENOSPC;  // No space left on device
        return NULL;
    }

    // Initialize node attributes
    memset(new_node->name, '\0', NAME_MAX_LEN + 1);  // Fill name characters with '\0'
    memcpy(new_node->name, new_node_name, len);  // Copy given name into node->name
    update_time(new_node, 1);  // Update node timestamps

    // Add file node to directory children
    children[parent_directory->num_children] = pointer_to_offset(fsptr, new_node);
    parent_directory->num_children++;
    update_time(parent_node, 1);

    if (is_file) {
        // Make a node for the file with size of 0
        new_node->type = 1;
        inode_file_t *file = &new_node->value.file;
        file->size = 0;
        file->first_block = 0;
    } else {
        // Make a node for the directory
        new_node->type = 2;
        parent_directory = &new_node->value.directory;
        parent_directory->num_children = ((size_t) 1);  // Set initial number of children to 1 (for '..')

        // Allocate memory for children block
        ask_size = 4 * sizeof(fs_offset);  // Allocate space for 4 children initially
        fs_offset *ptr = ((fs_offset *)malloc_impl(fsptr, NULL, &ask_size));
        if ((ask_size != 0) || (ptr == NULL)) {
            free_impl(fsptr, ptr);
            *errnoptr = ENOSPC;  // No space left on device
            return NULL;
        }
        // Save the offset to get to the children
        parent_directory->children = pointer_to_offset(fsptr, ptr);
        // Set first child to point to its parent
        *ptr = pointer_to_offset(fsptr, parent_node);
    }

    return new_node;
}

void remove_data(void *fsptr, file_block_t *block, size_t size) {
  // Check if the block is NULL
  if (block == NULL) {
    return;
  }

  size_t idx = 0;

  // Find the position to cut the file and free the rest
  while (size != 0) {
    // If cutting within this block
    if (size > block->allocated) {
      size -= block->allocated;
      block = offset_to_pointer(fsptr, block->next);
    } else {
      idx = size;
      size = 0;
    }
  }

  // Free the data starting at idx
  if ((idx + sizeof(data_block_t)) < block->allocated) {
    size_t *header = (size_t *)&((char *)offset_to_pointer(fsptr, block->data))[idx];
    *header = block->allocated - idx - sizeof(size_t);
    // Free the memory
    free_impl(fsptr, ((void *)header) + sizeof(size_t));
  }

  // Free all data and blocks after this one
  block = offset_to_pointer(fsptr, block->next);
  file_block_t *next_block;

  while (block != fsptr) {
    // Free the data block
    free_impl(fsptr, offset_to_pointer(fsptr, block->data));

    // Get the next block before freeing the current block
    next_block = offset_to_pointer(fsptr, block->next);
    // Free the file block
    free_impl(fsptr, block);

    // Update the block pointer to the next one
    block = next_block;
  }
}

file_block_t *malloc_file_block(void *fsptr, int *errnoptr) {
    size_t block_size = sizeof(file_block_t);
    file_block_t *block = malloc_impl(fsptr, NULL, &block_size);
    if (!block) {
        *errnoptr = ENOSPC; // No space left on device
        return NULL;
    }
    memset(block, 0, sizeof(file_block_t));
    return block;
}

int append_data_block(void *fsptr, file_block_t *block, size_t size, int *errnoptr) {
    size_t block_space = block->size - block->allocated;
    size_t append_size = size < block_space ? size : block_space;
    void *data_block = &((char *)offset_to_pointer(fsptr, block->data))[block->allocated];
    memset(data_block, 0, append_size);
    block->allocated += append_size;
    if (append_size < size) {
        *errnoptr = ENOSPC;
        return 0;
    }
    return 1;
}

int extend_block_with_zeros(void *fsptr, file_block_t *block, size_t size) {
    size_t block_space = block->size - block->allocated;
    size_t append_size = size < block_space ? size : block_space;
    void *data_block = &((char *)offset_to_pointer(fsptr, block->data))[block->allocated];
    memset(data_block, 0, append_size);
    block->allocated += append_size;
    return append_size == size ? 1 : 0;
}

void free_file_blocks(void *fsptr, fs_offset first_block) {
    file_block_t *block = offset_to_pointer(fsptr, first_block);
    file_block_t *next_block;

    while (block != NULL) {
        next_block = offset_to_pointer(fsptr, block->next);
        free_impl(fsptr, block); // Free the current block
        block = next_block; // Move to the next block
    }
}

int add_data(void *fsptr, inode_file_t *file, size_t size, int *errnoptr) {
  file_block_t *block = offset_to_pointer(fsptr, file->first_block);

  // Variables for later use
  file_block_t *prev_block = NULL;
  //   size_t initial_file_size = file->size;

  // Handle case when file is empty: append first one manually
  if (((void *)block) == fsptr) {
    block = malloc_file_block(fsptr, errnoptr);
    if (!block)
      return -1;
    // Append first file_block
    file->first_block = pointer_to_offset(fsptr, block);
    // Add the first data block
    size_t block_size = size < BLOCK_SIZE ? size : BLOCK_SIZE;
    if (!append_data_block(fsptr, block, block_size, errnoptr))
      return -1;
    size -= block_size;
  }
  // Extend the last block by appending zeros
  else {
    while (block->next != 0) {
      size_t block_space = block->size - block->allocated;
      size_t append_size = size < block_space ? size : block_space;
      if (!extend_block_with_zeros(fsptr, block, append_size))
        return -1;
      size -= append_size;
      if (size == 0)
        return 0;
      block = offset_to_pointer(fsptr, block->next);
    }
  }

  // Append blocks until all data is added
  while (size > 0) {
    block = malloc_file_block(fsptr, errnoptr);
    if (!block)
      return -1;
    if (!append_data_block(fsptr, block, size, errnoptr)) {
      free_file_blocks(fsptr, file->first_block);
      return -1;
    }
    size -= block->allocated;
    if (prev_block)
      prev_block->next = pointer_to_offset(fsptr, block);
    prev_block = block;
  }

  return 0;
}

/* End of helper functions */

int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr, uid_t uid,
                          gid_t gid, const char *path, struct stat *stbuf) {
    // Mount the filesystem
    mount_filesystem(fsptr, fssize);

    // Resolve the path to get the corresponding inode
    inode_t *node = resolve_path(fsptr, path, 0);

    // Path could not be resolved
    if (node == NULL) {
        *errnoptr = ENOENT;
        return -1;
    }

    // Set UID and GID
    stbuf->st_uid = uid;
    stbuf->st_gid = gid;

    // Set file or directory attributes
    if (node->type == 1) {
        // File attributes
        stbuf->st_mode = __S_IFREG; // Regular file
        stbuf->st_nlink = ((nlink_t)1); // Number of hard links
        stbuf->st_size = (off_t) node->value.file.size; // Size of the file
        stbuf->st_atime = (__time_t) node->time[0].tv_sec; // Last access time
        stbuf->st_mtime = (__time_t) node->time[1].tv_sec; // Last modification time
    } else {
        // Directory attributes
        stbuf->st_mode = __S_IFDIR; // Directory
        inode_directory_t *directory = &node->value.directory;
        fs_offset *children = offset_to_pointer(fsptr, directory->children);
        stbuf->st_nlink = ((nlink_t)2); // Number of hard links (minimum 2 for directories)
        for (size_t i = 1; i < directory->num_children; i++) {
            inode_t *child_node = (inode_t *)offset_to_pointer(fsptr, children[i]);
            if (child_node->type == 2) {
                stbuf->st_nlink++;
            } 
        }
    }

    return 0;
}

int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr) {
    // Mount the filesystem
    mount_filesystem(fsptr, fssize);

    // Resolve the path to get the corresponding inode
    inode_t *node = resolve_path(fsptr, path, 0);

    // Path could not be resolved
    if (node == NULL) {
      *errnoptr = ENOENT;
      return -1;
    }

    // Check if the node is a directory
    if (node->type != 2) {
      *errnoptr = ENOTDIR;
      return -1;
    }

    // Get the directory structure
    inode_directory_t *directory = &node->value.directory;

    // Check if the directory is empty or contains only "." and ".."
    if (directory->num_children == 1) {
      return 0;
    }

    size_t n_children = directory->num_children;
    // Allocate space for the names of all children, except "." and ".."
    void **ptr = (void **)calloc(n_children - ((size_t)1), sizeof(char *));
    fs_offset *children = offset_to_pointer(fsptr, directory->children);

    // Check that calloc was successful
    if (ptr == NULL) {
      *errnoptr = EINVAL;
      return -1;
    }

    char **names = ((char **) ptr);
    size_t len;
    // Fill the array with the names of the directory entries
    for (size_t i = ((size_t)1); i < n_children; i++) {
      node = ((inode_t *)offset_to_pointer(fsptr, children[i]));  // Child node
      len = strlen(node->name);
      names[i - 1] = (char *)malloc(len + 1);
      strcpy(names[i - 1], node->name);  // strcpy(dst,src)
      names[i - 1][len] = '\0';
    }

    *namesptr = names;
    return ((int)(n_children - 1));
}

int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  mount_filesystem(fsptr, fssize);

  // Make a directory, 1 because it is a file
  inode_t *node = make_node(fsptr, path, errnoptr, 1);

  // Check if the node was successfully created, if it wasn't the errnoptr was
  // already set so we just return failure with -1
  if (node == NULL) {
    return -1;
  }

  return 0;
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

int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  mount_filesystem(fsptr, fssize);

  // Make a directory, 0 because it is not a file
  inode_t *node = make_node(fsptr, path, errnoptr, 0);

  // Check if the node was successfully created, if it wasn't the errnoptr was
  // already set so we just return failure with -1
  if (node == NULL) {
    return -1;
  }

  return 0;
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

int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {
  mount_filesystem(fsptr, fssize);

  // Check if offset is negative
  if (offset < 0) {
    *errnoptr = EFAULT; // Bad address
    return -1;
  }

  size_t new_size = (size_t)offset;

  // Resolve the path to get the node representing the file
  inode_t *node = resolve_path(fsptr, path, 0);

  // Check if the path is valid
  if (node == NULL) {
    *errnoptr = ENOENT; // No such file or directory
    return -1;
  }

  // Ensure that the node represents a file
  if (node->type == 1) {
    *errnoptr = EISDIR; // Is a directory
    return -1;
  }

  inode_file_t *file = &node->value.file;
  file_block_t *block = offset_to_pointer(fsptr, file->first_block);

  // If the new size is the same, do nothing
  if (file->size == new_size) {
    update_time(node, 0); // File access only
    return 0;
  }
  // If the new size is smaller, remove excess data
  else if (file->size > new_size) {
    update_time(node, 1); // File access and modification

    // Update the file size
    file->size = new_size;

    // Remove excess data from the file
    remove_data(fsptr, block, new_size);
  }
  // If the new size is larger, append zeros to extend the file
  else {
    update_time(node, 1); // File access and modification

    // Extend the file by appending zeros
    if (add_data(fsptr, file, new_size, errnoptr) != 0) {
      return -1;
    }
  }

  return 0; // Success
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
