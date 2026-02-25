#include <rtthread.h>
#include <asm/io.h>
#include <fh_arch.h>
#ifdef RT_USING_PM
#include <pm.h>
#endif

#ifdef FH_USING_UART_SHAREDMODE
#define UART_DEV_NAME "uart0"
#else
#define UART_DEV_NAME "uart1"
#endif

extern rt_device_t fh_hw_uart_init(unsigned int base, /*base address for UART register*/
              char *name,  /*device name*/
              int baudrate, /*115200,57600,38400,19200,9600*/
              int parity, /*0:no parity; 1:odd; 2:even*/
              int stop);  /*1:1 stop-bit;  2: 2stop-bits*/

extern void fh_hw_uart_resume(struct rt_device *dev, int baudrate, int parity, int stop);

static rt_device_t st_uart_dev;

static void uart2_pin_switch(void)
{
#ifdef FH_USING_UART_SHAREDMODE
    SET_REG_M(0x0c0000ac, 1 << 9, 1 << 9);
    SET_REG_M(0x0c0000b0, 1 << 12, 1 << 12);
    SET_REG(0x10200040, 0x0);
    SET_REG(0x10200044, 0x0);
#else
    SET_REG_M(0x0c0000ac, 1 << 10, 1 << 10);
    SET_REG_M(0x0c0000b0, 1 << 13, 1 << 13);
    SET_REG(0x10200048, 0x2);
    SET_REG(0x1020004c, 0x2);
#endif
}

#ifdef RT_USING_PM
int uart_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    return RT_EOK;
}

void uart_resume(const struct rt_device *device, rt_uint8_t mode)
{
#if defined (FH_USING_XBRSH) && !defined (FH_USING_UART_SHAREDMODE)
#else
    uart2_pin_switch();
    fh_hw_uart_resume(device, 115200, 0, 1);

    rt_console_set_device(UART_DEV_NAME);
#endif
}

struct rt_device_pm_ops uart_pm_ops =
{
    .suspend = uart_suspend,
    .resume = uart_resume
};
#endif

#ifdef FH_USING_UART_SHAREDMODE
void uart_init(void)
{
    unsigned int base = UART0_REG_BASE;

    st_uart_dev = fh_hw_uart_init(base, UART_DEV_NAME, 115200, 0, 1);
    if (!st_uart_dev)
    {
        rt_kprintf("uart device register failed!\n");
    }

    rt_device_open(st_uart_dev, 0);

#ifdef RT_USING_PM
    rt_pm_device_register(st_uart_dev, &uart_pm_ops);
#endif
}

void change_console_to_uart0(void)
{
  struct rt_device *device = RT_NULL;

  device = rt_device_find(UART_DEV_NAME);
  if (device)
  {
    fh_hw_uart_resume(device, 115200, 0, 1);
  }

  rt_console_set_device(UART_DEV_NAME);
}

#else
void uart_init(void)
{
    unsigned int base = UART1_REG_BASE;

    /*now, we use UART2 for demo*/
    uart2_pin_switch();

    st_uart_dev = fh_hw_uart_init(base, UART_DEV_NAME, 115200, 0, 1);
    if (!st_uart_dev)
    {
        rt_kprintf("uart device register failed!\n");
    }

    rt_device_open(st_uart_dev, 0);

    rt_console_set_device(UART_DEV_NAME);

#ifdef RT_USING_PM
    rt_pm_device_register(st_uart_dev, &uart_pm_ops);
#endif
}
#endif
