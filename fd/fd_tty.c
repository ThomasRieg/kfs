/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_tty.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 16:17:51 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:41:54 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "fd_tty.h"
#include "../errno.h"
#include "../tasks/task.h"
#include "../libk/libk.h"

t_file_ops g_tty_ops = {.read = tty_read, .write = tty_write, .close = tty_close};

int32_t tty_read(t_file *f, void *buf, size_t n)
{
	if (f && buf && n) {
		t_tty *tty = f->priv;
		if (!tty->cmd.index) {
			do {
				yield();
				//printk("yielding because no tty ready\n");
			} while (!tty->cmd.index);
		}

		tty->cmd.index--;
		*(unsigned char *)buf = tty->cmd.buffer[0];
		return 1;
	}
	return (-ENOSYS);
}

int32_t tty_write(t_file *f, const void *buf, size_t n)
{
	if (f)
		write(buf, n);
	return (n);
}

int32_t tty_close(t_file *f)
{
	if (f)
		return (-ENOSYS);
	return (-ENOSYS);
}
