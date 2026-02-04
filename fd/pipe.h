/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipe.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 14:35:19 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:42:33 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PIPE_H
#define PIPE_H

#include "../common.h"
#include "fd.h"

#define PIPE_SIZE 8192

typedef struct s_pipe
{
	uint8_t buf[PIPE_SIZE];
	uint32_t rpos, wpos;
	uint32_t used;
	uint32_t readers;
	uint32_t writers;
	// wait queues / blocked lists if you have them
} t_pipe;

typedef struct s_pipe_end
{
	t_pipe *p;
	bool is_read_end; // 1 for read end, 0 for write end
} t_pipe_end;

int32_t pipe_read(t_file *f, void *buf, size_t n);
int32_t pipe_write(t_file *f, const void *buf, size_t n);
int32_t pipe_close(t_file *f);

extern t_file_ops g_pipe_read_ops;
extern t_file_ops g_pipe_write_ops;

#endif