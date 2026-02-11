/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_tty.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 16:09:55 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:42:05 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FD_TTY_H
#define FD_TTY_H

#include "../common.h"
#include "fd.h"
#include "../tty/tty.h"

int32_t tty_read(t_file *f, void *buf, size_t n);
int32_t tty_write(t_file *f, const void *buf, size_t n);
int32_t tty_close(t_file *f);
int32_t tty_ioctl(t_file *f, unsigned int op, unsigned int val);

extern t_file_ops g_tty_ops;

#endif
