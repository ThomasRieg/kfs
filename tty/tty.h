/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tty.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 03:10:42 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/15 15:22:24 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TTY_H
#define TTY_H

#define TTY_MAX 10

#include "../vga/vga.h"
#include "../libk/ft_vector.h"

// https://elixir.bootlin.com/linux/v6.18.6/source/include/uapi/asm-generic/termbits.h
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

/* c_lflag bits */
#define ECHO	0x00008

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
	bool read_eof;
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
