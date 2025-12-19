/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tty.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 01:56:38 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 13:57:27 by alier            ###   ########.fr       */
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
    else if (g_current_tty >= g_active_tty - 1)
    {
        g_active_tty++;
    }
    g_current_tty++;
    load_tty();
}

void tty_add_input(char c) {
	// echo mode
	if (c == '\r')
		write(&(char){'\n'}, 1);
	else
		write(&c, 1);

	t_tty *curr = &g_ttys[g_current_tty];
	if (c == '\r') {
		if (curr->cmd_len != 0) {
			if (curr->cmd_len == 4 && memcmp(curr->cmd, "stop", 4) == 0)
				writes("stopping...\n");
			else
				writes("command not found!\n");
			curr->cmd_len = 0;
		}
	}
	else if (curr->cmd_len != sizeof(curr->cmd))
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
