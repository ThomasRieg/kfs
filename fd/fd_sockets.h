/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_sockets.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 15:16:24 by thrieg            #+#    #+#             */
/*   Updated: 2026/03/24 14:12:42 by thrieg           ###   ########.fr       */
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

#define SYS_SOCKET 1	  /* sys_socket(2)		*/
#define SYS_BIND 2		  /* sys_bind(2)			*/
#define SYS_CONNECT 3	  /* sys_connect(2)		*/
#define SYS_LISTEN 4	  /* sys_listen(2)		*/
#define SYS_ACCEPT 5	  /* sys_accept(2)		*/
#define SYS_GETSOCKNAME 6 /* sys_getsockname(2)		*/
#define SYS_GETPEERNAME 7 /* sys_getpeername(2)		*/
#define SYS_SOCKETPAIR 8  /* sys_socketpair(2)		*/
#define SYS_SEND 9		  /* sys_send(2)			*/
#define SYS_RECV 10		  /* sys_recv(2)			*/
#define SYS_SENDTO 11	  /* sys_sendto(2)		*/
#define SYS_RECVFROM 12	  /* sys_recvfrom(2)		*/
#define SYS_SHUTDOWN 13	  /* sys_shutdown(2)		*/
#define SYS_SETSOCKOPT 14 /* sys_setsockopt(2)		*/
#define SYS_GETSOCKOPT 15 /* sys_getsockopt(2)		*/
#define SYS_SENDMSG 16	  /* sys_sendmsg(2)		*/
#define SYS_RECVMSG 17	  /* sys_recvmsg(2)		*/
#define SYS_ACCEPT4 18	  /* sys_accept4(2)		*/
#define SYS_RECVMMSG 19	  /* sys_recvmmsg(2)		*/
#define SYS_SENDMMSG 20	  /* sys_sendmmsg(2)		*/

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

t_unix_conn *conn_alloc(void);
void conn_put(t_unix_conn *c);
void conn_get(t_unix_conn *c);

extern t_file_ops g_socket_ops;

#endif