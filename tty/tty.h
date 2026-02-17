/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tty.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 03:10:42 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/17 01:46:02 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TTY_H
#define TTY_H

#define TTY_MAX 10

#include "../vga/vga.h"
#include "../libk/ft_vector.h"
#include "../waitq/waitq.h"

// https://elixir.bootlin.com/linux/v6.18.6/source/include/uapi/asm-generic/termbits.h
// https://elixir.bootlin.com/linux/v6.18.6/source/include/uapi/asm-generic/termbits-common.h
#define NCCS 19
typedef unsigned char cc_t;
typedef unsigned int tcflag_t;

/* c_cc characters */
#define VINTR		 0
#define VQUIT		 1
#define VERASE		 2
#define VKILL		 3
#define VEOF		 4
#define VTIME		 5
#define VMIN		 6
#define VSWTC		 7
#define VSTART		 8
#define VSTOP		 9
#define VSUSP		10
#define VEOL		11
#define VREPRINT	12
#define VDISCARD	13
#define VWERASE		14
#define VLNEXT		15
#define VEOL2		16

/* c_iflag bits */
#define ICRNL	0x100			/* Map CR to NL on input */
#define IUCLC	0x0200
#define IXON	0x0400
#define IXOFF	0x1000
#define IMAXBEL	0x2000
#define IUTF8	0x4000

/* c_oflag bits */
#define OPOST	0x01			/* Perform output processing */
#define OLCUC	0x00002
#define ONLCR	0x00004
#define OCRNL	0x08
#define NLDLY	0x00100
#define   NL0	0x00000
#define   NL1	0x00100
#define CRDLY	0x00600
#define   CR0	0x00000
#define   CR1	0x00200
#define   CR2	0x00400
#define   CR3	0x00600
#define TABDLY	0x01800
#define   TAB0	0x00000
#define   TAB1	0x00800
#define   TAB2	0x01000
#define   TAB3	0x01800
#define   XTABS	0x01800
#define BSDLY	0x02000
#define   BS0	0x00000
#define   BS1	0x02000
#define VTDLY	0x04000
#define   VT0	0x00000
#define   VT1	0x04000
#define FFDLY	0x08000
#define   FF0	0x00000
#define   FF1	0x08000

/* c_cflag bit meaning */
#define CBAUD		0x0000100f
#define CSIZE		0x00000030
#define   CS5		0x00000000
#define   CS6		0x00000010
#define   CS7		0x00000020
#define   CS8		0x00000030
#define CSTOPB		0x00000040
#define CREAD		0x00000080
#define PARENB		0x00000100
#define PARODD		0x00000200
#define HUPCL		0x00000400
#define CLOCAL		0x00000800
#define CBAUDEX		0x00001000
#define BOTHER		0x00001000
#define B38400		0x0000000f
#define     B57600	0x00001001
#define    B115200	0x00001002
#define    B230400	0x00001003
#define    B460800	0x00001004
#define    B500000	0x00001005
#define    B576000	0x00001006
#define    B921600	0x00001007
#define   B1000000	0x00001008
#define   B1152000	0x00001009
#define   B1500000	0x0000100a
#define   B2000000	0x0000100b
#define   B2500000	0x0000100c
#define   B3000000	0x0000100d
#define   B3500000	0x0000100e
#define   B4000000	0x0000100f
#define CIBAUD		0x100f0000	/* input baud rate */

/* c_lflag bits */
#define ISIG	0x00001
#define ICANON	0x00002
#define XCASE	0x00004
#define ECHO	0x00008
#define ECHOE	0x00010
#define ECHOK	0x00020
#define ECHONL	0x00040
#define NOFLSH	0x00080
#define TOSTOP	0x00100
#define ECHOCTL	0x00200
#define ECHOPRT	0x00400
#define ECHOKE	0x00800
#define FLUSHO	0x01000
#define PENDIN	0x04000
#define IEXTEN	0x08000
#define EXTPROC	0x10000

struct termios {
	tcflag_t c_iflag;		/* input mode flags */
	tcflag_t c_oflag;		/* output mode flags */
	tcflag_t c_cflag;		/* control mode flags */
	tcflag_t c_lflag;		/* local mode flags */
	cc_t c_line;			/* line discipline */
	cc_t c_cc[NCCS];		/* control characters */
};

typedef struct s_tty
{
	unsigned char buf[VGA_SIZE + 1]; // full screen snapshot, TODO make this infinitely growing after kfs-3
	unsigned int cursor;			 // = vga_text_location (treat it as nb chars in buf)
	t_vector cmd;
	struct termios termios;
	t_waitq wait_read; //wait inside read, so wake up these when you write something
	t_waitq wait_write; //wait inside write, so wake up these when you write something
	bool read_eof;
	uint32_t fg_pgid;        // foreground process group
    uint32_t session_sid;    // session leader
} t_tty;

// not sure if I actually want them extern or if I want them static, if I only access it from tty.c they should be static but for now extern is easier
extern t_tty g_ttys[TTY_MAX]; // TODO make this infinitely growing after kfs-3 maybe
extern unsigned int g_active_tty;
extern unsigned int g_current_tty;

// save the vga buffer into the tty buffer
void save_tty();
// silently does nothing if you can't add a tty, just stay on the last one
void next_tty();
// silently does nothing if you are on the first tty, just stay on the first one
void prev_tty();
void tty_add_input(char c);

void write(const char *str, unsigned int n);

void writes(const char *str);

#endif
