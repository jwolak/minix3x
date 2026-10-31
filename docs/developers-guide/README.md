# Minix3x Developers Guide

This guide collects practical information for developing and contributing to
this Minix3x repository. It complements the existing manual pages and
repository-specific instructions in the main [README](../../README.md).

## Contents

- [Overview of Minix3x architecture](#overview-of-minix3x-architecture)
  - [Micro-kernel](#micro-kernel)
    - [Message passing](#message-passing)
    - [Memory grants](#memory-grants)
    - [Kernel system calls](#kernel-system-calls)
  - [User-mode server processes](#user-mode-server-processes)
  - [Drivers](#drivers)
  - [Servers](#servers)
  - [User-land](#user-land)
- [Minix3x Kernel API](#minix3x-kernel-api)
  - [Organization of Kernel Calls](#organization-of-kernel-calls)
  - [Overview of kernel calls in Minix3x](#overview-of-kernel-calls-in-minix3x)
  - [Minix3x kernel call interface](#minix3x-kernel-call-interface)
    - [SYS_ABORT](#sys_abort)

## Overview of Minix3x architecture

**Minix3x** is a POSIX-compatible operating system based on a microkernel architecture. It consists of a minimal kernel and a collection of user-mode server processes that provide system services to user-space applications and users.

The main goal of its architecture is to achieve **high reliability**, **fault isolation**, and **security**. By running most operating system components as user-mode processes, Minix3x follows the principle of least authority, restricting the resources and operations accessible to each component.

The microkernel is responsible for essential low-level functionality, such as **interrupt handling**, low-level **process scheduling**, and **inter-process communication (IPC)**. Higher-level services, including **device drivers** and **file system servers**, run outside the kernel in isolated user-mode processes.

This design reduces the amount of code running in privileged mode and limits the impact of individual component failures. If a service or driver crashes, it can potentially be restarted without requiring the entire operating system to reboot. This approach also improves modularity, as individual components can be developed, replaced, and maintained independently.

Key benefits of this architecture:

- **Fault isolation** – A failure in a single device driver or server does not necessarily cause the entire operating system to crash.
- **Self-healing** – Certain services can be automatically restarted after a failure, depending on the component and system configuration.
- **Inter-process communication (IPC)** – System components communicate through controlled communication mechanisms rather than relying on direct function calls within a shared kernel space.
- **Security** – Running less code with kernel-level privileges reduces the potential impact of bugs and security vulnerabilities.
- **Modularity and maintainability** – Device drivers and system services can be developed, replaced, and maintained as independent components.

**Comparison with monolithic kernels** – In operating systems such as Linux, many device drivers and kernel services run in kernel space, whereas Minix3x moves more system components into user space.

Overall, Minix3x aims to provide a reliable, secure, and modular operating system by minimizing the amount of code running in kernel space and isolating system components from one another. This architecture helps limit the impact of failures and makes individual components easier to maintain and recover.

### Micro-kernel

The microkernel is the core component of **Minix3x**. It is responsible for essential low-level functionality, including interrupt handling, process scheduling, basic process management, and inter-process communication (IPC).

The microkernel is the only component of Minix3x that runs in kernel mode. This means that it has access to privileged CPU operations and is responsible for coordinating access to critical hardware resources. These include hardware interrupts, low-level memory protection mechanisms, and the facilities required to manage and isolate processes.

Unlike monolithic kernels, Minix3x keeps most operating system services outside the kernel, running them as separate user-mode processes. Device drivers, file system servers, and other system services interact with the microkernel through controlled interfaces and IPC mechanisms rather than accessing kernel internals directly.

Keeping the microkernel small reduces the amount of code that runs with elevated privileges, helping to minimize the potential impact of bugs and security vulnerabilities. It also establishes a foundation for fault isolation: failures in user-mode components can be contained without necessarily affecting the entire system.

By delegating higher-level functionality to user-mode services, the microkernel provides a minimal and controlled execution environment focused on process management, communication, and the fundamental mechanisms required to run the operating system.

#### Message passing

The micro-kernel provides a form of inter-process communication called message passing, in which messages are exchanged to communicate to the rest of the system. The messages themselves are fixed-size 64 bytes buffers

#### Memory grants

To transfer larger amounts of data, memory grants allow processes to grant other processes indirect read and/or write access to well-defined memory regions of its virtual address space. The grants are managed by the process that emitted them and the kernel performs the act of reading/writing upon request. The memory grant credentials (4 byte identifiers) are exchanged by message passing.

#### Kernel system calls

Kernel system calls are provided to allow access to message passing, memory grants and the hardware to the rest of the system. They are provided by the micro-kernel and are specific to Minix. In particular, they are not to be confused with POSIX system calls, which are implemented at a higher abstraction level.

System calls (and thus message passing and memory grants) are subject to strict permissions checking. A process is limited by which system calls it is allowed to make, to whom it may send messages and which memory grants it has access to. Any unauthorized action is met with a error code saying no permission.

### User-mode server processes

On top of the micro-kernel is running a collection of server processes that offer services to the rest of the system. Those processes run in user mode and thus lack the authority to directly access hardware or memory that doesn't belong to them. They use system calls to interact with other processes and hardware through the micro-kernel.

In Minix3x everything but the micro-kernel is a user process, however server processes are conceptually divided in two layers.

### Drivers

The drivers are processes who handle a specific hardware component. Each of those processes has access through the micro-kernel to its hardware resources (input/output ports, interrupts).

Examples of processes belonging in this layer are mass storage (floppy disk, hard disk, CD/DVD/Blu-Ray/tape drive…), network card, printer and serial port drivers.

### Servers

The servers are processes offering services to the rest of the system. To do so, they use driver processes, the micro-kernel or other servers to perform the requests they receive. Servers do not have access to hardware resources like drivers do.

Examples of servers are the virtual memory, virtual file system, actual file systems, network stack and data store servers.

## User-land

The user-land is the collection of processes, libraries, files and components that the users of the operating system use to get work done. User-land processes are processes just like drivers and servers, but unlike those they do not have specific privileges. To access facilities such as the file system or the network, they perform requests to servers. The application programming interface (API) provided at the lowest layer is POSIX, a set of common system calls (not the Minix micro-kernel ones) and libraries functions all Unix and Unix-like operating systems share.

Since Minix 3.2.0, most of the user-land is imported from NetBSD, with Minix-specific utilities mostly for administrating the servers and drivers. In a sense, Minix is now a reimplementation of the NetBSD user-land on top of a micro-kernel and now looks familiar to BSD users and developers.

Minix3x is provided with a base system mostly identical to the NetBSD one. Extra software and utilities is usually installed via pkgin, which is a binary package manager for pkgsrc, the huge collection of programs and libraries ports available on NetBSD and other operating systems.

## Minix3x Kernel API

In general, kernel calls allows system processes to request kernel services, for example to perform privileged operations. This document briefly discusses the organization of kernel calls in Minix3x and provides an overview of all kernel calls.

### Organization of Kernel Calls

A kernel call means that a request is sent to a kernel where it is handled by one of the kernel tasks. The details of assembling a request message, sending it to the kernel and awaiting the response are conveniently hidden in a system library. The header file of this library is ```src/include/minix/syslib.h``` and its implementation is found in ```src/lib/syslib```.

The actual implementation of the kernel calls is defined in the ```SYSTEM``` kernel task. Suppose that a program makes a ```sys_call()``` system call. By convention, this call is transformed into a request message with type ```SYS_CALL``` that is sent to the kernel task SYSTEM. The ```SYSTEM``` task handles the request in a function named ```do_call()``` and returns the result.

The mapping of kernel call numbers and handler functions is done during the ```SYSTEM``` task's initialization response ```src/kernel/system.c```. The prototypes of the handler functions are declared in ```src/kernel/system.h```. Their implementation is contained in separate files in the directory ```src/kernel/system/```. These files are compiled into the library ```/src/kernel/system/system.a``` that is linked to the kernel.

The kernel call numbers and their request and response parameters are defined in ```src/include/minix/com.h```. Kernel calls all start with ```SYS_``` and all parameters that belong to the same kernel call share a common prefix.

### Overview of kernel calls in Minix3x

| Kernel Call | Purpose |
|-------------|---------|
| *SYSTEM CONTROL* | |
| [SYS_ABORT](#sys_abort) | Abort Minix3x; the system is unable to continue |

### Minix3x kernel call interface

#### SYS_ABORT

Shutdown Minix3x and return to the boot monitor, if possible. This is used by PM, FS and TTY. Normal aborts usually are initiated by the user, for example by means of the shutdown command or typing Ctrl-Alt-Del. Minix3x will also be taken down if a fatal error occurs in the PM or FS.

The callers mentioned above are:

| Abbreviation | Name | Description |
| -------------- | ------ | ------------- |
| **PM** | Process Manager | Server that manages processes: creating and terminating them (`fork`, `exec`, `exit`, `wait`), signals, and process groups. Source: [`minix/servers/pm`](../../minix/servers/pm). |
| **FS** | File System | Server that handles file system requests from user programs, such as opening, reading, writing, and mounting. In current Minix3x sources it is implemented by the Virtual File System (VFS) server, which forwards requests to the actual file system servers. Source: [`minix/servers/vfs`](../../minix/servers/vfs). |
| **TTY** | Terminal driver | Driver that handles the console, keyboard, and terminal devices. It reports the Ctrl-Alt-Del key combination, which can start a system shutdown. Source: [`minix/drivers/tty/tty`](../../minix/drivers/tty/tty). |

##### Request parameters

| Parameter | Description |
| --------- | ----------- |
| `how` | How to shut down the system. A combination of `RB_*` flags defined in [`sys/sys/reboot.h`](../../sys/sys/reboot.h). It is passed in the `m_lsys_krn_sys_abort.how` field of the request message to `prepare_shutdown()` in the kernel. |

Values of `how` that affect the shutdown behavior:

| Value | Behavior |
| ----- | -------- |
| `RB_AUTOBOOT` (0) | Reset the system (reboot). This is the default when no other flag is set. |
| `RB_HALT` | Halt the system without rebooting; it is safe to turn off the computer. |
| `RB_POWERDOWN` | Halt and turn the power off, if supported (`RB_HALT` with an additional flag). |

The kernel does not shut down immediately. `prepare_shutdown()` sets a timer for one second, so that other processes can finish their shutdown work. When it fires, `minix_shutdown()` disables hardware interrupts and the local timer, prints the shutdown message, and calls the architecture-specific `arch_shutdown()`.

##### Return values

| Value | Meaning |
| ----- | ------- |
| `OK` | The shutdown sequence was started. The call is not expected to return to a running system. |

##### Library functions

```c
int sys_abort(int how);
```

Defined in [`minix/lib/libsys/sys_abort.c`](../../minix/lib/libsys/sys_abort.c). The kernel handler is `do_abort()` in [`minix/kernel/system/do_abort.c`](../../minix/kernel/system/do_abort.c).

##### Example

A regular user program cannot call `sys_abort()`: the kernel checks the caller's privileges, and only system services whose configuration lists `ABORT` in their `system` section are allowed to use it. User programs shut the system down with `reboot(2)` or `shutdown(8)`, which go through PM.

The example service [`abortdemo`](../../minix/drivers/examples/abortdemo/abortdemo.c) shows the call in practice. Its [configuration](../../minix/drivers/examples/abortdemo/abortdemo.conf) grants the `ABORT` kernel call. **Starting it shuts the whole system down.** Run it as root, preferably in an emulator:

```bash
minix-service up /service/abortdemo -config /etc/system.conf.d/abortdemo -args "halt"
```

The argument selects the `how` value: `halt` (`RB_HALT`, the default), `poweroff` (`RB_POWERDOWN`) or `reboot` (`RB_AUTOBOOT`). About one second after the call, the kernel prints the shutdown message and stops the system as described above.

###### Building the example without building the whole system

After one full build of the system (see the main [README](../../README.md#building-the-system)), the toolchain and the installed libraries already exist in the object directory, so the example can be built on its own. The paths below assume the object directory `../obj.i386` used in the build command:

```bash
cd minix/drivers/examples/abortdemo
TOOLDIR=$(ls -d ../../../../../obj.i386/tooldir*)
$TOOLDIR/bin/nbmake-i386 obj    # only needed the first time
$TOOLDIR/bin/nbmake-i386
```

The resulting static i386 executable is `../obj.i386/minix/drivers/examples/abortdemo/abortdemo`. To get it into the system, copy `abortdemo` to `/service/` and `abortdemo.conf` to `/etc/system.conf.d/abortdemo` on a running Minix3x, or run `$TOOLDIR/bin/nbmake-i386 install` to install both into the destination directory (`destdir`) and then create a new disk image as described in the main README.

If you change a library such as `libsys`, rebuild and install it first in the same way (`nbmake-i386` and `nbmake-i386 install` in `minix/lib/libsys`); otherwise the example is linked against the previously installed version.

```cpp
#include <minix/drivers.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <string.h>

static int abort_how = RB_HALT;

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
    int sys_abort_result = -1;

    /* Only act on a fresh start, not after a restart by RS. */
    if (type != SEF_INIT_FRESH)
        return OK;

    printf("abortdemo: calling sys_abort(0x%x)\n", abort_how);

    /*
     * The kernel does not stop at once: it prints a message and shuts down
     * after about one second, so that other processes can finish their work.
     */
    sys_abort_result = sys_abort(abort_how);
    if (sys_abort_result != OK)
        printf("abortdemo: sys_abort failed: %d\n", sys_abort_result);

    return sys_abort_result;
}

int main(int argc, char **argv)
{
    message msg;
    int result = -1;
    int status = -1;

    if (argc > 1) {
        if (strcmp(argv[1], "reboot") == 0)
            abort_how = RB_AUTOBOOT;
        else if (strcmp(argv[1], "poweroff") == 0)
            abort_how = RB_POWERDOWN;
        else if (strcmp(argv[1], "halt") != 0)
            printf("abortdemo: unknown mode '%s', using halt\n", argv[1]);
    }

    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);
    sef_startup();

    /*
     * Keep running until the system goes down; exiting now would make RS
     * restart the service. A HARD_STOP notification arrives before shutdown.
     */
    for (;;) {
        if ((result = sef_receive_status(ANY, &msg, &status)) != OK)
            printf("abortdemo: sef_receive_status failed: %d\n", result);
    }

    return OK;
}

```
