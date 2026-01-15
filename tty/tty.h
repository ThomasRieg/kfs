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

typedef struct s_tty
{
	unsigned char buf[VGA_SIZE + 1]; // full screen snapshot, TODO make this infinitely growing after kfs-3
	unsigned int cursor;			 // = vga_text_location (treat it as nb chars in buf)
	t_vector cmd;
	unsigned char attr; // for futur use maybe
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
