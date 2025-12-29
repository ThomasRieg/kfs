/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vga.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:33:35 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/29 23:42:16 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../io.h"
#include "vga.h"
#include "../libk/libk.h"

unsigned int g_vga_text_location = 0;
unsigned char * const g_vga_text_buf = (unsigned char *)0xb8000;
unsigned int g_vga_text_color = VGA_WHITE;

void update_cursor(int pos)
{
	outb(PORT_VGA_INDEX, 0x0F);
	outb(PORT_VGA_INDEXED, (unsigned char) (pos & 0xFF));
	outb(PORT_VGA_INDEX, 0x0E);
	outb(PORT_VGA_INDEXED, (unsigned char) ((pos >> 8) & 0xFF));
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
    for (unsigned int i = 0; i < VGA_WIDTH; i++) {
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

void write(const char *str, unsigned int n) {
	if (g_vga_text_location + 1 >= VGA_SIZE)
		scroll_down();
	for (unsigned int i = 0; i < n; i++) {
		if (str[i] == '\n')
			g_vga_text_location += (VGA_WIDTH * 2) - (g_vga_text_location % (VGA_WIDTH * 2));
		else
		{
			g_vga_text_buf[g_vga_text_location] = str[i];
			g_vga_text_buf[g_vga_text_location + 1] = g_vga_text_color;
			g_vga_text_location += 2;
		}
		if (g_vga_text_location + 1 >= VGA_SIZE)
			scroll_down();
	}
	update_cursor(g_vga_text_location / 2);
}

void writes(const char *str) {
	if (g_vga_text_location + 1 >= VGA_SIZE)
		scroll_down();
	while (*str) {
		if (*str == '\n')
			g_vga_text_location += (VGA_WIDTH * 2) - (g_vga_text_location % (VGA_WIDTH * 2));
		else
		{
			g_vga_text_buf[g_vga_text_location] = *str;
			g_vga_text_buf[g_vga_text_location + 1] = g_vga_text_color;
			g_vga_text_location += 2;
		}
		if (g_vga_text_location + 1 >= VGA_SIZE)
			scroll_down();
		str++;
	}
	update_cursor(g_vga_text_location / 2);
}

void clear_vga_screen()
{
    memset(g_vga_text_buf, 0, VGA_SIZE);
}
