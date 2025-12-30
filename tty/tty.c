/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tty.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 01:56:38 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/30 13:37:42 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "tty.h"
#include "../vga/vga.h"
#include "../libk/libk.h"

t_tty g_ttys[TTY_MAX];
unsigned int g_active_tty = 1;
unsigned int g_current_tty = 0;

//save the vga buffer into the tty buffer
void save_tty()
{
    memcpy(g_ttys[g_current_tty].buf, g_vga_text_buf, VGA_SIZE);
    g_ttys[g_current_tty].cursor = g_vga_text_location;
}

//puts the current tty buffer into the vga buffer
void load_tty()
{
    memcpy(g_vga_text_buf, g_ttys[g_current_tty].buf, VGA_SIZE);
    g_vga_text_location = g_ttys[g_current_tty].cursor;
    update_cursor(g_vga_text_location / 2);
}

//silently does nothing if you can't add a tty, just stay on the last one
void next_tty()
{
    if (g_current_tty >= TTY_MAX - 1)
    {
        return; //last tty, can't create another one
    }
    g_current_tty++;
    if (g_current_tty < g_active_tty)
    {
        load_tty();
    }
    else
    {
        g_active_tty++;
        clear_vga_screen();
        writes("> ");
    }
}

void handle_command(unsigned char len, const unsigned char *cmd);

void tty_add_input(char c) {
	t_tty *curr = &g_ttys[g_current_tty];

	// echo mode
	if (c == '\r')
		write(&(char){'\n'}, 1);
	else if (c == '\b') {
		if (curr->cmd_len != 0) {
			curr->cmd_len--;
			g_vga_text_location -= 2;
			g_vga_text_buf[g_vga_text_location] = 0;
			update_cursor(g_vga_text_location / 2);
		}
	} else
		write(&c, 1);

	if (c == '\r') {
		handle_command(curr->cmd_len, curr->cmd);
		curr->cmd_len = 0;
	}
	else if (c != '\b' && curr->cmd_len != sizeof(curr->cmd))
		curr->cmd[curr->cmd_len++] = c;
}

//silently does nothing if you are on the first tty, just stay on the first one
void prev_tty()
{
    if (g_current_tty == 0)
    {
        return; //first tty
    }
    g_current_tty--;
    load_tty();
}
