# Advanced-Embedded-Operating-system

JOS Kernel development (C, Assembly language, GDB)
Aug 2016 – Dec 2016
Implementation of the JOS exo-kernel for x86 architecture emulated by QEMU.
1. Memory Management Infrastructure: Implemented a physical page allocator, configured page directory and page
tables for kernel and user processes to provide an address space abstraction.
2. Interrupt and Exception Handling: Configured the Interrupt Descriptor Table (IDT), wrote handlers for interrupts
and exceptions and implemented protected control transfer from user to kernel mode for execution of system calls.
3. Process Management: Implemented copy-on-write fork to spawn processes, configured pre-emptive round-robin
scheduling using timer interrupts and wrote kernel level APIs for Inter-Process Communication (IPC).
4. Symmetric Multiprocessing: Initialized application processors from bootstrap processor using MP configuration
table and APIC units. Implemented big kernel lock to provide concurrency control.
5. Network Stack: Implemented the whole device driver for Intel 82540EM (E1000) network interface card to help the
kernel access the Ethernet. Usage of open-source lwIP as protocol stack and BSD socket interface to communicate
with the server. Implemented system calls to help user programs access network resources.
