/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   inode.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 14:41:33 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:42:57 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INODE_H
#define INODE_H

#include "../common.h"
#include "fd.h"

typedef struct inode
{
	unsigned int inode_nr;
	unsigned int file_offset;
} t_inode;

int32_t inode_read(t_file *f, void *buf, size_t n);
int32_t inode_write(t_file *f, const void *buf, size_t n);
int32_t inode_close(t_file *f);

extern t_file_ops g_inode_ops;

#endif