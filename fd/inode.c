/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   inode.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 15:36:46 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:43:02 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inode.h"
#include "../errno.h"
#include "../ext2.h"

t_file_ops g_inode_ops = {.read = inode_read, .write = inode_write, .close = inode_close};

int32_t inode_read(t_file *f, void *buf, size_t n)
{
	t_inode *inode = (t_inode *)f->priv;
	int written = read_inode(inode->inode_nr, inode->file_offset, buf, n);
	if (written > 0)
		inode->file_offset += written;
	return (written);
}

int32_t inode_write(t_file *f, const void *buf, size_t n)
{
	return n;
	if (f && buf && n)
		return (-ENOSYS);
	return (-ENOSYS);
}

// vfree inode if no refcount left
// doesn't free file, assumed to not be dynamically allocated
int32_t inode_close(t_file *f)
{
	if (f)
		return (-ENOSYS);
	return (-ENOSYS);
}
