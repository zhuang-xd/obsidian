/*
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 */
#include <rthw.h>
#include <rtthread.h>
#include <asm/arc6xx.h>

#define CTL_FS_FLAG   0x100
#define AUX_DC_CTRL   0x48
#define AUX_DC_INVL   0x4a
#define AUX_DC_IVDC   0x47
#define AUX_DC_FLSH   0x4b
#define AUX_DC_FLDL   0x4c
#define AUX_DC_BILD   0x72

unsigned int mmu_dcache_linesz(void)
{
    unsigned int clsz = 16 << ((fh_hw_readaux(AUX_DC_BILD) >> 16) & 0x0f);

    return clsz;
}

void mmu_clean_dcache_intron(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int clsz = 16 << ((fh_hw_readaux(AUX_DC_BILD) >> 16) & 0x0f);

    unsigned int base = buffer & (~(clsz - 1));
    unsigned int end  = buffer + size - 1;
    unsigned int loop;

    for (loop = base; loop <= end; loop += clsz)
    {
        fh_hw_writeaux(AUX_DC_FLDL, loop);
        while ((fh_hw_readaux(AUX_DC_CTRL) & CTL_FS_FLAG) == CTL_FS_FLAG);
    }
}

void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
	rt_base_t level;
	unsigned int done = 0;
	
    unsigned int clsz = 16 << ((fh_hw_readaux(AUX_DC_BILD) >> 16) & 0x0f);

    unsigned int base = buffer & (~(clsz - 1));
    unsigned int end  = buffer + size - 1;
    unsigned int loop;
    if (size == 0)
        return;

    level = rt_hw_interrupt_disable();
    for (loop = base; loop <= end; loop += clsz)
    {
        fh_hw_writeaux(AUX_DC_FLDL, loop);
        if (++done >= 200)
        {
        	while ((fh_hw_readaux(AUX_DC_CTRL) & CTL_FS_FLAG) == CTL_FS_FLAG);
        	rt_hw_interrupt_enable(level);
        	done = 0;
        	level = rt_hw_interrupt_disable();
        	continue;
        }
        while ((fh_hw_readaux(AUX_DC_CTRL) & CTL_FS_FLAG) == CTL_FS_FLAG);
    }
    rt_hw_interrupt_enable(level);
}

void fh_hw_flush_dcache_all(void)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    fh_hw_writeaux(AUX_DC_FLSH, 1);
    while ((fh_hw_readaux(AUX_DC_CTRL) & CTL_FS_FLAG) == CTL_FS_FLAG);
    rt_hw_interrupt_enable(level);
}

void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    rt_base_t level;
    unsigned int clsz = -(16 << (fh_hw_readaux(AUX_DC_BILD) >> 16));

    unsigned int base = buffer & clsz;
    unsigned int end  = (buffer + size - 1) & clsz;
    unsigned int loop;
    unsigned int done = 0;
    if (size == 0)
        return;
    
    level = rt_hw_interrupt_disable();
    for (loop = base; loop <= end; loop += (-clsz))
    {
        fh_hw_writeaux(AUX_DC_INVL, loop);
        while ((fh_hw_readaux(AUX_DC_CTRL) & CTL_FS_FLAG) == CTL_FS_FLAG);
        
        if (++done >= 200)
        {
        	rt_hw_interrupt_enable(level);
        	done = 0;
        	level = rt_hw_interrupt_disable();
        }
    }
    rt_hw_interrupt_enable(level);
}

void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    mmu_clean_dcache(buffer, size);
    mmu_invalidate_dcache(buffer, size);
}

void fh_hw_flush_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    mmu_clean_dcache(buffer, size);
}

void fh_hw_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    mmu_invalidate_dcache(buffer, size);
}

/*
void fh_hw_invalidate_dcache_all(void)
{
    fh_hw_writeaux(AUX_DC_IVDC, 1);
    while ((fh_hw_readaux(AUX_DC_CTRL) & CTL_FS_FLAG) == CTL_FS_FLAG);
}
*/
