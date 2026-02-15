# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    int_entrypoint.s                                   :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/11 23:37:01 by thrieg            #+#    #+#              #
#    Updated: 2026/02/15 15:21:03 by thrieg           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

    .intel_syntax noprefix
    .text
    .extern signal_deliver_if_needed

    .global isr_common_stub
    .global isr_common_epilogue
    .global isr_common_epilogue_with_signals
isr_common_stub:

    # --- Save segment registers ---
    push ds
    push es
    push fs
    push gs

    # --- Save general registers ---
    pusha

    # --- Load kernel segments ---
    mov ax, 0x10     # kernel code (in gdt)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    # --- Pass pointer to regs to C ---
    push esp         # esp now points at start of regs frame
    call isr_dispatch_c
    add esp, 4

isr_common_epilogue_with_signals:
    push esp         # esp now points at start of regs frame
    call signal_deliver_if_needed
    add esp, 4

isr_common_epilogue:
    # --- Restore registers & segments ---
    popa
    pop gs
    pop fs
    pop es
    pop ds

    # Pop int_no + err_code
    add esp, 8

    iret
