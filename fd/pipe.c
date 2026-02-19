/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipe.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 14:35:14 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/19 22:22:24 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "pipe.h"
#include "../tasks/task.h"
#include "../errno.h"

t_file_ops g_pipe_read_ops = {.read = pipe_read, .write = NULL, .close = pipe_close};
t_file_ops g_pipe_write_ops = {.read = NULL, .write = pipe_write, .close = pipe_close};

static inline uint32_t pipe_space(t_pipe *p) { return sizeof(p->buf) - p->used; }

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
	t_pipe_end *pipe_end = (t_pipe_end *)f->priv;
	if (!pipe_end || !pipe_end->is_read_end)
		return (-EBADF);
	t_pipe *pipe = pipe_end->p;
	if (!pipe)
		return (-EBADF);
	uint8_t *buf = (uint8_t *)buf_; //syscall_read validated this buffer already
	uint32_t readed = 0;
	//bool was_full = (pipe->used == sizeof(pipe->buf));
	while (!readed)
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
			if (readed)
			{
				waitq_wake_one(&pipe->wait_write);
				return (readed);
			}
			return (-EAGAIN);
		}
		else if(!readed && pipe->writers)
		{
			print_debug("sleeping in pipe read pid %u\n", g_curr_task->task_id);
			sleep_on(&pipe->wait_read, WAIT_PIPE_READ);
		}
		if (!readed && has_pending_signals(g_curr_task) && !(flags_first_pending_signal(g_curr_task) & SA_RESTART))
		{
			print_trace("pipe_read: woken up by signal didn't read anything yet, return -EINTR\n");
			return (-EINTR);
		}
	}
	waitq_wake_one(&pipe->wait_write);
	if (pipe->used)
		waitq_wake_one(&pipe->wait_read); //in case another reader is waiting but no writer sleeping on wait_write to wake them up
	return (readed);
}

int32_t pipe_write(t_file *f, const void *buf_, size_t n)
{
	t_pipe_end *pipe_end = (t_pipe_end *)f->priv;
	if (!pipe_end || pipe_end->is_read_end)
		return (-EBADF);
	t_pipe *pipe = pipe_end->p;
	if (!pipe)
		return (-EBADF);
	uint8_t *buf = (uint8_t *)buf_; //syscall_write validated this buffer already
	uint32_t wrote = 0;
	//bool was_empty = pipe->used == 0;
	while (!wrote)
	{
		if (!pipe->readers)
		{
			enqueue_sig(g_curr_task, SIGPIPE);
			return (-EPIPE);
		}
		else if (((n <= PIPE_BUF) && (pipe_space(pipe) >= n)) || ((n > PIPE_BUF) && (pipe_space(pipe))))
		{
			wrote += pipe_write_ring(pipe, buf + wrote, (uint32_t)(n - wrote));
		}
		if (f->flags & O_NONBLOCK)
		{
			if (wrote)
			{
				waitq_wake_one(&pipe->wait_read);
				return (wrote);
			}
			return (-EAGAIN);
		}
		else if(!wrote && pipe->readers)
		{
			print_debug("yieded in pipe write\n");
			sleep_on(&pipe->wait_write, WAIT_PIPE_WRITE);
		}
		if (!wrote && has_pending_signals(g_curr_task) && !(flags_first_pending_signal(g_curr_task) & SA_RESTART))
		{
			print_trace("pipe_write: woken up by signal, didn't write anything yet, return -EINTR\n");
			return (-EINTR);
		}
	}
	waitq_wake_one(&pipe->wait_read);
	if (pipe_space(pipe))
		waitq_wake_one(&pipe->wait_write); //in case another writer is waiting but no reader sleeping on wait_read to wake them up
	return (wrote);
}

// vfree pipe_end
// vfree pipe if no reader/writers left
// doesn't free file, assumed to not be dynamically allocated
int32_t pipe_close(t_file *f)
{
	t_pipe_end *pipe_end = (t_pipe_end *)f->priv;
	if (!pipe_end)
		return (-EBADF);
	t_pipe *pipe = pipe_end->p;
	if (!pipe)
		return (-EBADF);
	if (pipe_end->is_read_end)
	{
		if (pipe->readers)
			pipe->readers = 0;
		else
		 	kernel_panic("uncoherent pipe state \n", NULL);
		waitq_wake_all(&pipe->wait_write);
	}
	else
	{
		if (pipe->writers)
			pipe->writers = 0;
		else
		 	kernel_panic("uncoherent pipe state \n", NULL);
		waitq_wake_all(&pipe->wait_read);
	}
	if (!pipe->readers && !pipe->writers)
	{
		vfree(pipe);
	}
	vfree(pipe_end);
	f->priv = NULL;
	return (0);
}
