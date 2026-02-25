/*
 * File      : arm6xx.h
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

#ifndef __ARC6XX_H__
#define __ARC6XX_H__

__extension__ static inline unsigned int __attribute__((__always_inline__))
fh_hw_readreg(unsigned int addr)
{
    unsigned int val;
    asm volatile ("ld.di\t%0,[%1]" : "=r"(val) : "r"(addr));
    return val;
}

__extension__ static inline unsigned int __attribute__((__always_inline__))
fh_hw_readaux(unsigned int addr)
{
    unsigned int val;
    asm volatile ("lr\t%0,[%1]" : "=r"(val) : "r"(addr));
    return val;
}

__extension__ static inline unsigned char __attribute__((__always_inline__))
fh_hw_readbyte(unsigned int addr)
{
    unsigned char val;
    asm volatile ("ldb.di\t%0,[%1]" : "=r"(val) : "r"(addr));
    return val;
}

__extension__ static inline void __attribute__((__always_inline__))
fh_hw_writereg(unsigned int addr, unsigned int val)
{
    asm volatile ("st.di\t%0,[%1]" : : "r"(val), "r"(addr));
}

__extension__ static inline void __attribute__((__always_inline__))
fh_hw_writeaux(unsigned int addr, unsigned int val)
{
    asm volatile ("sr\t%0,[%1]" : : "r"(val),"r"(addr));
}

__extension__ static inline void __attribute__((__always_inline__))
fh_hw_writebyte(unsigned int addr,unsigned char val)
{
    asm volatile ("stb.di\t%0,[%1]" : : "r"(val), "r"(addr));
}

#endif
