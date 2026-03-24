/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_tty.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 16:17:51 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/20 04:19:53 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "fd_tty.h"
#include "../errno.h"
#include "../tasks/task.h"
#include "../libk/libk.h"
#include "../common.h"
#include "fd.h"

t_file_ops g_tty_ops = {.read = tty_read, .write = tty_write, .close = tty_close, .ioctl = tty_ioctl};

struct winsize
{
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel; /* unused */
	unsigned short ws_ypixel; /* unused */
};

#define KB_101 0x02

// IOCTLs
#define KDGKBTYPE 0x4B33
#define KDGKBENT 0x4B46
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410
#define TIOCGWINSZ 0x5413
#define TIOCGWINSZ 0x5413

struct kbentry {
	unsigned char  kb_table;
	unsigned char  kb_index;
	unsigned short kb_value;
};

int32_t tty_read(t_file *f, void *buf, size_t n)
{
	if (f && buf && n) {
		t_tty *tty = f->priv;
		if (g_curr_task->pgid != tty->fg_pgid)
		{
			print_debug("attemting to read on unowned terminal by pid %u\n", g_curr_task->task_id);
			int target_pgid = g_curr_task->pgid;
			t_task *curr = g_task_list;
			while (curr)
			{
				if ((int)curr->pgid == target_pgid)
				{
					enqueue_sig(curr, SIGTTIN);
				}
				curr = curr->next_all_task;
				if (curr == g_task_list) break;
			}
			return (-EINTR);
		}
		//if (!(f->flags & O_NONBLOCK))
		//{
			while (!tty->cmd.index && !tty->read_eof)
			{
				print_trace("tty_read: pid %u sleeping because no tty ready\n", g_curr_task->task_id);
				sleep_on(&tty->wait_read, WAIT_TTY_READ);
				print_trace("tty_read: pid %u woke up\n", g_curr_task->task_id);
				if (has_pending_signals(g_curr_task) && !(flags_first_pending_signal(g_curr_task) & SA_RESTART))
				{
					print_trace("tty_read: woken up by signal without SA_RESTART\n");
					if (tty->cmd.index || tty->read_eof) break;
					print_trace("tty_read: tty buffer empty, return -EINTR\n");
					return (-EINTR);
				}
			}
		//}
		if (tty->read_eof)
		{
			print_trace("read eof acknowledged in tty_read\n");
			tty->read_eof = false;
			if (!tty->cmd.index)
			 	return 0; //posix says that EOF is treated as newline if there's already a command
		}

		uint32_t to_copy = n < tty->cmd.index ? n : tty->cmd.index;
		memcpy(buf, tty->cmd.buffer, to_copy);
		memmove(tty->cmd.buffer, tty->cmd.buffer + to_copy, tty->cmd.index - to_copy);
		tty->cmd.index -= to_copy;
		print_trace("tty_read: about to return tocopy; %u\n", to_copy);
		if (tty->cmd.index)
			waitq_wake_one(&tty->wait_read); //wake another reader to finish the cmd
		if (!to_copy) return (-EAGAIN);
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
	if (g_curr_task->pgid != tty->fg_pgid)
	{
		print_trace("attemting to write on unowned terminal by pid %u\n", g_curr_task->task_id);
		int target_pgid = tty->fg_pgid;
		t_task *curr = g_task_list;
		while (curr)
		{
			if ((int)curr->pgid == target_pgid)
			{
				enqueue_sig(curr, SIGTTOU);
			}
			curr = curr->next_all_task;
			if (curr == g_task_list) break;
		}
		return (-EINTR);
	}
	write_main(buf, n);
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
	print_trace("tty_ioctl(op=0x%x, val=0x%x) pid=%u pgid=%u fg_pgid=%u\n",
        op, val, g_curr_task->task_id, g_curr_task->pgid, tty->fg_pgid);
	switch (op) {
	case KDGKBTYPE:
		*(int *)val = KB_101;
		return 0;
	case KDGKBENT:
		if (!user_range_ok((virt_ptr)val, sizeof(struct kbentry), true, &g_curr_task->proc_memory))
			return -EFAULT;
		struct kbentry *kbe = (struct kbentry *)val;
		print_trace("kdgkbent: tbl %u index %u\n", kbe->kb_table, kbe->kb_index);
		kbe->kb_value = 0;
		return 0;
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
		*(unsigned int *)val = tty->fg_pgid;
		print_trace("TIOCGPGRP -> %u\n", tty->fg_pgid);
		return 0;
	case TIOCSPGRP:
		if (!user_range_ok((virt_ptr)val, sizeof(unsigned int), false, &g_curr_task->proc_memory))
        return -EFAULT;
		uint32_t new = *(uint32_t *)val;
		print_trace("TIOCSPGRP <- %u\n", new);
		if (!pgid_exists(new))
			return (-ESRCH);
		tty->fg_pgid = new;
		return 0;
	case TCGETS:
		if (!user_range_ok((virt_ptr)val, sizeof(struct termios), true, &g_curr_task->proc_memory))
			return -EFAULT;
		*(struct termios *)val = tty->termios;
		return 0;
	case TCSETS:
	case TCSETSW: // drain output before applying
	case TCSETSF: // drain output before applying, flush input
		if (!user_range_ok((virt_ptr)val, sizeof(struct termios), false, &g_curr_task->proc_memory))
			return -EFAULT;
		tty->termios = *(struct termios *)val;
		return 0;
	}
	return -EINVAL;
}
