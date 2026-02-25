#ifndef _c2c_comm__h__
#define _c2c_comm__h__

#if defined(CONFIG_CHIP_MC632X)

#define C2C_INTR0_REG_BASE      (0x0A100000)
#define C2C_INTR1_REG_BASE      (0x0A200000)
#define C2C_INTR_REQ            (C2C_INTR1_REG_BASE+0)
#define C2C_CLR_REMOTE          (C2C_INTR1_REG_BASE+4)
#define C2C_CLR_LOCAL           (C2C_INTR1_REG_BASE+8)
#define C2C_INT_EN              (C2C_INTR1_REG_BASE+0x0c)
#define C2C_INT_RAW             (C2C_INTR1_REG_BASE+0x10)
#define C2C_INT_MASK            (C2C_INTR1_REG_BASE+0x14)

#define fh_mcu_c2c_wakeup_host()                writel(2, C2C_INTR_REQ)
#define fh_mcu_trigger_interrupt_to_host()      writel(1, C2C_INTR_REQ)
#define fh_mcu_clear_interrupt()                writel(0xffffffff, C2C_CLR_REMOTE)
#define fh_mcu_notify_xbus_buffer_address(addr) writel((unsigned int)(addr), C2C_INTR_REQ)
#define fh_c2c_init() do { \
    writel(0xffffffff, C2C_CLR_REMOTE); \
    writel(0xffffffff, C2C_CLR_LOCAL); \
    writel(0xffffffff, (C2C_INTR0_REG_BASE+0x0c)); \
    writel(0xffffffff, C2C_INT_EN);} while(0)

#else //CONFIG_CHIP_MC632X
#define fh_mcu_trigger_interrupt_to_host()      writel(1<<8, ARM_INT_RAWSTATUS)
#define fh_mcu_clear_interrupt()                writel(0, ARC_INT_RAWSTATUS);
#define fh_mcu_notify_xbus_buffer_address(addr) writel((unsigned int)(addr), ARM_INT_RAWSTATUS)
#define fh_c2c_init()                           do{}while(0)
#endif //CONFIG_CHIP_MC632X

#endif /*_c2c_comm__h__*/
