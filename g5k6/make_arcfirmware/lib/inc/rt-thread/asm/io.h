#ifndef __FH_IO_H__
#define __FH_IO_H__
#include <asm/arc6xx.h>

#define readb(addr)      fh_hw_readbyte((unsigned int)(addr))
#define readl(addr)      fh_hw_readreg((unsigned int)(addr))
#define writel(val,addr) fh_hw_writereg((unsigned int)(addr), val)
#define writeb(val,addr) fh_hw_writebyte((unsigned int)(addr), val)

#define reg_read(addr)         readl(addr)
#define reg_write(addr,value)  writel(value, addr)

#define GET_REG(addr)          reg_read(addr)
#define SET_REG(addr, value)   reg_write(addr, value)
#define SET_REG_M(addr, value, mask) \
    reg_write(addr, (reg_read(addr) & (~(mask))) | ((value) & (mask)))

#endif
