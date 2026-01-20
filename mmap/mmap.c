/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:25:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/20 22:33:56 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mmap.h"
#include "../common.h"

void *mmap(void *addr, uint32_t lengthint, int prot, int flags, int fd, uint32_t offset)
{
    if (offset || fd > 0)
        return (MAP_FAILED); //not supported yet, add it when (if) we add memory with fd backing
}