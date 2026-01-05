/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   convertstuff.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 04:21:45 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/05 17:58:18 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

void int32_str_10(char out[12], int n) {
	unsigned int u = n < 0 ? -n : n;
	unsigned int i = 1;
	do {
		i++;
	} while (u /= 10);
	if (n < 0)
		i++;
	u = n < 0 ? -n : n;
	out[--i] = 0;
	do {
		out[--i] = u % 10 + '0';
		u /= 10;
	} while(u);
	if (n < 0)
		out[--i] = '-';
}

void uint32_str_10(char out[11], unsigned int n) {
	unsigned int i = 1;
	unsigned int u = n;
	do {
		i++;
	} while (u /= 10);
	out[--i] = 0;
	do {
		out[--i] = n % 10 + '0';
		n /= 10;
	} while(n);
}

void u32_to_hex(char out[9], unsigned int x, int upper)
{
    static const char *lo = "0123456789abcdef";
    static const char *hi = "0123456789ABCDEF";
    const char *digits = upper ? hi : lo;

    for (int i = 7; i >= 0; --i) {
        out[i] = digits[x & 0xF];
        x >>= 4;
    }
    out[8] = 0;
}
