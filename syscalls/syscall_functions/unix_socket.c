/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   unix_socket.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 18:25:58 by thrieg            #+#    #+#             */
/*   Updated: 2026/03/24 14:31:02 by thrieg           ###   ########.fr       */
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

uint32_t do_socket(int domain, int type, int proto, int flags)
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
uint32_t do_bind(int fd, const struct sockaddr_un *uaddr, uint32_t addrlen)
{
	/*if (!user_range_ok(uaddr, addrlen, 0, g_curr_task->proc_memory))
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

uint32_t do_listen(int fd, int backlog)
{
	t_unix_socket *s = fd_get_unix_socket(fd);
	if (!s)
		return (-ENOTSOCK);
	return (unixsock_listen(s, backlog));
}

uint32_t do_connect(int fd, const struct sockaddr_un *uaddr, uint32_t addrlen)
{
	if (!user_range_ok((const virt_ptr)uaddr, addrlen, 0, g_curr_task->proc_memory))
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

uint32_t do_accept(int fd, int flags)
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

uint32_t do_socketpair(int domain, int type, int proto, int *sv)
{
	(void)proto;

	// validate args
	if (domain != AF_UNIX)
		return (-EAFNOSUPPORT);
	if (type != SOCK_STREAM)
		return (-EOPNOTSUPP);

	if (!user_range_ok(sv, 2 * sizeof(int), 1, g_curr_task->proc_memory))
		return (-EFAULT);

	// find 2 free fds
	int i, j;
	for (i = 0; i < (int)MAX_OPEN_FILES; i++)
	{
		if (!g_curr_task->open_files[i])
			break;
	}
	for (j = i + 1; j < (int)MAX_OPEN_FILES; j++)
	{
		if (!g_curr_task->open_files[j])
			break;
	}
	if (j >= (int)MAX_OPEN_FILES)
		return (-EMFILE);

	// allocate connection (shared)
	t_unix_conn *conn = conn_alloc();
	if (!conn)
		return (-ENOMEM);

	// allocate sockets
	t_unix_socket *a = unix_socket_alloc(SOCK_STREAM);
	if (!a)
		return (conn_put(conn), -ENOMEM);

	t_unix_socket *b = unix_socket_alloc(SOCK_STREAM);
	if (!b)
		return (conn_put(conn), unix_socket_put(a), -ENOMEM);

	// setup connected state
	a->state = US_CONNECTED;
	a->conn = conn;
	a->side = 0;

	b->state = US_CONNECTED;
	b->conn = conn;
	b->side = 1;

	// conn is now owned by both sockets
	conn_get(conn); // second ref for second endpoint

	// create file objects
	t_file *fa = alloc_socket_file(a, O_RDWR);
	if (!fa)
		return (conn_put(conn), unix_socket_put(a), unix_socket_put(b), -ENOMEM);

	t_file *fb = alloc_socket_file(b, O_RDWR);
	if (!fb)
		return (conn_put(conn), unix_socket_put(a), unix_socket_put(b), vfree(fa), -ENOMEM);

	// drop extra socket refs (files now own them)
	unix_socket_put(a);
	unix_socket_put(b);

	// put in fd list
	g_curr_task->open_files[i] = fa;
	g_curr_task->open_files[j] = fb;

	sv[0] = i;
	sv[1] = j;

	return (0);
}

uint32_t syscall_socket(t_interrupt_data *regs)
{
	int domain = (int)regs->ebx;
	int type = (int)regs->ecx;
	int proto = (int)regs->edx;
	print_trace("socket %d %d %d\n", domain, type, proto);
	return do_socket(domain, type, proto, 0);
}

// 360
uint32_t syscall_socketpair(t_interrupt_data *regs)
{
	int domain = (int)regs->ebx;
	int type = (int)regs->ecx;
	int proto = (int)regs->edx;
	int *sv = (int *)regs->esi;

	return do_socketpair(domain, type, proto, sv);
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

// 102
uint32_t syscall_socketcall(t_interrupt_data *regs)
{
	int call = regs->ebx;
	unsigned long *args = (unsigned long *)regs->ecx;
	uint32_t err = 0;
	switch (call)
	{
	case SYS_SOCKET:
		if (!user_range_ok(args, 4 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_socket(args[0], args[1], args[2], args[3]);
		break;
	case SYS_BIND:
		if (!user_range_ok(args, 3 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_bind(args[0], (const struct sockaddr_un *)args[1], args[2]);
		break;
	case SYS_CONNECT:
		if (!user_range_ok(args, 3 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_connect(args[0], (const struct sockaddr_un *)args[1], args[2]);
		break;
	case SYS_LISTEN:
		if (!user_range_ok(args, 2 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_listen(args[0], args[1]);
		break;
	case SYS_ACCEPT:
		if (!user_range_ok(args, 2 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_accept(args[0], args[1]);
		break;
	case SYS_SOCKETPAIR:
		if (!user_range_ok(args, 4 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_socketpair(args[0], args[1], args[2], (int *)args[3]);
		break;
	case SYS_ACCEPT4:
		if (!user_range_ok(args, 2 * sizeof(args), false, g_curr_task->proc_memory))
			return (-ERANGE);
		err = do_accept(args[0], args[1]);
		break;
	default:
		err = -ENOSYS;
		print_err("socketcall called with unhandled call number %d by pid %u\n", call, g_curr_task->task_id);
		break;
	}
	return (err);
}
