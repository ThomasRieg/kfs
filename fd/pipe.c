/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipe.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 14:35:14 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/05 16:43:57 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "pipe.h"
#include "../tasks/task.h"
#include "../errno.h"

t_file_ops g_pipe_read_ops = {.read = pipe_read, .write = NULL, .close = pipe_close};
t_file_ops g_pipe_write_ops = {.read = NULL, .write = pipe_write, .close = pipe_close};

static inline uint32_t pipe_space(t_pipe *p) { return PIPE_SIZE - p->used; }

static uint32_t pipe_read_ring(t_pipe *p, uint8_t *dst, uint32_t n)
{
	uint32_t total = 0;
	while (total < n && p->used)
	{
		uint32_t chunk = n - total;
		uint32_t until_end = PIPE_SIZE - p->rpos;
		if (chunk > until_end)
			chunk = until_end;
		if (chunk > p->used)
			chunk = p->used;

		memcpy(dst + total, p->buf + p->rpos, chunk);
		p->rpos = (p->rpos + chunk) % PIPE_SIZE;
		p->used -= chunk;
		total += chunk;
	}
	return total;
}

static uint32_t pipe_write_ring(t_pipe *p, const uint8_t *src, uint32_t n)
{
	uint32_t total = 0;
	while (total < n && pipe_space(p))
	{
		uint32_t chunk = n - total;
		uint32_t until_end = PIPE_SIZE - p->wpos;
		uint32_t space = pipe_space(p);

		if (chunk > until_end)
			chunk = until_end;
		if (chunk > space)
			chunk = space;

		memcpy(p->buf + p->wpos, src + total, chunk);
		p->wpos = (p->wpos + chunk) % PIPE_SIZE;
		p->used += chunk;
		total += chunk;
	}
	return total;
}

int32_t pipe_read(t_file *f, void *buf_, size_t n)
{
	// printk("pipe_read called\n");
	t_pipe *pipe = ((t_pipe_end *)f->priv)->p;
	uint8_t *buf = (uint8_t *)buf_;
	uint32_t readed = 0;
	while (readed < n)
	{
		if (pipe->used)
		{
			readed += pipe_read_ring(pipe, buf + readed, (uint32_t)(n - readed));
		}
		else if (!pipe->writers)
		{
			return (readed); // no read = readed 0 = eof
		}
		if (f->flags & O_NONBLOCK)
		{
			return readed ? (readed) : (-EAGAIN);
		}
		// printk("pipe_read yielded\n");
		yield(); // TODO implement real sleep
	}
	return (readed);
}

int32_t pipe_write(t_file *f, const void *buf_, size_t n)
{
	// printk("pipe_write called\n");
	t_pipe *pipe = ((t_pipe_end *)f->priv)->p;
	uint8_t *buf = (uint8_t *)buf_;
	uint32_t wrote = 0;
	while (wrote < n)
	{
		if (!pipe->readers)
		{
			g_curr_task->pending_signals |= SIGPIPE;
			return (-EPIPE);
		}
		else if (pipe_space(pipe))
		{
			wrote += pipe_write_ring(pipe, buf + wrote, (uint32_t)(n - wrote));
			printk("debug wrote %u in pipe\n", wrote);
		}
		if (f->flags & O_NONBLOCK)
		{
			return wrote ? (wrote) : (-EAGAIN);
		}
		yield(); // TODO implement real sleep
	}
	return (wrote);
}

// vfree pipe_end
// vfree pipe if no reader/writers left
// doesn't free file, assumed to not be dynamically allocated
int32_t pipe_close(t_file *f)
{
	t_pipe_end *pipe_end = (t_pipe_end *)f->priv;
	t_pipe *pipe = pipe_end->p;
	if (pipe_end->is_read_end)
	{
		pipe->readers--;
		// TODO wake writers if 0 readers when sleep system implemented
	}
	else
	{
		pipe->writers--;
		// TODO wake writers if 0 readers when sleep system implemented
	}
	if (!pipe->readers && !pipe->writers)
	{
		vfree(pipe);
	}
	vfree(pipe_end);
	return (0);
}
