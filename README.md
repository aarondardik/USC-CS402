# USC CSCI 402 — Operating Systems

Implementation of a Unix-like operating system kernel in C, completed as part of USC's CSCI 402 Operating Systems course.

The project was developed in three stages, progressively implementing core operating-system functionality from process management through filesystems and virtual memory.

## Project Overview

### Kernel 1 — Processes and Threads

The first stage focuses on fundamental process and thread management.

Key concepts include:

- Kernel-level process and thread management
- Thread scheduling and synchronization
- System calls
- Process lifecycle management
- Low-level C programming and pointer manipulation

### Kernel 2 — Virtual File System

The second stage implements a virtual filesystem and the interfaces required to interact with persistent storage.

Key components include:

- Virtual File System (VFS)
- On-disk filesystem
- File and directory operations
- Block-device interfaces
- ATA/DMA device drivers
- Kernel/user-space interfaces

### Kernel 3 — Virtual Memory

The final stage implements a virtual memory subsystem.

Key components include:

- Page tables and address translation
- Virtual-to-physical memory mapping
- Page allocation and management
- Copy-on-write
- `mmap`
- Memory protection and fault handling

## Technologies

- **Language:** C
- **Systems:** Processes, threads, filesystems, device drivers, virtual memory
- **Architecture:** Unix-like operating-system kernel

## What I Learned

This project provided hands-on experience with the mechanisms underlying modern operating systems, particularly the interaction between processes, memory, filesystems, and hardware.

Working at the kernel level required careful reasoning about pointers, memory ownership, concurrency, resource management, and the boundary between hardware and software. The project also provided experience debugging systems where errors can propagate across multiple layers of the software stack.

## Repository Structure

```text
USC-CS402/
├── Kernel 1/
├── Kernel 2/
└── Kernel 3/
```

Each directory contains the corresponding stage of the kernel implementation.
