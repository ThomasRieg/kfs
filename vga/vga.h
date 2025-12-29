/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vga.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:29:07 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/29 23:38:26 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT * 2)

enum vga_color {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW        = 14,
    VGA_WHITE         = 15
};

extern unsigned int g_vga_text_location;

extern unsigned char * const g_vga_text_buf;

extern unsigned int g_vga_text_color;

void update_cursor(int pos);

void scroll_down();

void vga_set_color(uint8_t foreground, uint8_t background);

void vga_get_color(uint8_t *foreground, uint8_t *background);

void writes(const char *str);

void write(const char *str, unsigned int n);

void clear_vga_screen();

#endif
