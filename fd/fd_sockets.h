/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_sockets.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 15:16:24 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/26 19:08:07 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_H
#define SOCKET_H

#include "../common.h"
#include "fd.h"
#include "../waitq/waitq.h"

enum us_state
{
	US_UNBOUND,
	US_BOUND,
	US_LISTENING,
	US_CONNECTING,
	US_CONNECTED,
	US_CLOSED
};

#define SOCK_BUFF_SIZE 8196
#define SOCK_BACKLOG_MAX_MAX 128

#define AF_UNIX 1

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

#define UNIX_PATH_MAX 108

typedef struct sock_buf
{
	// ring buffer
	char buf[SOCK_BUFF_SIZE];
	uint32_t rpos, wpos;
	uint32_t used;
	t_waitq rwait; // readers wait for size>0
	t_waitq wwait; // writers wait for space>0
} t_sock_buf;

typedef struct unix_conn
{
	int refcnt;
	struct sock_buf a_to_b;
	struct sock_buf b_to_a;
	int closed_a; // A has shut down write
	int closed_b; // B has shut down write
} t_unix_conn;

typedef struct pending_conn
{
	struct unix_socket *client; // the active opener
	struct unix_conn *conn;		// already allocated buffers
	// optional: credentials, etc.
	struct pending_conn *next;
} t_pending_conn;

struct sockaddr_un
{
	uint16_t sun_family;
	char sun_path[UNIX_PATH_MAX];
};

typedef struct unix_socket
{
	int refcnt;
	int type; // SOCK_STREAM or SOCK_DGRAM
	enum us_state state;

	// binding
	struct vnode *bound_vnode; // points to socket inode entry for pathname binds
	char *bound_path;

	// listening
	int backlog_max;
	int backlog_len;
	struct pending_conn *pend_head, *pend_tail;
	t_waitq accept_wait;
	t_waitq connect_wait;

	// connected
	struct unix_conn *conn;
	int side; // 0 = A, 1 = B (which direction in unix_conn)
} t_unix_socket;

void unix_socket_put(t_unix_socket *s);
void unix_socket_get(t_unix_socket *s);
t_unix_socket *unix_socket_alloc(int type);
int unixsock_listen(t_unix_socket *s, int backlog);
int unixsock_connect(struct unix_socket *cl, struct unix_socket *srv, int flags);
int unixsock_accept(struct unix_socket *srv, struct unix_socket **out, int flags);

int32_t unixsock_read(t_file *f, void *buf, size_t n);
int32_t unixsock_write(t_file *f, const void *buf, size_t n);
int unixsock_close(t_file *f);

extern t_file_ops g_socket_ops;

#endif