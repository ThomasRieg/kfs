# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    int_entrypoint.s                                   :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/11 23:37:01 by thrieg            #+#    #+#              #
#    Updated: 2026/02/20 02:43:13 by thrieg           ###   ########.fr        #
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



.intel_syntax noprefix
.global switch_to
.type switch_to, @function

# void switch_to(uint32_t *prev_kctx_esp, uint32_t next_kctx_esp);
switch_to:
    # stack at entry:
    # [esp+0] return addr
    # [esp+4] prev_kctx_esp*
    # [esp+8] next_kctx_esp

    push ebp
    push ebx
    push esi
    push edi

    mov eax, [esp + 4 + 16]    # eax = prev_kctx_esp*   (16 bytes pushed)
    mov edx, [esp + 8 + 16]    # edx = next_kctx_esp

    mov [eax], esp             # *prev_kctx_esp = current esp (after pushes)
    mov esp, edx               # esp = next_kctx_esp

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
.size switch_to, .-switch_to