/**
 * Copyright (c) 2015-2024 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-30     luoyj      the first version
 *
 */

#ifndef __FH_PWM_H__
#define __FH_PWM_H__

struct fh_pwm_config
{
    unsigned int period_ns;
    unsigned int duty_ns;
#define  FH_PWM_PULSE_LIMIT         (0x0)
#define  FH_PWM_PULSE_NOLIMIT       (0x1)
    unsigned int pulses;
    unsigned int pulse_num;
#define FH_PWM_STOPLVL_LOW		(0x0)
#define FH_PWM_STOPLVL_HIGH		(0x3)
#define FH_PWM_STOPLVL_KEEP		(0x1)
    unsigned int stop_module;
    unsigned int delay_ns;
    unsigned int phase_ns;
};

struct fh_pwm_status
{
    unsigned int done_cnt;
    unsigned int total_cnt;
    unsigned int busy;
    unsigned int error;
};

struct fh_pwm_chip_data
{
    int id;
    struct fh_pwm_config config;
    struct fh_pwm_status status;
};

unsigned int fh_pwm_output_enable(unsigned int n);
unsigned int fh_pwm_output_disable(unsigned int n);
unsigned int fh_pwm_output_multi_enable(unsigned int mask);
int fh_pwm_set_config(struct fh_pwm_chip_data *chip_data);
void rt_hw_pwm_init(void);

#endif