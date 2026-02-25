/*
 * File      : arch.h
 * This file is part of FH8620 BSP for RT-Thread distribution.
 *
 * Copyright (c) 2016 Shanghai Fullhan Microelectronics Co., Ltd.
 * All rights reserved
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
 *  Visit http://www.fullhan.com to get contact with Fullhan.
 *
 * Change Logs:
 * Date           Author       Notes
 */

#ifndef ARCH_H_
#define ARCH_H_

/*****************************/
/* BSP CONTROLLER BASE       */
/*****************************/
#define UART0_REG_BASE        (0x08f00000)
#define UART1_REG_BASE        (0x09000000)
#define UART2_REG_BASE        (0x09100000)
#define CPU_SYS_CLK_RF_REG_BASE    (0x09200200)
#define TMR_REG_BASE        (0x09800000)
#define GPIO0_REG_BASE        (0x0B000000)
#define GPIO1_REG_BASE        (0x0B010000)
#define GPIO2_REG_BASE        (0x0B020000)
#define GPIO3_REG_BASE        (0x0B030000)
#define GPIO4_REG_BASE        (0x0B040000)
#define GPIO5_REG_BASE        (0x0B050000)
#define GPIO6_REG_BASE      (0x0B060000)

#define I2C0_REG_BASE                       (0x08d00000)
#define I2C1_REG_BASE                       (0x08e00000)
#define I2S_REG_BASE                        (0x08900000)
#define RTC_REG_BASE                        (0x2c900000)
#define TIMER0_REG_BASE                     (0x09800000)
#define SYS_TIMER_REG_BASE                  (0x09700000)
#define WDT0_REG_BASE                       (0x09600000)
#define TIMER0_REG_BASE                     (0x09800000)
#define SPI0_REG_BASE                       (0x08c00000)
#define SPI1_REG_BASE                       (0x08b00000)
#define PWM_REG_BASE                        (0x09500000)

#define CPU_MTX_APB_REG_BASE        (0x0b200000)
#define CPU_SYS_AHB_REG_BASE        (0x0c000000)
#define SMT0_REG_BASE               (0x0a400000)
#define SMT1_REG_BASE               (0x0a800000)
#define TOP_PRE_DIV_RF_REG_BASE     (0x10300000)
#define TOP_CLK_RF_REG_BASE         (0x10300200)
#define TOP_CLK_GATE_RF_REG_BASE    (0x10300400)
#define CEN_GLB_APB_REG_BASE        (0x10000000)
#define CEN_PIN_REG_BASE            (0x10200000)
#define SADC_REG_BASE               (0x1c500000)
#define GMAC_REG_BASE               (0x1c600000)
#define EPHY_REG_BASE               (0x1c900000)

#define DMC_SYS_APB_REG_BASE        (0x24000000)
#define DMC_REG_REG_BASE            (0x2C100000)
#define DDR_PHY_REG_BASE            (0x2C200000)
#define DFI_MON_REG_BASE            (0x2C400000)
#define DDR_FW_REG_BASE             (0x2C500000)
#define PERF_MON0_REG_BASE          (0x24800000)
#define PERF_MON1_REG_BASE          (0x24900000)
#define PERF_MON2_REG_BASE          (0x24a00000)

#define VEU_SYS_PIN_REG_BASE    (0x1c100000)
#define IRAM_REG_BASE           (0x1c200000)
#define ISP_SYS_PIN_REG_BASE    (0x2c000000)
#define CSI_SYS_PIN_REG_BASE    (0x2c600000)
#define CPU_SYS_PIN_REG_BASE    (0x0b100000)

#define VEU_SYS_APB_REG_BASE        (0x1C000000)
#define USB2_APB_CFG                            (0x1CA00000)
#define ISP_SYS_APB_REG_BASE        (0x2c100000)
#define PTS_REG_BASE                (0x2c300000)


#define DMAC0_REG_BASE      (0x0C700000)

#define SDC0_REG_BASE       (0x2D100000)
#define SDC1_REG_BASE       (0x2D200000)
#define GICD_BASE           (0x00201000)
#define GICC_BASE           (0x00202000)

#define USB_PHY_REG_BASE    (0x1CA00000)
#define USB_REG_BASE        (0x1D100000)
#define AES_REG_BASE        (0x0C600000)
#define EFUSE_REG_BASE      (0x0A700000)

#define NNA_REG_BASE        (0x1D200000)

#define CPU_CTRL_REG_BASE        (0x01000000)


#define SYS_REG_V2P(va)          (va)
#define SYS_REG_P2V(pa)          (pa)

typedef enum IRQn {
    TMR1_IRQn          = 3,
    I2C0_IRQn          = 15,
    NN_IRQn            = 14,
    I2C1_IRQn          = 16,
    WDT_IRQn           = 21,
    DMAC0_IRQn         = 25,
    PMU_ARC_IRQn       = 26,
    GPIO0_IRQn         = 41,
    GPIO1_IRQn         = 42,
    GPIO2_IRQn         = 43,
    GPIO3_IRQn         = 44,
    GPIO4_IRQn         = 45,
    GPIO5_IRQn         = 46,
    GPIO6_IRQn         = 47,
    EXT_TMR0_IRQn      = 67,
    SADC_IRQn          = 51,
} IRQn_Type;

enum DMA_HW_HS_MAP
{
    UART0_RX = 0,
    UART0_TX,
    UART1_RX,
    UART1_TX,
    I2C0_RX,
    I2C0_TX,
    I2C1_RX,
    I2C1_TX,
    AES_RX,
    AES_TX,
    SPI1_RX,
    SPI1_TX,
    SPI0_RX,
    SPI0_TX,
    ACODEC_TX,
    ACODEC_RX,
    I2S_TX,
    I2S_RX,
    UART2_RX,
    UART2_TX,
    VOU_V0_C,
    VOU_V0_Y,
    VOU_OSD,
    DMIC_R, //23
    DMIC_L, //24
    DMA_HW_HS_END,
};

#define DW_DMA_MAX_NR_CHANNELS 4
#define TIMER_CLOCK     32000

#endif /* ARCH_H_ */
