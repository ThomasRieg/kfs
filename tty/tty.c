/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tty.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 01:56:38 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 03:39:19 by thrieg           ###   ########.fr       */
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