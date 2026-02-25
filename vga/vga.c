/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vga.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:33:35 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/16 16:22:08 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../io.h"
#include "vga.h"
#include "../tty/tty.h"
#include "../libk/libk.h"

unsigned int g_vga_text_location = 0;
unsigned char *const g_vga_text_buf = (unsigned char *)0xb8000 + KERNEL_VIRT_BASE;
unsigned int g_vga_text_color = VGA_WHITE;

void update_cursor(int pos)
{
	outb(PORT_VGA_INDEX, 0x0F);
	outb(PORT_VGA_INDEXED, (unsigned char)(pos & 0xFF));
	outb(PORT_VGA_INDEX, 0x0E);
	outb(PORT_VGA_INDEXED, (unsigned char)((pos >> 8) & 0xFF));
}

void vga_set_color(uint8_t foreground, uint8_t background)
{
	g_vga_text_color = (foreground & 0b00001111) + (background << 4);
}

void vga_get_color(uint8_t *foreground, uint8_t *background)
{
	*foreground = g_vga_text_color & 0b00001111;
	*background = g_vga_text_color >> 4;
}

static void clear_last_line(void)
{
	unsigned int start = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
	for (unsigned int i = 0; i < VGA_WIDTH; i++)
	{
		g_vga_text_buf[start + i * 2] = ' ';
		g_vga_text_buf[start + i * 2 + 1] = VGA_WHITE;
	}
}

void scroll_down()
{
	for (unsigned int i = VGA_WIDTH * 2; i < VGA_SIZE; i++)
	{
		g_vga_text_buf[i - VGA_WIDTH * 2] = g_vga_text_buf[i];
	}
	clear_last_line();
	g_vga_text_location = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
}

unsigned short ctrl_seq_i = 0;

static inline void vga_add_char(char c)
{
	if (c == '\e' && ctrl_seq_i == 0) {
		ctrl_seq_i++;
		return;
	} else if (ctrl_seq_i == 1) {
		if (c == '[')
			ctrl_seq_i++;
		else
			ctrl_seq_i = 0;
		return;
	}
	else if (ctrl_seq_i == 2) {
		if (c == 'J') {
			for (int i = g_vga_text_location; i < VGA_SIZE; i += 2) {
				g_vga_text_buf[i] = ' ';
			}
		} else if (c == 'H') {
			g_vga_text_location = 0;
		}
		ctrl_seq_i = 0;
		if (c != 'J' && c != 'H') {
			print_err("unknown ANSI char: %c\n", c);
		}
		return;
	}

	if (c == '\t')
		c = ' ';

	unsigned short col = g_vga_text_location % (VGA_WIDTH * 2);
	if (c == '\b' && col != 0)
		g_vga_text_location -= 2;
	else if (c == 0x7f && col != 0) {
		g_vga_text_location -= 2;
		g_vga_text_buf[g_vga_text_location] = ' ';
	}
	else if (c == '\r')
		g_vga_text_location -= g_vga_text_location % (VGA_WIDTH * 2);
	else if (c == '\n')
		g_vga_text_location += (VGA_WIDTH * 2);
	else
	{
		g_vga_text_buf[g_vga_text_location] = c;
		g_vga_text_buf[g_vga_text_location + 1] = g_vga_text_color;
		g_vga_text_location += 2;
	}
	if (g_vga_text_location + 1 >= VGA_SIZE)
		scroll_down();
}

void vga_write(t_tty *tty, const char *str, unsigned int n)
{
	if (g_vga_text_location + 1 >= VGA_SIZE)
		scroll_down();
	for (unsigned int i = 0; i < n; i++)
	{
		if (str[i] == '\n' && tty->termios.c_oflag & ONLCR) {
			vga_add_char('\r');
			vga_add_char('\n');
		} else
			vga_add_char(str[i]);
	}
	update_cursor(g_vga_text_location / 2);
}

void vga_writes(t_tty *tty, const char *str)
{
	if (g_vga_text_location + 1 >= VGA_SIZE)
		scroll_down();
	while (*str)
	{
		if (*str == '\n' && tty->termios.c_oflag & ONLCR) {
			vga_add_char('\r');
			vga_add_char('\n');
		} else
			vga_add_char(*str);
		str++;
	}
	update_cursor(g_vga_text_location / 2);
}

void vga_clear_screen()
{
	for (unsigned int i = 0; i < VGA_SIZE - 1; i += 2)
	{
		g_vga_text_buf[i] = 0;
		g_vga_text_buf[i + 1] = VGA_WHITE; // white foreground for cursor
	}
	g_vga_text_location = 0;
}
