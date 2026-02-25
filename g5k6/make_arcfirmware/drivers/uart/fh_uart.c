#include <rtthread.h>
#include <rthw.h>
#include <fh_arch.h>
#include <asm/io.h>
#include "uart.h"

static void uart_configure(uart *port, int baudrate, int parity, int stopbits)
{
    unsigned int brate = (((UART_CLOCK_FREQ / baudrate) + 8) / 16);
    unsigned int lcr = UART_LCR_DLS8;

    writel(1, &port->UART_SRR);
    udelay(10);

    /*disable interrupt*/
    writel(0, &port->UART_IER);

    // baud rate
    writel(UART_LCR_DLAB, &port->UART_LCR);
    writel(brate & 0xff, &port->RBRTHRDLL);
    writel((brate >> 8) & 0xff, &port->DLHIER);
    writel(0, &port->UART_LCR);    /* clear DLAB */

    // line control
    if (stopbits == 2) /*2 stopbits, else 1 stopbits...*/
        lcr |= (1 << 2);
    if (parity == 1) /*odd parity*/
        lcr |= (1 << 3);
    if (parity == 2) /*even parity*/
        lcr |= ((1 << 3) | (1 << 4));
    writel(lcr, &port->UART_LCR);

    // fifo control
    writel(UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR, &port->UART_FCR);
}

static rt_err_t uart_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_size_t uart_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    int i;
    unsigned int lsr;
    unsigned char *str = (unsigned char *)buffer;
    uart *port = (uart *)dev->user_data;

    for(i=0; i<size; i++)
    {
        lsr = readl(&port->UART_LSR);
        if (!(lsr & UART_LSR_DR))
            break;
        *(str++) = readl(&port->UART_RBR) & 0xFF;
    }

    return i;
}

static rt_size_t uart_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    int len = size;
    unsigned char *str = (unsigned char *)buffer;
    uart *port = (uart *)dev->user_data;

    while(*str && len-- > 0)
    {
        while (!(readl(&port->UART_USR) & UART_USR_TFNF));
        if (*str == '\n')
            writel('\r', &port->UART_THR);
        while (!(readl(&port->UART_USR) & UART_USR_TFNF));
        if (*str == '\r')
            writel('\n', &port->UART_THR);
        while (!(readl(&port->UART_USR) & UART_USR_TFNF));
        writel(*str, &port->UART_THR);
        str++;
    }

    return size;
}

#ifdef FH_USING_UART_SHAREDMODE
rt_device_t fh_hw_uart_init(unsigned int base, char *name, int baudrate, int parity, int stop)
{
    struct rt_device *dev = (struct rt_device *)rt_malloc(sizeof(struct rt_device));

    if (!dev)
        goto Error;

    rt_memset(dev, 0, sizeof(struct rt_device));
    dev->open = uart_open;
    dev->read = uart_read;
    dev->write = uart_write;
    dev->user_data = (void *)base;

    if (rt_device_register(dev, name, 0) != 0)
    {
        rt_free(dev);
        goto Error;
    }

    return dev;

Error:
    return RT_NULL;
}

#else

rt_device_t fh_hw_uart_init(unsigned int base, char *name, int baudrate, int parity, int stop)
{
    struct rt_device *dev = (struct rt_device *)rt_malloc(sizeof(struct rt_device));

    if (!dev)
        goto Error;

    rt_memset(dev, 0, sizeof(struct rt_device));
    dev->open = uart_open;
    dev->read = uart_read;
    dev->write = uart_write;
    dev->user_data = (void *)base;

    if (rt_device_register(dev, name, 0) != 0)
    {
        rt_free(dev);
        goto Error;
    }

    uart_configure((uart *)dev->user_data, baudrate, parity, stop);

    return dev;

Error:
    return RT_NULL;
}
#endif

void fh_hw_uart_resume(struct rt_device *dev, int baudrate, int parity, int stop)
{
    return uart_configure((uart *)dev->user_data, baudrate, parity, stop);
}

