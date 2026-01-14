/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   s_regs.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 14:25:18 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/14 17:53:25 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REGS_H
#define REGS_H

#include <stdint.h>

typedef struct __attribute__((packed)) t_regs
{
	/* general registers saved by pusha */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t orig_esp; /* the esp that existed before pusha */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	/* segment registers saved by common stub (int_entrypoint.s)*/
	uint32_t ds;
	uint32_t es;
	uint32_t fs;
	uint32_t gs;

	/* pushed by micro-stub (isr_stubs.s) */
	uint32_t int_no;
	uint32_t err_code;

	/* pushed automatically by CPU */
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	/* Only present if coming from lower privilege (user->kernel) (check cs & 3 before using)*/
	uint32_t useresp;
	uint32_t ss;
} t_regs;

#endif