/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vga.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:29:07 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 13:56:35 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT * 2)

enum vga_color {
	VGA_WHITE = 15
};

extern unsigned int g_vga_text_location;

extern unsigned char * const g_vga_text_buf;

void update_cursor(int pos);

void scroll_down();

void writes(const char *str);

void write(const char *str, unsigned int n);

void clear_vga_screen();

#endif
