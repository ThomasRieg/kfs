/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   unix_socket.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 18:25:58 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/26 19:31:57 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../fd/fd_sockets.h"
#include "../../tasks/task.h"
#include "../../errno.h"

static size_t k_strnlen(const char *s, size_t max)
{
	size_t i = 0;
	while (i < max && s[i])
		i++;
	return i;
}

static t_file *alloc_socket_file(t_unix_socket *s, int oflags)
{
	t_file *f = (t_file *)vcalloc(1, sizeof(*f));
	if (!f)
		return NULL;

	f->flags = oflags;
	f->ops = &g_socket_ops;
	f->pos = 0;
	f->priv = s;
	f->refcnt = 1;
	f->type = FILE_SOCKET;

	unix_socket_get(s); /* file holds a ref */
	return f;
}

static t_unix_socket *fd_get_unix_socket(int fd)
{
	if (fd < 0 || (uint32_t)fd >= MAX_OPEN_FILES)
		return (NULL);
	t_file *f = g_curr_task->open_files[fd];
	if (!f || f->type != FILE_SOCKET)
		return (NULL);
	return ((t_unix_socket *)f->priv);
}

static uint32_t do_socket(int domain, int type, int proto, int flags)
{
	(void)proto;

	if (domain != AF_UNIX)
		return (-EAFNOSUPPORT);
	if (type != SOCK_STREAM)
		return -EOPNOTSUPP;

	uint32_t fd;
	for (fd = 0; fd < MAX_OPEN_FILES; fd++)
	{
		if (!g_curr_task->open_files[fd])
			break;
	}
	if (fd >= MAX_OPEN_FILES)
		return (-EMFILE);

	t_unix_socket *s = unix_socket_alloc(type);
	if (!s)
		return (-ENOMEM);

	t_file *f = alloc_socket_file(s, O_RDWR | flags);
	unix_socket_put(s); // file keeps the ref; drop temporary
	if (!f)
		return (-ENOMEM);
	f->flags = flags;

	g_curr_task->open_files[fd] = f;
	return (fd);
}

// TODO implement
static uint32_t do_bind(int fd, const struct sockaddr_un *uaddr, uint32_t addrlen)
{
	/*if (!user_range_ok(uaddr, addrlen, 0, &g_curr_task->proc_memory))
		return (-EFAULT);
	if (addrlen < sizeof(uint16_t))
		return (-EINVAL);

	t_unix_socket *s = fd_get_unix_socket(fd);
	if (!s)
		return (-ENOTSOCK);

	struct sockaddr_un sa;
	const uint8_t *src = (const uint8_t *)uaddr;
	uint8_t *dst = (uint8_t *)&sa;
	for (uint32_t i = 0; i < (uint32_t)sizeof(sa) && i < addrlen; i++)
		dst[i] = src[i];

	if (sa.sun_family != AF_UNIX)
		return (-EAFNOSUPPORT);

	size_t max = (addrlen > offsetof(struct sockaddr_un, sun_path))
					 ? (addrlen - offsetof(struct sockaddr_un, sun_path))
					 : 0;
	if (max > UNIX_PATH_MAX)
		max = UNIX_PATH_MAX;

	size_t plen = k_strnlen(sa.sun_path, max);
	if (plen == 0)
		return -EINVAL;

	return (uint32_t)unix_bind_path(s, sa.sun_path, plen);*/
	if (fd && uaddr && addrlen)
		return (-ENOSYS);
	return (-ENOSYS);
}

static uint32_t do_listen(int fd, int backlog)
{
	t_unix_socket *s = fd_get_unix_socket(fd);
	if (!s)
		return (-ENOTSOCK);
	return (unixsock_listen(s, backlog));
}

static uint32_t do_connect(int fd, const struct sockaddr_un *uaddr, uint32_t addrlen)
{
	if (!user_range_ok((const virt_ptr)uaddr, addrlen, 0, &g_curr_task->proc_memory))
		return (-EFAULT);

	t_unix_socket *cl = fd_get_unix_socket(fd);
	if (!cl)
		return (-ENOTSOCK);

	struct sockaddr_un sa;
	const uint8_t *src = (const uint8_t *)uaddr;
	uint8_t *dst = (uint8_t *)&sa;
	for (uint32_t i = 0; i < (uint32_t)sizeof(sa) && i < addrlen; i++)
		dst[i] = src[i];

	if (sa.sun_family != AF_UNIX)
		return (-EAFNOSUPPORT);

	size_t max = (addrlen > offsetof(struct sockaddr_un, sun_path))
					 ? (addrlen - offsetof(struct sockaddr_un, sun_path))
					 : 0;
	if (max > UNIX_PATH_MAX)
		max = UNIX_PATH_MAX;

	size_t plen = k_strnlen(sa.sun_path, max);
	if (plen == 0)
		return (-EINVAL);

	int flags = g_curr_task->open_files[fd]->flags;
	/*unsigned int inode = path_to_inode(path, relative_dir_inode_nr);
	if (!inode)
		return (-ENOTSOCK);*/
	t_unix_socket *srv = NULL; // TODO VFS lookup here from path
	return (uint32_t)unixsock_connect(cl, srv, flags);
}

static uint32_t do_accept(int fd, int flags)
{
	t_unix_socket *srv = fd_get_unix_socket(fd);
	if (!srv)
		return (-ENOTSOCK);

	t_unix_socket *acc = NULL;
	int r = unixsock_accept(srv, &acc, flags);
	if (r < 0)
		return (r);

	uint32_t new_fd;
	for (new_fd = 0; new_fd < MAX_OPEN_FILES; new_fd++)
	{
		if (!g_curr_task->open_files[new_fd])
			break;
	}
	if (new_fd >= MAX_OPEN_FILES)
		return (-EMFILE);

	t_file *f = alloc_socket_file(acc, O_RDWR | (flags & O_NONBLOCK));
	unix_socket_put(acc);
	if (!f)
		return (-ENOMEM);

	g_curr_task->open_files[new_fd] = f;
	return ((uint32_t)new_fd);
}

uint32_t syscall_socket(t_interrupt_data *regs)
{
	int domain = (int)regs->ebx;
	int type = (int)regs->ecx;
	int proto = (int)regs->edx;
	print_trace("socket %d %d %d\n", domain, type, proto);
	return do_socket(domain, type, proto, 0);
}

uint32_t syscall_bind(t_interrupt_data *regs)
{
	int fd = (int)regs->ebx;
	struct sockaddr_un *uaddr = (struct sockaddr_un *)regs->ecx;
	uint32_t addrlen = regs->edx;
	print_trace("bind %d %p %u\n", fd, uaddr, addrlen);
	return do_bind(fd, uaddr, addrlen);
}

uint32_t syscall_listen(t_interrupt_data *regs)
{
	int fd = (int)regs->ebx;
	int backlog = (int)regs->ecx;
	print_trace("listen %d %d\n", fd, backlog);
	return do_listen(fd, backlog);
}

uint32_t syscall_connect(t_interrupt_data *regs)
{
	int fd = (int)regs->ebx;
	struct sockaddr_un *uaddr = (struct sockaddr_un *)regs->ecx;
	uint32_t addrlen = regs->edx;
	print_trace("connect %d %p %u\n", fd, uaddr, addrlen);
	return do_connect(fd, uaddr, addrlen);
}

uint32_t syscall_accept4(t_interrupt_data *regs)
{
	int fd = (int)regs->ebx;
	int flags = (int)regs->ecx;
	print_trace("accept4 %d %d\n", fd, flags);
	return do_accept(fd, flags);
}
