/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd.h                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 14:39:32 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:22:03 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FD_H
#define FD_H

#include "../common.h"

enum e_flags
{
	O_RDONLY = 1,
	O_WRONLY = (1 << 1),
	O_NONBLOCK = (1 << 2),
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

typedef struct s_file_ops
{
	read_fn read;
	write_fn write;
	close_fn close; //only called when fd refcnt becomes 0 in syscall_close
	// later: ioctl, poll, mmap, seek, readdir, fstat...
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