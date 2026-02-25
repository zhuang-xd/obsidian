/**
 * Copyright (c) 2015-2024 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-30     luoyj      the first version
 *
*/

#include <rtthread.h>
#include <fh_def.h>
#include <fh_arch.h>
#include "fh_pwm.h"
#include <fh_clock.h>

#define PWM_REG_INTERVAL       (0x80)
#define PWM_MAX_RANGE          (0x03ffffff)

#define OFFSET_PWM_BASE(n)		(PWM_REG_INTERVAL + PWM_REG_INTERVAL * n)

#define OFFSET_PWM_GLOBAL_CTRL0		(0x000)
#define OFFSET_PWM_GLOBAL_CTRL1		(0x004)
#define OFFSET_PWM_GLOBAL_CTRL2		(0x008)
#define OFFSET_PWM_INT_ENABLE		(0x010)
#define OFFSET_PWM_INT_STATUS		(0x014)

#define OFFSET_PWM_CTRL(n)			(0x000 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG0(n)			(0x004 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG1(n)			(0x008 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG2(n)			(0x00c + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG3(n)			(0x010 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG4(n)			(0x014 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS0(n)		(0x020 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS1(n)		(0x024 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS2(n)		(0x028 + OFFSET_PWM_BASE(n))

#define NSEC_PER_SEC 1000000000L

struct fh_pwm_obj
{
	UINT32 base;
	UINT32 input_clock;
};

struct fh_pwm_obj pwm_obj = {
    .base = PWM_REG_BASE,
};

static void fh_pwm_config_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL0);
    reg &= ~(1 << n);
    SET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL0, reg);
}

static void fh_pwm_config_enable(unsigned int n)
{
	unsigned int reg;

	reg = GET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL0);
	reg |= (1 << n);
	SET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL0, reg);
}

static unsigned int _pwm_set_stoptime_bit(int chn)
{
	return (0x1 << (16 + chn));
}

unsigned int fh_pwm_output_enable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL2);
    reg |= (1 << n);
    SET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL2, reg);
    return 0;
}

unsigned int fh_pwm_output_multi_enable(unsigned int mask)
{
    SET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL2, mask);
    return 0;
}

unsigned int fh_pwm_output_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL2);
    reg &= ~(1 << n);
    SET_REG(pwm_obj.base + OFFSET_PWM_GLOBAL_CTRL2, reg);
    return 0;
}

int fh_pwm_set_config(struct fh_pwm_chip_data *chip_data)
{
	unsigned int base = pwm_obj.base;
	unsigned int clk_rate = pwm_obj.input_clock;
	unsigned int period, duty, delay, phase, reg;
	unsigned int inverse = 0;
	unsigned int ctrl = 0;

	fh_pwm_config_disable(chip_data->id);

	period = chip_data->config.period_ns / (NSEC_PER_SEC / clk_rate);
    duty   = chip_data->config.duty_ns / (NSEC_PER_SEC / clk_rate);
    delay  = chip_data->config.delay_ns / (NSEC_PER_SEC / clk_rate);
    phase  = chip_data->config.phase_ns / (NSEC_PER_SEC / clk_rate);

	if (period > PWM_MAX_RANGE) {
		rt_kprintf("PWM: period exceed range\n");
		return -RT_EINVAL;
	}

	if (period == 0x0) {
		rt_kprintf("PWM: period too low\n");
		return -RT_EINVAL;
	}

	if (duty > PWM_MAX_RANGE) {
		rt_kprintf("PWM: duty exceed range\n");
		return -RT_EINVAL;
	}

	if (duty > period) {
		rt_kprintf("PWM: duty is over period\n");
		return -RT_EINVAL;
	}

	/* Calculate Inverse */
	phase = phase % period;
	if ((phase + duty) > period) {
		phase   = phase + duty - period;
		duty    = period - duty;
		inverse = 1;
	}

	rt_kprintf("set period: 0x%x\n", period);
	rt_kprintf("set duty: 0x%x\n", duty);
	rt_kprintf("set phase: 0x%x\n", phase);
	rt_kprintf("set delay: 0x%x\n", delay);

	SET_REG(base + OFFSET_PWM_CFG0(chip_data->id), period);
	SET_REG(base + OFFSET_PWM_CFG1(chip_data->id), duty);
	SET_REG(base + OFFSET_PWM_CFG2(chip_data->id), phase);
	SET_REG(base + OFFSET_PWM_CFG3(chip_data->id), delay);

	if (chip_data->config.delay_ns)
		ctrl |= 1 << 3;

	if (!chip_data->config.pulse_num)
		ctrl |= 1 << 0;

	ctrl |= (chip_data->config.stop_module & 0x3) << 1;

	ctrl |= inverse << 4;

	SET_REG(base + OFFSET_PWM_CTRL(chip_data->id), ctrl);
	rt_kprintf("set ctrl: 0x%x\n", ctrl);

	ctrl = GET_REG(base + OFFSET_PWM_GLOBAL_CTRL1);

	reg = (chip_data->config.stop_module >> 4) & 0x1;
	if (reg)
		ctrl |= _pwm_set_stoptime_bit(chip_data->id);
	else
		ctrl &= _pwm_set_stoptime_bit(chip_data->id);

	SET_REG(base + OFFSET_PWM_GLOBAL_CTRL1, ctrl);

	SET_REG(base + OFFSET_PWM_CFG4(chip_data->id), chip_data->config.pulse_num);
	rt_kprintf("set pulses: 0x%x\n", chip_data->config.pulse_num);

	fh_pwm_config_enable(chip_data->id);
	return 0;
}

int fh_pwm_probe(void *data)
{
	struct fh_pwm_obj *pwm_obj = (struct fh_pwm_obj *)data;
	struct clk *clk_rate = RT_NULL;

	clk_rate = clk_get(RT_NULL, "pwm_clk");
	if (clk_rate == RT_NULL) {
        rt_kprintf("ERROR: %s, clk_get error\n", __func__);
		return -1;
	} else {
		clk_enable(clk_rate);
		pwm_obj->input_clock = clk_rate->clk_out_rate;
	}
	return 0;
}

void rt_hw_pwm_init(void)
{
	fh_pwm_probe(&pwm_obj);
}