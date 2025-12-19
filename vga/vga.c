/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vga.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:33:35 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 03:19:32 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vga.h"
#include "../libk/libk.h"

unsigned int g_vga_text_location = 0;
unsigned char * const g_vga_text_buf = (unsigned char *)0xb8000;

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
			g_vga_text_buf[g_vga_text_location + 1] = VGA_WHITE;
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
			g_vga_text_buf[g_vga_text_location + 1] = VGA_WHITE;
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
