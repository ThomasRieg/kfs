/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_tty.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 16:17:51 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 03:15:11 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "fd_tty.h"
#include "../errno.h"
#include "../tasks/task.h"
#include "../libk/libk.h"
#include <stdint.h>

t_file_ops g_tty_ops = {.read = tty_read, .write = tty_write, .close = tty_close, .ioctl = tty_ioctl};

struct winsize
{
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel; /* unused */
	unsigned short ws_ypixel; /* unused */
};

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410
#define TIOCGWINSZ 0x5413
#define TIOCGWINSZ 0x5413

int32_t tty_read(t_file *f, void *buf, size_t n)
{
	if (f && buf && n) {
		t_tty *tty = f->priv;
		while (!tty->cmd.index && !tty->read_eof) {
			print_trace("tty_read: sleeping because no tty ready\n");
			sleep_on(&tty->wait_read, WAIT_TTY_READ);
		}
		if (tty->read_eof) {
			if (tty->cmd.index)
				waitq_wake_one(&tty->wait_read); //wake another reader to finish the cmd
			tty->read_eof = false;
			return 0;
		}

		uint32_t to_copy = n < tty->cmd.index ? n : tty->cmd.index;
		memcpy(buf, tty->cmd.buffer, to_copy);
		memmove(buf, buf + to_copy, tty->cmd.index - to_copy);
		tty->cmd.index -= to_copy;
		if (tty->cmd.index)
				waitq_wake_one(&tty->wait_read); //wake another reader to finish the cmd
		return to_copy;
	}
	return (-EINVAL);
}

int32_t tty_write(t_file *f, const void *buf, size_t n)
{
	if (!f)
		return (-EBADF);
	t_tty *tty = f->priv;
	if (!tty)
		return (-EBADF);
	write(buf, n);
	waitq_wake_one(&tty->wait_read);
	return (n);
}

int32_t tty_close(t_file *f)
{
	if (!f)
		return (-EBADF);
	t_tty *tty = f->priv;
	if (!tty)
		return (-EBADF);
	waitq_wake_all(&tty->wait_read);
	waitq_wake_all(&tty->wait_write);
	return (-ENOSYS);
}

int32_t tty_ioctl(t_file *f, unsigned int op, unsigned int val)
{
	t_tty *tty = f->priv;
	switch (op) {
	case TIOCGWINSZ:
		if (!user_range_ok((virt_ptr)val, sizeof(struct winsize), true, &g_curr_task->proc_memory))
			return -EFAULT;
		struct winsize *ws = (struct winsize *)val;
		ws->ws_row = 25;
		ws->ws_col = 80;
		ws->ws_xpixel = 0;
		ws->ws_ypixel = 0;
		return 0;
	case TIOCGPGRP:
		if (!user_range_ok((virt_ptr)val, sizeof(unsigned int), true, &g_curr_task->proc_memory))
			return -EFAULT;
		unsigned int *pgrp = (unsigned int *)val;
		*pgrp = 0;
		return 0;
	case TIOCSPGRP:
		if (!user_range_ok((virt_ptr)val, sizeof(unsigned int), false, &g_curr_task->proc_memory))
			return -EFAULT;
		return 0;
	case TCGETS:
		if (!user_range_ok((virt_ptr)val, sizeof(struct termios), true, &g_curr_task->proc_memory))
			return -EFAULT;
		*(struct termios *)val = tty->termios;
		return 0;
	case TCSETS:
		if (!user_range_ok((virt_ptr)val, sizeof(struct termios), false, &g_curr_task->proc_memory))
			return -EFAULT;
		tty->termios = *(struct termios *)val;
		return 0;
	}
	return -EINVAL;
}
