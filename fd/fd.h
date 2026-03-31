/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd.h                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 14:39:32 by thrieg            #+#    #+#             */
/*   Updated: 2026/03/24 16:39:58 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FD_H
#define FD_H

#include "../common.h"

struct pollfd
{
	int fd;		   /* file descriptor */
	short events;  /* requested events */
	short revents; /* returned events */
};

enum e_flags
{
	O_RDONLY = 0,
	O_WRONLY = 1,
	O_RDWR = 2,
	O_CREAT = 0100,
	O_TRUNC = 01000,
	O_NONBLOCK = 2048,
	O_APPEND = 02000,
	O_CLOEXEC = 524288,
};

enum open_file_type
{
	FILE_REGULAR,
	FILE_PIPE,
	FILE_SOCKET,
	FILE_TERMINAL,
};

typedef struct s_file t_file;

typedef int32_t (*read_fn)(t_file *f, void *buf, uint32_t n);
typedef int32_t (*write_fn)(t_file *f, const void *buf, uint32_t n);
typedef int32_t (*close_fn)(t_file *f);
typedef int32_t (*ioctl_fn)(t_file *f, unsigned int op, unsigned int val);
typedef int32_t (*poll_fn)(t_file *file, struct pollfd *fd, int timeout);

typedef struct s_file_ops
{
	read_fn read;
	write_fn write;
	close_fn close; // only called when fd refcnt becomes 0 in syscall_close
	ioctl_fn ioctl;
	poll_fn poll;
	// later: mmap, seek, readdir, fstat...
} t_file_ops;

typedef struct s_file
{
	uint32_t refcnt;
	uint32_t flags;
	enum open_file_type type;
	uint64_t pos; // for regular files; ignored for pipes/sockets
	void *priv;	  // points to inode/pipe_end/socket
	t_file_ops *ops;
} t_file;

#endif
