/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#include "fh_def.h"
#include "gpio.h"
#include "mol_gpio.h"
#include <fh_arch.h>
#include <rtservice.h>
#include <rtthread.h>
#include <rthw.h>
#ifdef RT_USING_PM
#include <pm.h>
#endif

#if defined(RT_DEBUG) && defined(MOL_GPIO_DEBUG)
#define PRINT_GPIO_DBG(fmt, args...)   \
    do                                 \
    {                                  \
        rt_kprintf("GPIO_DEBUG: "); \
        rt_kprintf(fmt, ##args);       \
    } while (0)
#else
#define PRINT_GPIO_DBG(fmt, args...) \
    do                               \
    {                                \
    } while (0)
#endif

extern int __rt_ffs(int value);

#define GPIO_MAX_CHIP_NUM 7
#define GPIO_NUM_PER_CHIP 8

int gpio_irq_type[GPIO_MAX_CHIP_NUM * GPIO_NUM_PER_CHIP];

static struct mol_gpio_chip gpio_obj[] = {
    {
        .id = 0,
        .base = GPIO0_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO0_IRQn,
        .chip = {
            .base = 0,
            .ngpio = 8,
        },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
    {
        .id = 1,
        .base = GPIO1_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO1_IRQn,
        .chip = {
            .base = 8,
            .ngpio = 8,
        },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
    {
        .id = 2,
        .base = GPIO2_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO2_IRQn,
        .chip = {
            .base = 16,
            .ngpio = 8,
        },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
    {
        .id = 3,
        .base = GPIO3_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO3_IRQn,
        .chip = {
            .base = 24,
            .ngpio = 8,
        },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
    {
        .id = 4,
        .base = GPIO4_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO4_IRQn,
        .chip = {
            .base = 32,
            .ngpio = 8,
        },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
    {
        .id = 5,
        .base = GPIO5_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO5_IRQn,
        .chip = {
            .base = 40,
            .ngpio = 8,
        },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
    {
        .id = 6,
        .base = GPIO6_REG_BASE,
        .trigger_type = HARDWARE,
        .irq = GPIO6_IRQn,
        .chip = {
            .base = 48,
            .ngpio = 8,
           },
        .pmchangedirmask = 0xffffffff,
        .pmsaveddirval = 0,
    },
};
#ifdef RT_USING_PM
int gpio_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    int i = 0;


    for (i = 0; i < GPIO_MAX_CHIP_NUM; i++ ) {
        gpio_obj[i].pmsaveddirval = reg_read(gpio_obj[i].base+0x8);
        SET_REG_M(gpio_obj[i].base + 0x08, 0 , gpio_obj[i].pmchangedirmask);
    }


    return RT_EOK;
}

void gpio_resume(const struct rt_device *device, rt_uint8_t mode)
{
    int i = 0;

    for (i = 0; i < GPIO_MAX_CHIP_NUM;i++) {
            SET_REG(gpio_obj[i].base + 0x08,  gpio_obj[i].pmsaveddirval);
    }
}

struct rt_device_pm_ops gpio_pm_ops =
{
    .suspend = gpio_suspend,
    .resume = gpio_resume
};
#endif

void set_gpio_pm_exclude(unsigned int excludenum)
{
    unsigned int idx = excludenum/GPIO_NUM_PER_CHIP;
    unsigned int off = excludenum % GPIO_NUM_PER_CHIP;

    if (idx >= GPIO_MAX_CHIP_NUM || off >= GPIO_NUM_PER_CHIP)
    {
        rt_kprintf("%s: ERROR, gpio num too large!\n", __func__);
        return;
    }

    gpio_obj[idx].pmchangedirmask  = gpio_obj[idx].pmchangedirmask & (~(1<<off));

}

#if 1
static unsigned int gpio_to_base(unsigned int gpio)
{
    struct gpio_chip *gc;
    struct mol_gpio_chip *chip;

    gc   = gpio_to_chip(gpio);
    chip = rt_container_of(gc, struct mol_gpio_chip, chip);

    return chip->base;
}
#endif

static int _gpio_to_irq(unsigned int gpio)
{
    return (MAX_HANDLERS - 1 - gpio);
}

static int chip_get_direction(struct gpio_chip *gc, unsigned int offset)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg  = GET_REG(chip->base + REG_GPIO_DIR);
    reg  &= BIT(offset);
    reg  = reg >> offset;

    return reg;
}

static int chip_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DATA);
    reg &= BIT(offset);
    reg = reg >> offset;

    return reg;
}

static void chip_gpio_set(struct gpio_chip *gc, unsigned int offset, int val)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DMSK);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_DMSK, reg);
    reg  = GET_REG(chip->base + REG_GPIO_DATA);
    reg = val ? (reg | BIT(offset)) : (reg & ~BIT(offset));

    SET_REG(chip->base + REG_GPIO_DATA, reg);
}

