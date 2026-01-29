/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmap.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:24:53 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/29 16:19:53 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MMAP_H
#define MMAP_H

#include "../common.h"
#include "../tasks/task.h"

#define USERLAND_HEAP_START_VA 0x200000u

enum e_mmap_prot
{
	PROT_EXEC = 4u,	 // Pages may be executed.
	PROT_READ = 1u,	 // Pages may be read.
	PROT_WRITE = 2U, // Pages may be written.
	PROT_NONE = 0U,	 // Pages may not be accessed.
};

enum e_mmap_flag
{
	MAP_SHARED = 0x1u,
	MAP_PRIVATE = 0x2u,

	MAP_ANONYMOUS = 0x20,
	MAP_FIXED = 0x10, // only posix (required) flag
};

#define MAP_FAILED ((void *)-1)

void *mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset, t_mm *mm);
int syscall_munmap(void *addr, size_t length);

/*Return Value
On success, mmap() returns a pointer to the mapped area. On error, the value MAP_FAILED (that is, (void *) -1) is returned, and errno is set appropriately. On success, munmap() returns 0, on failure -1, and errno is set (probably to EINVAL).
Errors

EACCES
	A file descriptor refers to a non-regular file. Or MAP_PRIVATE was requested, but fd is not open for reading. Or MAP_SHARED was requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode. Or PROT_WRITE is set, but the file is append-only.
EAGAIN
	The file has been locked, or too much memory has been locked (see setrlimit(2)).
EBADF
	fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).
EINVAL
	We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary).
EINVAL
	(since Linux 2.6.12) length was 0.
EINVAL
	flags contained neither MAP_PRIVATE or MAP_SHARED, or contained both of these values.
ENFILE
	The system limit on the total number of open files has been reached.
ENODEV
	The underlying file system of the specified file does not support memory mapping.
ENOMEM
	No memory is available, or the process's maximum number of mappings would have been exceeded.
EPERM
	The prot argument asks for PROT_EXEC but the mapped area belongs to a file on a file system that was mounted no-exec.
ETXTBSY
	MAP_DENYWRITE was set but the object specified by fd is open for writing.
EOVERFLOW
	On 32-bit architecture together with the large file extension (i.e., using 64-bit off_t): the number of pages used for length plus number of pages used for offset would overflow unsigned long (32 bits).

Use of a mapped region can result in these signals:

SIGSEGV
	Attempted write into a region mapped as read-only.
SIGBUS
	Attempted access to a portion of the buffer that does not correspond to the file (for example, beyond the end of the file, including the case where another process has truncated the file).*/

#endif