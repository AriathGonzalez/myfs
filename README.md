# FUSE Filesystem Implementation

## Overview

This project is a custom **FUSE-based filesystem** designed to implement and manage a simple, persistent filesystem in user space. The filesystem supports basic file and directory operations, including file creation, directory listing, file truncation, and file access management.

The main focus of this project was on **memory management**, **system-level programming**, and **Linux integration**, allowing for seamless interaction with standard Unix tools and commands.

---

## Features

- **Filesystem Initialization**: Initializes the filesystem with persistent state recovery. Ensures that the filesystem doesn’t get reinitialized at mount time if it was already initialized.
- **Memory Management**: Implements dynamic memory allocation and deallocation within the filesystem’s memory space, managing free memory regions.
- **File and Directory Representation**: Creates hierarchical data structures for representing files, directories, and metadata, storing the location of the root directory.
- **Filesystem Operations**: Implements essential Unix filesystem operations (`getattr`, `readdir`, `mknod`, `mkdir`, `truncate`), ensuring compatibility with commands like `ls`, `touch`, and `truncate`.

---

## Key Components

1. **Filesystem Handle**  
   This structure manages the filesystem's state, including memory allocation and the root directory's location. It ensures that the filesystem is initialized correctly and avoids re-initialization when remounting.

2. **Memory Allocation**  
   The project includes functions for finding and allocating free memory regions within the filesystem’s memory space. It also features mechanisms to free up these regions when no longer needed.

3. **Filesystem Traversal**  
   Helper functions are implemented for path resolution, enabling the traversal of directories and paths within the filesystem. These functions help ensure smooth file access and modification operations.

4. **Unix Compatibility**  
   The filesystem is designed to work seamlessly with basic Unix commands such as `ls`, `touch`, and `truncate`, mimicking standard filesystem behaviors.

---

## Installation

### Requirements:
- **Linux** (or a Unix-based system)
- **FUSE library** installed on your system
- **C compiler** (GCC or Clang)
  
To install FUSE on a Linux system, you can typically use your package manager. For example:

```bash
sudo apt-get install libfuse-dev
```

## Build

```bash
git clone https://github.com/yourusername/fuse-filesystem.git
cd fuse-filesystem
make
```

## Usage

```bash
mkdir /mnt/myfs
./myfs /mnt/myfs
```

