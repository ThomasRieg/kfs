/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fd_sockets.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 15:16:13 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/26 19:08:14 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "fd_sockets.h"
#include "../errno.h"
#include "../vmalloc/vmalloc.h"
#include "../libk/libk.h"

t_file_ops g_socket_ops = {.read = unixsock_read, .write = unixsock_write, .close = unixsock_close};

static t_unix_conn *conn_alloc(void)
{
	t_unix_conn *c = (t_unix_conn *)vcalloc(1, sizeof(*c));
	if (!c)
		return (NULL);
	c->refcnt = 1;
	if (!c->a_to_b.buf || !c->b_to_a.buf)
	{
		vfree(c);
		return (NULL);
	}
	return c;
}

static void conn_free(t_unix_conn *c)
{
	vfree(c);
}

static void conn_get(t_unix_conn *c)
{
	if (c)
		c->refcnt++;
}

static void conn_put(t_unix_conn *c)
{
	if (!c)
		return;
	if (--c->refcnt == 0)
	{
		vfree(c);
	}
}

t_unix_socket *unix_socket_alloc(int type)
{
	t_unix_socket *s = (t_unix_socket *)vcalloc(1, sizeof(*s));
	if (!s)
		return (NULL);
	s->refcnt = 1;
	s->type = type;
	s->state = US_UNBOUND;
	return (s);
}

void unix_socket_get(t_unix_socket *s)
{
	if (s)
		s->refcnt++;
}

void unix_socket_put(t_unix_socket *s)
{
	if (!s)
		return;
	if (--s->refcnt == 0)
	{
		if (s->conn)
			conn_put(s->conn);
		if (s->bound_path)
			vfree(s->bound_path);

		/* if LISTENING: drain pending queue */
		while (s->pend_head)
		{
			t_pending_conn *p = s->pend_head;
			s->pend_head = p->next;
			/* abort pending connect */
			if (p->client)
			{
				p->client->state = US_CLOSED;
				waitq_wake_all(&p->client->connect_wait);
			}
			conn_put(p->conn);
			vfree(p);
		}
		s->pend_tail = NULL;
		s->backlog_len = 0;

		vfree(s);
	}
}

static void queue_push(t_unix_socket *srv, t_pending_conn *p)
{
	p->next = NULL;
	if (!srv->pend_tail)
	{
		srv->pend_head = srv->pend_tail = p;
	}
	else
	{
		srv->pend_tail->next = p;
		srv->pend_tail = p;
	}
	srv->backlog_len++;
}

static t_pending_conn *queue_pop(t_unix_socket *srv)
{
	t_pending_conn *p = srv->pend_head;
	if (!p)
		return NULL;
	srv->pend_head = p->next;
	if (!srv->pend_head)
		srv->pend_tail = NULL;
	p->next = NULL;
	srv->backlog_len--;
	return p;
}

int unixsock_listen(t_unix_socket *s, int backlog)
{
	if (!s)
		return (-EINVAL);
	if (s->type != SOCK_STREAM)
		return (-EOPNOTSUPP);
	if (s->state != US_BOUND)
		return (-EINVAL);

	if (backlog < 0)
		backlog = 0;
	if (backlog > SOCK_BACKLOG_MAX_MAX)
		backlog = SOCK_BACKLOG_MAX_MAX; /* cap */
	s->backlog_max = backlog;
	s->backlog_len = 0;
	s->pend_head = s->pend_tail = NULL;
	s->state = US_LISTENING;
	return (0);
}

int unixsock_connect(struct unix_socket *cl, struct unix_socket *srv, int flags)
{
	if (srv->state != US_LISTENING)
		return (-ECONNREFUSED);
	if (cl->state != US_UNBOUND && cl->state != US_BOUND)
		return (-EINVAL);

	struct unix_conn *c = conn_alloc();
	if (!c)
		return (-ENOMEM);
	struct pending_conn *p = vmalloc(sizeof *p);
	if (!p)
		return (conn_free(c), -ENOMEM);
	p->client = cl;
	p->conn = c;
	p->next = NULL;

	// enqueue
	// spin_lock(&srv->lock);
	if (srv->backlog_len >= srv->backlog_max)
	{
		// spin_unlock(&srv->lock);
		conn_put(c);
		vfree(p);
		return (-ECONNREFUSED);
	}
	queue_push(srv, p);

	cl->state = US_CONNECTING;
	cl->conn = c; // hold ref
	cl->side = 0;

	waitq_wake_all(&srv->accept_wait);

	if (flags & O_NONBLOCK)
		return (-EINPROGRESS);

	// sleep until accept flips to CONNECTED or error/close
	sleep_on(&cl->connect_wait, WAIT_SOCK_CONNECT);
	if (cl->state == US_CONNECTED)
		return 0;
	return (-ECONNABORTED);
}

