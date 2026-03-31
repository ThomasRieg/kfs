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

static int32_t inode_llseek(t_file *file, unsigned long long int offset, unsigned long long int *result, unsigned int whence) {
	t_inode *inode = (t_inode *)file->priv;
	switch (whence) {
	case SEEK_SET:
		inode->file_offset = offset;
		break;
	case SEEK_CUR:
		inode->file_offset += offset;
		break;
	case SEEK_END:
		;struct stat stat;
		int status = stat_inode(inode->inode_nr, &stat);
		if (status < 0)
			return status;
		inode->file_offset = stat.st_size + offset;
		break;
	}
	*result = inode->file_offset;
	return 0;
}

t_file_ops g_inode_ops = {.read = inode_read, .write = inode_write, .close = inode_close, .llseek = inode_llseek};

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
	t_inode *inode = (t_inode *)f->priv;
	if (f->flags & O_APPEND) {
		struct stat stat;
		int status = stat_inode(inode->inode_nr, &stat);
		if (status < 0)
			return status;
		inode->file_offset = stat.st_size;
	}
	int written = ext2_write_inode_contents(inode->inode_nr, inode->file_offset, buf, n);
	if (written > 0)
		inode->file_offset += written;
	return written;
}

// vfree inode if no refcount left
// doesn't free file, assumed to not be dynamically allocated
int32_t inode_close(t_file *f)
{
	if (f)
		return (-ENOSYS);
	return (-ENOSYS);
}