static int chip_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DIR);
    reg &= ~(1 << offset);
    SET_REG(chip->base + REG_GPIO_DIR, reg);

    reg = GET_REG(chip->base + REG_GPIO_DMSK);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_DMSK, reg);

    reg = GET_REG(chip->base + REG_GPIO_INEN);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_INEN, reg);

    return 0;
}

static int chip_direction_output(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DIR);
    reg |= (1 << offset);
    SET_REG(chip->base + REG_GPIO_DIR, reg);

    reg = GET_REG(chip->base + REG_GPIO_DMSK);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_DMSK, reg);

    reg = GET_REG(chip->base + REG_GPIO_DATA);
    reg = value ? (reg | (1 << offset)) : (reg & ~(1 << offset));
    SET_REG(chip->base + REG_GPIO_DATA, reg);

    return 0;
}

static void chip_set_direction(struct gpio_chip *gc, unsigned int offset, unsigned direction)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DIR);

    reg = direction ? (reg | (1 << offset)) : (reg & ~(1 << offset));
    SET_REG(chip->base + REG_GPIO_DIR, reg);
}

static int chip_set_debounce(struct gpio_chip *chip, unsigned int offset, unsigned debounce)
{
    return 0;
}

static int chip_to_irq(struct gpio_chip *gc, unsigned int offset)
{
    return (MAX_HANDLERS - gc->base - offset - 1);
}

static int chip_set_irq_type(struct gpio_chip *gc, unsigned int offset, unsigned type)
{
    struct mol_gpio_chip *chip;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    int int_type     = GET_REG(chip->base + REG_GPIO_IS);
    int int_polarity = GET_REG(chip->base + REG_GPIO_IEV);
    int both_edge    = GET_REG(chip->base + REG_GPIO_IBE);
    both_edge        &= ~BIT(offset);

    switch (type & IRQ_TYPE_TRIGGER_MASK)
    {
    case IRQ_TYPE_EDGE_BOTH:
        both_edge |= BIT(offset);
        int_type &= ~BIT(offset);
        break;
    case IRQ_TYPE_EDGE_RISING:
        int_type &= ~BIT(offset);
        int_polarity |= BIT(offset);
        break;
    case IRQ_TYPE_EDGE_FALLING:
        int_type &= ~BIT(offset);
        int_polarity &= ~BIT(offset);
        break;
    case IRQ_TYPE_LEVEL_HIGH:
        int_type |= BIT(offset);
        int_polarity |= BIT(offset);
        break;
    case IRQ_TYPE_LEVEL_LOW:
        int_type |= BIT(offset);
        int_polarity &= ~BIT(offset);
        break;
    case IRQ_TYPE_NONE:
        return 0;
    default:
        return -RT_ERROR;
    }

    SET_REG(chip->base + REG_GPIO_IS, int_type);
        SET_REG(chip->base + REG_GPIO_IEV, int_polarity);
        SET_REG(chip->base + REG_GPIO_IBE, both_edge);

    gpio_irq_type[gc->base + offset] = type;
    return 0;
}

void chip_irq_enable(unsigned int irq)
{
    int reg;
    int gpio = MAX_HANDLERS - irq;
    int base = gpio_to_base(gpio);

    reg = GET_REG(base + REG_GPIO_RIS);
    SET_REG(base + REG_GPIO_IC, reg);

    reg = GET_REG(base + REG_GPIO_IE);
    reg |= BIT((MAX_HANDLERS - irq - 1) % 8);
    SET_REG(base + REG_GPIO_IE, reg);
}

void chip_irq_disable(unsigned int irq)
{
    int reg;
    int gpio = MAX_HANDLERS - irq;
    int base = gpio_to_base(gpio);

    reg = GET_REG(base + REG_GPIO_IE);
    reg &= ~BIT((MAX_HANDLERS - irq - 1) % 8);
    SET_REG(base + REG_GPIO_IE, reg);
}

static void mol_gpio_interrupt(int irq, void *param)
{
    unsigned int irq_status, offset;
    struct mol_gpio_chip *chip;
    struct gpio_chip *gc = (struct gpio_chip *)param;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);

    irq_status = GET_REG(chip->base + REG_GPIO_MIS);
    offset = __rt_ffs(irq_status) - 1;//look for who trigger the irq

    rt_hw_interrupt_mask(chip->irq);
    generic_handle_irq(chip_to_irq(gc, offset));
    SET_REG(chip->base + REG_GPIO_IC, (0x1 << offset));//handle irq and clear irq flag

    rt_hw_interrupt_umask(chip->irq);
}

static struct irq_chip gpio_int_chip = {
    .name       = "gpio_intc",
    .irq_unmask   = chip_irq_enable,
    .irq_mask = chip_irq_disable,
};