//*out is malloced
int unixsock_accept(struct unix_socket *srv, struct unix_socket **out, int flags)
{
	if (srv->state != US_LISTENING)
		return (-EINVAL);

	// spin_lock(&srv->lock);
	while (srv->backlog_len == 0)
	{
		if (flags & O_NONBLOCK)
		{
			// spin_unlock(&srv->lock);
			return (-EAGAIN);
		}
		// spin_unlock(&srv->lock);
		sleep_on(&srv->accept_wait, WAIT_SOCK_ACCEPT);
		// spin_lock(&srv->lock);
	}

	struct pending_conn *p = queue_pop(srv);
	if (!p)
		// kernel_panic("incoherent kernel state in unixsock_accept, empty queue\n", NULL);
		return (-EAGAIN);
	// spin_unlock(&srv->lock);

	struct unix_socket *acc = unix_socket_alloc(SOCK_STREAM);
	if (!acc)
	{
		/* abort the pending connect cleanly */
		p->client->state = US_CLOSED;
		waitq_wake_all(&p->client->connect_wait);
		conn_put(p->conn);
		vfree(p);
		return (-ENOMEM);
	}
	acc->state = US_CONNECTED;
	acc->conn = p->conn; // take ref
	conn_get(acc->conn);
	acc->side = 1;

	// finalize client
	p->client->state = US_CONNECTED;
	// already has conn pointer and side=0
	waitq_wake_all(&p->client->connect_wait);

	*out = acc;
	conn_put(p->conn);
	vfree(p);
	return (0);
}

/*#include "../ext2.h"
t_unix_socket *bind(const char *path, int *err, unsigned int relative_dir_inode_nr)
{
	unsigned int inode = path_to_inode(path, relative_dir_inode_nr);
	if (inode)
		return (*err = EADDRINUSE, NULL);

}*/

static int peer_shutdown_write(t_unix_socket *s)
{
	if (s->side == 0)
		return s->conn->closed_b;
	return s->conn->closed_a;
}

/* You can expand later to handle full shutdown/read-close semantics */
static int peer_gone(t_unix_socket *s)
{
	(void)s;
	return 0;
}

static inline struct sock_buf *rx_buf(struct unix_socket *s)
{
	return (s->side == 0) ? &s->conn->b_to_a : &s->conn->a_to_b;
}
static inline struct sock_buf *tx_buf(struct unix_socket *s)
{
	return (s->side == 0) ? &s->conn->a_to_b : &s->conn->b_to_a;
}

static uint32_t socket_read_ring(struct sock_buf *p, uint8_t *dst, uint32_t n)
{
	uint32_t total = 0;
	while (total < n && p->used)
	{
		uint32_t chunk = n - total;
		uint32_t until_end = SOCK_BUFF_SIZE - p->rpos;
		if (chunk > until_end)
			chunk = until_end;
		if (chunk > p->used)
			chunk = p->used;

		memcpy(dst + total, p->buf + p->rpos, chunk);
		p->rpos = (p->rpos + chunk) % SOCK_BUFF_SIZE;
		p->used -= chunk;
		total += chunk;
	}
	return total;
}

static inline uint32_t pipe_space(struct sock_buf *p) { return sizeof(p->buf) - p->used; }

static uint32_t socket_write_ring(struct sock_buf *p, const uint8_t *src, uint32_t n)
{
	uint32_t total = 0;
	while (total < n && pipe_space(p))
	{
		uint32_t chunk = n - total;
		uint32_t until_end = SOCK_BUFF_SIZE - p->wpos;
		uint32_t space = pipe_space(p);

		if (chunk > until_end)
			chunk = until_end;
		if (chunk > space)
			chunk = space;

		memcpy(p->buf + p->wpos, src + total, chunk);
		p->wpos = (p->wpos + chunk) % SOCK_BUFF_SIZE;
		p->used += chunk;
		total += chunk;
	}
	return total;
}

int32_t unixsock_read(t_file *f, void *buf, size_t n)
{
	struct unix_socket *s = f->priv;
	if (s->state != US_CONNECTED)
		return (-ENOTCONN);

	struct sock_buf *rx = rx_buf(s);

	for (;;)
	{
		// spin_lock(&rx->lock);
		if (rx->used > 0)
		{
			size_t r = socket_read_ring(rx, buf, n);
			// spin_unlock(&rx->lock);
			waitq_wake_one(&rx->wwait);
			return ((int32_t)r);
		}
		// empty: check peer shutdown -> EOF
		if (peer_shutdown_write(s))
		{
			// spin_unlock(&rx->lock);
			return (0);
		}
		if (f->flags & O_NONBLOCK)
		{
			// spin_unlock(&rx->lock);
			return (-EAGAIN);
		}
		// spin_unlock(&rx->lock);
		sleep_on(&rx->rwait, WAIT_SOCK_READ);
	}
}

int32_t unixsock_write(t_file *f, const void *buf, size_t n)
{
	struct unix_socket *s = f->priv;
	if (s->state != US_CONNECTED)
		return (-ENOTCONN);
	if (peer_gone(s))
		return (-EPIPE);

	struct sock_buf *tx = tx_buf(s);

	size_t written = 0;
	while (written < n)
	{
		// spin_lock(&tx->lock);
		size_t w = socket_write_ring(tx, (((const uint8_t *)buf) + written), n - written);
		if (w)
		{
			written += w;
			// spin_unlock(&tx->lock);
			waitq_wake_all(&tx->rwait);
			continue;
		}
		if (f->flags & O_NONBLOCK)
		{
			// spin_unlock(&tx->lock);
			return written ? ((int32_t)written) : (-EAGAIN);
		}
		// spin_unlock(&tx->lock);
		sleep_on(&tx->wwait, WAIT_SOCK_WRITE);
	}
	return ((int32_t)written);
}

int unixsock_close(t_file *f)
{
	t_unix_socket *s = (t_unix_socket *)f->priv;
	if (s)
		unix_socket_put(s);
	f->priv = NULL;
	return (0);
}
