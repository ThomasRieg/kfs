# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    context_switch.s                                   :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/19 15:57:19 by thrieg            #+#    #+#              #
#    Updated: 2026/01/19 16:24:32 by thrieg           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

.text

# void switch_to(uint32_t *next_kesp, uint32_t next_cr3);
# cdecl: [esp+4]=next_kesp, [esp+8]=next_cr3
.global switch_to
switch_to:
    mov 4(%esp), %eax        # eax = next_kesp
    mov 8(%esp), %edx        # edx = next_cr3

    mov %edx, %cr3           # switch address space if you have per-task CR3
    mov %eax, %esp           # switch stack last (or first, but args must be captured)

    ret

	