int mol_gpio_probe(void *priv_data)
{
    int err;
    int idx;
    struct mol_gpio_chip *gc = (struct mol_gpio_chip *)priv_data;
    char irqname[8] = {0};

    gc->chip.get_direction    = chip_get_direction;
    gc->chip.direction_input  = chip_direction_input;
    gc->chip.direction_output = chip_direction_output;
    gc->chip.get              = chip_gpio_get;
    gc->chip.set              = chip_gpio_set;
    gc->chip.set_direction    = chip_set_direction;
    gc->chip.set_debounce     = chip_set_debounce;
    gc->chip.set_type         = chip_set_irq_type;
    gc->chip.to_irq           = chip_to_irq;

    err = gpiochip_add(&gc->chip);
    if (err)
    {
        rt_kprintf("GPIO controller load fail\n");
        return err;
    }

    rt_sprintf(irqname, "gpio_%d", gc->id);
    rt_hw_interrupt_install(gc->irq, mol_gpio_interrupt, gc, irqname);
    for (idx = 0; idx < gc->chip.ngpio; idx++)
        irq_set_chip(_gpio_to_irq(gc->chip.base + idx), &gpio_int_chip);

    rt_hw_interrupt_umask(gc->irq);

    return 0;
}

struct rt_device * gpio_device;

void rt_hw_gpio_init(void)
{
    int i;

    PRINT_GPIO_DBG("%s start\n", __func__);
    gpio_device = (struct rt_device *)rt_malloc(sizeof (struct rt_device));
    for (i = 0; i < GPIO_MAX_CHIP_NUM; i++)
        mol_gpio_probe(&gpio_obj[i]);
#ifdef RT_USING_PM
    rt_pm_device_register(gpio_device, &gpio_pm_ops);
#endif

    PRINT_GPIO_DBG("%s end\n", __func__);
}

#include <rtthread.h>
#include <rtdevice.h>
#include <delay.h>
#include "gpio.h"
#include "mol_gpio.h"
#include <rthw.h>

static void gpio_irq_test(int irq, void *param)
{
    rt_kprintf("Enter gpio irq\n");
}

void gpio_test(void)
{
    int arr1[4] = {11, 17, 33, 41};
    int arr2[4] = {12, 25, 41, 49};
    int i, k, ret;

    rt_kprintf("gpio test start\n");
    for(i = 0; i < 1; i++) {
        rt_kprintf("test gpio%d gpio%d\n", arr1[i], arr2[i]);

        ret = gpio_request(arr1[i]);
        if(ret) {
            rt_kprintf("request gpio%d failed\n", arr1[i]);
            return;
        }
        ret = gpio_request(arr2[i]);
        if(ret) {
            rt_kprintf("request gpio%d failed\n", arr2[i]);
            return;
        }
        rt_kprintf("request gpio success\n");

        ret = gpio_direction_input(arr1[i]);
        if(ret) {
            rt_kprintf("set gpio%d direction failed\n", arr1[i]);
            return;
        }
        ret = gpio_direction_output(arr2[i], 0);
        if(ret) {
            rt_kprintf("set gpio%d direction failed\n", arr2[i]);
            return;
        }
        rt_kprintf("set gpio%d as input, gpio%d as output success\n", arr1[i], arr2[i]);

        rt_hw_interrupt_install(gpio_to_irq(arr1[i]), gpio_irq_test, RT_NULL, "gpio_test");
        ret = gpio_set_irq_type(arr1[i], IRQ_TYPE_EDGE_BOTH);
        if(ret) {
            rt_kprintf("set gpio%d IRQ_TYPE_EDGE_BOTH failed\n", arr1[i]);
            return;
        }
            rt_kprintf("set gpio%d IRQ_TYPE_EDGE_BOTH success\n", arr1[i]);
        gpio_irq_enable(gpio_to_irq(arr1[i]));
        rt_kprintf("irqnum: %d\n", gpio_to_irq(arr1[i]));
        for(k = 0; k < 4; k++) {
            rt_thread_mdelay(1000);
            rt_kprintf("set gpio%d high\n", arr2[i]);
            rt_thread_mdelay(1000);
            gpio_set_value(arr2[i], 1);
            rt_thread_mdelay(1000);
            rt_kprintf("set gpio%d low\n", arr2[i]);
            gpio_set_value(arr2[i], 0);
            // rt_thread_mdelay(1000);
            // rt_kprintf("set gpio%d high\n", arr2[i]);
            // ret = gpio_set_value(arr2[i], 1);
        //     rt_thread_mdelay(1000);
        //     rt_kprintf("set gpio%d low\n", arr2[i]);
        //     gpio_set_value(arr2[i], 0);
        //     rt_thread_mdelay(1000);
        //     rt_kprintf("set gpio%d high\n", arr2[i]);
        //     gpio_set_value(arr2[i], 1);
        //     rt_thread_mdelay(1000);
        //     rt_kprintf("set gpio%d low\n\n\n", arr2[i]);
        //     gpio_set_value(arr2[i], 0);
        //     rt_thread_mdelay(1000);
        }
        gpio_irq_disable(arr1[i]);
        gpio_release(arr1[i]);
        gpio_release(arr2[i]);
    }
}
// MSH_CMD_EXPORT(gpio_test, test fullhan gpio);
