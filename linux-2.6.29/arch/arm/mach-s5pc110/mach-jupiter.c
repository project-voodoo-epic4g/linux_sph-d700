/* linux/arch/arm/mach-s5pc110/mach-jupiter.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/max8998.h>
#if 1//CONFIG_ARIES_VER_B1... pfe
#include <linux/regulator/max8893.h>
#endif//CONFIG_ARIES_VER_B1... pfe
#include <linux/regulator/pmic_cam.h>
#include <linux/leds.h>

//#include <linux/delay.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/videodev2.h>

#include <media/ce147_platform.h>
#include <media/s5ka3dfx_platform.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/setup.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/gpio.h>
#include <mach/param.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-rtc.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/fb.h>
#include <plat/fimc-ipc.h>
#include <plat/spi.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5pc110.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <plat/regs-fimc.h>
/* pmem */
#include <linux/android_pmem.h>
#include <plat/media.h>

#include <mach/max8998_function.h>

//Thomas Ryu 20100512, from Garnett
#include <linux/reboot.h>

#ifdef CONFIG_KERNEL_DEBUG_SEC
#include <linux/kernel_sec_common.h>
#endif


#if defined(CONFIG_SEC_HEADSET)
#include <mach/sec_jack.h>
#endif

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);


void jupiter_init_gpio(void);

extern void s5pc11x_reserve_bootmem(void);
extern void universal_sdhci2_set_platdata(void);

extern unsigned char maxim_chg_status(void);
extern void kernel_sec_hw_reset(bool bSilentReset);
extern void kernel_sec_clear_upload_magic_number(void);


#include "mach-common.c"



#if defined(CONFIG_TOUCHSCREEN_QT602240) || defined(CONFIG_ARIES_VER_B1)
static struct platform_device s3c_device_qtts = {
	.name = "qt602240-ts",
	.id = -1,
};
//#endif
#else
//#if defined(CONFIG_TOUCHSCREEN_MELFAS) //victory.boot
static struct platform_device s3c_device_melfasts = {
	.name = "melfas-ts",
	.id = -1,
};
#endif

struct platform_device sec_device_battery = {
	.name   = "sec-battery",
	.id		= -1,
};

#if 1 // hanapark SVC LED
struct gpio_led leds_gpio[] = {
	{
		.name = "red",
		.default_trigger = "default-on",	// hanapark DF15: Turn ON RED LED at boot time !
		.gpio = GPIO_SVC_LED_RED,
		.active_low = 0,
	},
	{
		.name = "blue",
		.default_trigger = NULL,
		.gpio = GPIO_SVC_LED_BLUE,
		.active_low = 0,
	}
};


struct gpio_led_platform_data leds_gpio_platform_data = {
	.num_leds = ARRAY_SIZE(leds_gpio),
	.leds = leds_gpio,
};


struct platform_device sec_device_leds_gpio = {
	.name   = "leds-gpio",
	.id		= -1,
	.dev = { .platform_data = &leds_gpio_platform_data },
};
#endif // hanapark SVC LED

/* PMIC */
static struct regulator_consumer_supply dcdc1_consumers[] = {
	{
		.supply		= "vddarm",
	},
};

static struct regulator_init_data max8998_dcdc1_data = {
	.constraints	= {
		.name		= "VCC_ARM",
		.min_uV		=  750000,
		.max_uV		= 1500000,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= ARRAY_SIZE(dcdc1_consumers),
	.consumer_supplies	= dcdc1_consumers,
};

static struct regulator_consumer_supply dcdc2_consumers[] = {
	{
		.supply		= "vddint",
	},
};

static struct regulator_init_data max8998_dcdc2_data = {
	.constraints	= {
		.name		= "VCC_INTERNAL",
		.min_uV		=  750000,
		.max_uV		= 1500000,
		.always_on	= 1,
//		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= ARRAY_SIZE(dcdc2_consumers),
	.consumer_supplies	= dcdc2_consumers,
};

static struct regulator_init_data max8998_dcdc4_data = {
	.constraints	= {
		.name		= "DCDC4",
		.min_uV		=  800000,
		.max_uV		= 2300000,
		//.always_on	= 1,
//		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
};


static struct regulator_init_data max8998_ldo4_data = {
        .constraints    = {
                .name           = "VCC_DAC",
                .min_uV         = 3300000,
                .max_uV         = 3300000,
		.always_on	= 1,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

#if 1 //20100506_inchul...
static struct regulator_init_data max8998_ldo6_data = {
        .constraints    = {
                .name           = "VBT_WL_2.6V",
                .min_uV         = 2800000,
                .max_uV         = 2800000,
		.always_on	= 1,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};
#endif

static struct regulator_init_data max8998_ldo11_data = {
        .constraints    = {
                .name           = "CAM_IO_2.8V",
                .min_uV         = 2800000,
                .max_uV         = 2800000,
		//.always_on	= 1,
               // .apply_uV       = 1,
		//.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo12_data = {
        .constraints    = {
                .name           = "CAM_5M_1.8V",
                .min_uV         = 1800000,
                .max_uV         = 1800000,
		//.always_on	= 1,
                //.apply_uV       = 1,
		//.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo13_data = {
        .constraints    = {
                .name           = "CAM_A_2.8V",
                .min_uV         = 2800000,
                .max_uV         = 2800000,
		//.always_on	= 1,
                //.apply_uV       = 1,
		//.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo14_data = {
        .constraints    = {
                .name           = "CAM_CIF_1.8V",
                .min_uV         = 1800000,
                .max_uV         = 1800000,
		//.always_on	= 1,
                //.apply_uV       = 1,
		//.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo15_data = {
        .constraints    = {
                .name           = "CAM_AF_2.8V",
                .min_uV         = 2800000,
                .max_uV         = 2800000,
		//.always_on	= 1,
                //.apply_uV       = 1,
		//.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

#if 1 //20100514_inchul.. To add the MIPI_1.8V_C110(LDO16) control (from HW)
static struct regulator_init_data max8998_ldo16_data = {
        .constraints    = {
                .name           = "MIPI_1.8V",
                .min_uV         = 1800000,
                .max_uV         = 1800000,
		.always_on	= 1,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};
#endif

static struct regulator_init_data max8998_ldo7_data = {
        .constraints    = {
                .name           = "VCC_LCD",
                .min_uV         = 1800000, //victory.boot
                .max_uV         = 1800000, //victory.boot
		.always_on	= 1,
                //.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo17_data = {
        .constraints    = {
                .name           = "PM_LVDS_VDD",
                .min_uV         = 2800000, //victory.boot
                .max_uV         = 2800000, //victory.boot
		.always_on	= 1,
                //.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};


static struct max8998_subdev_data universal_regulators[] = {
	{ MAX8998_DCDC1, &max8998_dcdc1_data },
	{ MAX8998_DCDC2, &max8998_dcdc2_data },
//	{ MAX8998_DCDC4, &max8998_dcdc4_data },
	{ MAX8998_LDO4, &max8998_ldo4_data },
#if 0 //20100528_inchul...
	{ MAX8998_LDO6, &max8998_ldo6_data },
#endif
//	{ MAX8998_LDO11, &max8998_ldo11_data },
//	{ MAX8998_LDO12, &max8998_ldo12_data },
//	{ MAX8998_LDO13, &max8998_ldo13_data },
//	{ MAX8998_LDO14, &max8998_ldo14_data },
//	{ MAX8998_LDO15, &max8998_ldo15_data },
#if 1 //20100514_inchul.. To add the MIPI_1.8V_C110(LDO16) control (from HW)
	{ MAX8998_LDO17, &max8998_ldo17_data },
	{ MAX8998_LDO16, &max8998_ldo16_data },
#endif
	{ MAX8998_LDO7, &max8998_ldo7_data },
};

static struct max8998_platform_data max8998_platform_data = {
	.num_regulators	= ARRAY_SIZE(universal_regulators),
	.regulators	= universal_regulators,
};


/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8998", (0xCC >> 1)),
		.platform_data = &max8998_platform_data,
	},
};

struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id		= -1,
};
struct platform_device s3c_device_8998consumer = {
        .name             = "max8998-consumer",
        .id               = 0,
  	.dev = { .platform_data = &max8998_platform_data },
};

struct platform_device	sec_device_rfkill = {
	.name = "bt_rfkill",
	.id	  = -1,
};

struct platform_device	sec_device_btsleep = {
	.name = "bt_sleep",
	.id	  = -1,
};


#if 1//CONFIG_ARIES_VER_B1... pfe
static struct regulator_init_data max8893_ldo1_data = {
        .constraints    = {
                .name           = "WIMAX_2.9V",
                .min_uV         = 2900000,
                .max_uV         = 2900000,
		.always_on	= 0,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8893_ldo2_data = {
        .constraints    = {
                .name           = "TOUCH_KEY_3.0V",
                .min_uV         = 3000000,   //20100628_inchul(from HW) 2.8V -> 3.0V
                .max_uV         = 3000000,  //20100628_inchul(from HW) 2.8V -> 3.0V
		.always_on	= 0,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8893_ldo3_data = {
        .constraints    = {
                .name           = "VCC_MOTOR_3.0V",
                .min_uV         = 3000000,
                .max_uV         = 3000000,
		.always_on	= 0,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8893_ldo4_data = {
        .constraints    = {
                .name           = "WIMAX_SDIO_3.0V",
                .min_uV         = 3000000,
                .max_uV         = 3000000,
		.always_on	= 0,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8893_ldo5_data = {
        .constraints    = {
                .name           = "VDD_RF & IO_1.8V",
                .min_uV         = 1800000,
                .max_uV         = 1800000,
		.always_on	= 0,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8893_buck_data = {
        .constraints    = {
                .name           = "VDDA & SDRAM & WIMAX_USB & RFC1C4_1.8V",
                .min_uV         = 1800000,
                .max_uV         = 1800000,
		.always_on	= 1,
                .apply_uV       = 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct max8893_subdev_data universal_8893_regulators[] = {
	{ MAX8893_LDO1, &max8893_ldo1_data },
	{ MAX8893_LDO2, &max8893_ldo2_data },
  { MAX8893_LDO3, &max8893_ldo3_data },
	{ MAX8893_LDO4, &max8893_ldo4_data },
  { MAX8893_LDO5, &max8893_ldo5_data },
  { MAX8893_BUCK, &max8893_buck_data },
};

static struct max8893_platform_data max8893_platform_data = {
	.num_regulators	= ARRAY_SIZE(universal_8893_regulators),
	.regulators	= universal_8893_regulators,
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("max8893", (0x3E)),
		.platform_data = &max8893_platform_data,
	},
};

struct platform_device s3c_device_8893consumer = {
        .name             = "max8893-consumer",
        .id               = 0,
  	.dev = { .platform_data = &max8893_platform_data },
};

#endif//CONFIG_ARIES_VER_B1... pfe

/*
void universal_wm8994_init(void)
{
 	int err;
	u32 temp;
	temp = __raw_readl(S5P_CLK_OUT);

	temp &= 0xFFFE0FFF; //clear bits 12~16
#ifdef CONFIG_SND_UNIVERSAL_WM8994_MASTER
        temp |= (0x11 << 12); //crystall
#else
	temp |= (0x02 << 12); // epll
	temp |= (0x5 << 20);
#endif

	printk("CLKOUT reg is %x\n",temp);
	 __raw_writel(temp, S5P_CLK_OUT);
	temp = __raw_readl(S5P_CLK_OUT);
	printk("CLKOUT reg is %x\n",temp);


	err = gpio_request( S5PC11X_MP03(6), "CODEC_LDO_EN");

	if (err) {
		printk(KERN_ERR "failed to request MP03(6) for "
			"codec control\n");
		return;
	}
	udelay(50);
	gpio_direction_output( S5PC11X_MP03(6) , 1);
        gpio_set_value(S5PC11X_MP03(6), 1);
	udelay(50);
        gpio_set_value(S5PC11X_MP03(6), 0);
	udelay(50);
        gpio_set_value(S5PC11X_MP03(6), 1);



	//select the headset??
	 err = gpio_request( S5PC11X_GPJ3(1), "EAR3.5_SW");

        if (err) {
                printk(KERN_ERR "failed to request MP03(6) for "
                        "codec control\n");
                return;
        }
	gpio_direction_output( S5PC11X_GPJ3(1) , 1);
        gpio_set_value(S5PC11X_GPJ3(1), 0);

	temp =	__raw_readl(S5PC11X_MP05CON);

	err = gpio_request( S5PC11X_GPJ4(2), "MIC_BIAS_IN");

        if (err) {
                printk(KERN_ERR "failed to request gpioj4-2 "
                        "codec control\n");
                return;
        }
        gpio_direction_output( S5PC11X_GPJ4(2) , 1);
}
*/


#ifdef CONFIG_FB_S3C_TL2796
#if defined(CONFIG_FB_S3C_TL2796_DUAL)
extern void s5pc110_sublcd_cfg_gpio(void);
extern int s5pc110_sublcd_on(void);
extern int s5pc110_sublcd_reset(void);
#endif



static void tl2796_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	#ifndef CONFIG_FB_S3C_MDNIE
	writel(0x2, S5P_MDNIE_SEL);
	#endif
	/* drive strength to max */
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x12c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x14c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x16c);
	writel(0x000000ff, S5PC11X_VA_GPIO + 0x18c);

	/* DISPLAY_CS */
	s3c_gpio_cfgpin(S5PC11X_MP01(1), S3C_GPIO_SFN(1));
	/* DISPLAY_CLK */
	s3c_gpio_cfgpin(S5PC11X_MP04(1), S3C_GPIO_SFN(1));
	/* DISPLAY_SO */
	s3c_gpio_cfgpin(S5PC11X_MP04(2), S3C_GPIO_SFN(1));
	/* DISPLAY_SI */
	s3c_gpio_cfgpin(S5PC11X_MP04(3), S3C_GPIO_SFN(1));

	/* DISPLAY_CS */
	s3c_gpio_setpull(S5PC11X_MP01(1), S3C_GPIO_PULL_NONE);
	/* DISPLAY_CLK */
	s3c_gpio_setpull(S5PC11X_MP04(1), S3C_GPIO_PULL_NONE);
	/* DISPLAY_SO */
	s3c_gpio_setpull(S5PC11X_MP04(2), S3C_GPIO_PULL_NONE);
	/* DISPLAY_SI */
	s3c_gpio_setpull(S5PC11X_MP04(3), S3C_GPIO_PULL_NONE);

	/*KGVS : configuring GPJ2(4) as FM interrupt */
	//s3c_gpio_cfgpin(S5PC11X_GPJ2(4), S5PC11X_GPJ2_4_GPIO_INT20_4); //jihyon82.kim for proximity sensor

#if defined(CONFIG_FB_S3C_TL2796_DUAL)
	/* SUB LCD */
	s5pc110_sublcd_cfg_gpio();
#endif
}

static int tl2796_backlight_on(struct platform_device *pdev)
{
	int err;
#if 0 //victory.boot
#if (defined CONFIG_JUPITER_VER_B4)
	err = gpio_request(S5PC11X_GPJ1(3), "MLCD_ON");

	if (err) {
		printk(KERN_ERR "failed to request GPJ1 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPJ1(3), 1);
	gpio_set_value(S5PC11X_GPJ1(3), 1);//ksoo add (2009.09.07)
	gpio_free(S5PC11X_GPJ1(3));



#if defined(CONFIG_FB_S3C_TL2796_DUAL)
	/* SUB LCD */
	s5pc110_sublcd_on();
#endif
#endif
#endif
	return 0;
}

static int tl2796_reset_lcd(struct platform_device *pdev)
{
	int err;

//  Ver1 & Ver2 universal board kyoungheon
	err = gpio_request(S5PC11X_MP05(5), "MLCD_RST");
        if (err) {
                printk(KERN_ERR "failed to request MP0(5) for "
                        "lcd reset control\n");
                return err;
        }

        gpio_direction_output(S5PC11X_MP05(5), 1);
        msleep(10);

        gpio_set_value(S5PC11X_MP05(5), 0);
        msleep(10);

        gpio_set_value(S5PC11X_MP05(5), 1);
        msleep(10);

        gpio_free(S5PC11X_MP05(5));

	return 0;
}


static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,

	.cfg_gpio = tl2796_cfg_gpio,
	.backlight_on = tl2796_backlight_on,
	.reset_lcd = tl2796_reset_lcd,
};

#define LCD_BUS_NUM 	3
#define DISPLAY_CS	S5PC11X_MP01(1)
#define SUB_DISPLAY_CS	S5PC11X_MP01(2)
#define DISPLAY_CLK	S5PC11X_MP04(1)
#define DISPLAY_SI	S5PC11X_MP04(3)


static struct spi_board_info spi_board_info[] __initdata = {
    	{
	    	.modalias	= "tl2796",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect	= 2,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};
#endif

#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1) || defined(CONFIG_SPI_CNTRLR_2)
static void s3c_cs_suspend(int pin, pm_message_t pm)
{
        /* Whatever need to be done */
}

static void s3c_cs_resume(int pin)
{
        /* Whatever need to be done */
}

static void s3c_cs_set(int pin, int lvl)
{
        if(lvl == CS_HIGH)
           s3c_gpio_setpin(pin, 1);
        else
           s3c_gpio_setpin(pin, 0);
}

static void s3c_cs_config(int pin, int mode, int pull)
{
        s3c_gpio_cfgpin(pin, mode);

        if(pull == CS_HIGH)
           s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
        else
           s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif

#if defined(CONFIG_SPI_CNTRLR_0)
static struct s3c_spi_pdata s3c_slv_pdata_0[] __initdata = {
        [0] = { /* Slave-0 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPB(1),
                .cs_mode      = S5PC11X_GPB_OUTPUT(1),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
        [1] = { /* Slave-1 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPA1(1),
                .cs_mode      = S5PC11X_GPA1_OUTPUT(1),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
};
#endif

#if defined(CONFIG_SPI_CNTRLR_1)
static struct s3c_spi_pdata s3c_slv_pdata_1[] __initdata = {
        [0] = { /* Slave-0 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPB(5),
                .cs_mode      = S5PC11X_GPB_OUTPUT(5),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
        [1] = { /* Slave-1 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPA1(3),
                .cs_mode      = S5PC11X_GPA1_OUTPUT(3),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
};
#endif

#if defined(CONFIG_SPI_CNTRLR_2)
static struct s3c_spi_pdata s3c_slv_pdata_2[] __initdata = {
        [0] = { /* Slave-0 */
                .cs_level     = CS_FLOAT,
                .cs_pin       = S5PC11X_GPH1(0),
                .cs_mode      = S5PC11X_GPH1_OUTPUT(0),
                .cs_set       = s3c_cs_set,
                .cs_config    = s3c_cs_config,
                .cs_suspend   = s3c_cs_suspend,
                .cs_resume    = s3c_cs_resume,
        },
};
#endif

static struct spi_board_info s3c_spi_devs[] __initdata = {
#if defined(CONFIG_SPI_CNTRLR_0)
        [0] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 100000,
                /* Connected to SPI-0 as 1st Slave */
                .bus_num         = 0,
                .irq             = IRQ_SPI0,
                .chip_select     = 0,
        },
        [1] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 100000,
                /* Connected to SPI-0 as 2nd Slave */
                .bus_num         = 0,
                .irq             = IRQ_SPI0,
                .chip_select     = 1,
        },
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
        [2] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 100000,
                /* Connected to SPI-1 as 1st Slave */
                .bus_num         = 1,
                .irq             = IRQ_SPI1,
                .chip_select     = 0,
        },
        [3] = {
                .modalias        = "spidev", /* Test Interface */
                .mode            = SPI_MODE_0 | SPI_CS_HIGH,    /* CPOL=0, CPHA=0 & CS is Active High */
                .max_speed_hz    = 100000,
                /* Connected to SPI-1 as 3rd Slave */
                .bus_num         = 1,
                .irq             = IRQ_SPI1,
                .chip_select     = 1,
        },
#endif

#if defined(CONFIG_SPI_CNTRLR_2)
/* For MMC-SPI using GPIO BitBanging. MMC connected to SPI CNTRL 2 as slave 0. */
#if defined (CONFIG_MMC_SPI_GPIO)
#define SPI_GPIO_DEVNUM 4
#define SPI_GPIO_BUSNUM 2

        [SPI_GPIO_DEVNUM] = {
                .modalias        = "mmc_spi", /* MMC SPI */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 400000,
                /* Connected to SPI-0 as 1st Slave */
                .bus_num         = SPI_GPIO_BUSNUM,
                .chip_select     = 0,
                .controller_data = S5PC11X_GPH1(0),
        },
#else
        [4] = {
                .modalias        = "mmc_spi", 	/* MMC SPI */
                .mode            = SPI_MODE_0,  /* CPOL=0, CPHA=0 */
                .max_speed_hz    = 400000,
                /* Connected to SPI-2 as 1st Slave */
                .bus_num         = 2,
                .irq             = IRQ_SPI2,
                .chip_select     = 0,
        },
#endif
#endif
};

#if defined(CONFIG_MMC_SPI_GPIO)
#define SPI_GPIO_ID 2
struct spi_gpio_platform_data s3c_spigpio_pdata = {
        .sck = S5PC11X_GPG2(0),
        .miso = S5PC11X_GPG2(2),
        .mosi = S5PC11X_GPG2(3),
        .num_chipselect = 1,
};

/* Generic GPIO Bitbanging contoller */
struct platform_device s3c_device_spi_bitbang = {
        .name           = "spi_gpio",
        .id             = SPI_GPIO_ID,
        .dev            = {
                .platform_data = &s3c_spigpio_pdata,
        }
};
#endif



static	struct	i2c_gpio_platform_data	i2c4_platdata = {
	.sda_pin		= GPIO_AP_SDA_18V,
	.scl_pin		= GPIO_AP_SCL_18V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
//	.scl_is_output_only	= 1,
};

static struct platform_device s3c_device_i2c4 = {
	.name				= "i2c-gpio",
	.id					= 4,
	.dev.platform_data	= &i2c4_platdata,
};


//etinum.victory.boot
static	struct	i2c_gpio_platform_data	i2c5_platdata = {
	.sda_pin		= AP_SDA_30V,
	.scl_pin		= AP_SCL_30V,
	.udelay			= 2,	/* 250KHz */
//	.udelay			= 4,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
//	.scl_is_output_only	= 1,
};

static struct platform_device s3c_device_i2c5 = {
	.name				= "i2c-gpio",
	.id					= 5,
	.dev.platform_data	= &i2c5_platdata,
};


static	struct	i2c_gpio_platform_data	i2c6_platdata = {
	.sda_pin		= GPIO_AP_PMIC_SDA,
	.scl_pin		= GPIO_AP_PMIC_SCL,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c6 = {
	.name				= "i2c-gpio",
	.id					= 6,
	.dev.platform_data	= &i2c6_platdata,
};

#if 0 //suik_Check
#if 1 //etinum.victory.keyboard
#include <linux/max7359_keypad.h>

#define KEYCODE_EX(row,column,code) (((row&0xf)<<28)|((column&0xf)<<24)|(code&0xffff))
#define KEYCODE(row,column) (((row&0xf)<<28)|((column&0xf)<<24)|((row*8+column+1)&0xffff))
static unsigned int keypad_keycode[] = {

KEYCODE(0,0), KEYCODE(0,1), KEYCODE(0,2), KEYCODE(0,3), KEYCODE(0,4), KEYCODE(0,5), KEYCODE(0,6), KEYCODE(0,7),
KEYCODE(1,0), KEYCODE(1,1), KEYCODE(1,2), KEYCODE(1,3), KEYCODE(1,4), KEYCODE(1,5), KEYCODE(1,6), KEYCODE(1,7),
KEYCODE(2,0), KEYCODE(2,1), KEYCODE(2,2), KEYCODE(2,3), KEYCODE(2,4), KEYCODE(2,5), KEYCODE(2,6), KEYCODE(2,7),
KEYCODE(3,0), KEYCODE(3,1), KEYCODE(3,2), KEYCODE(3,3), KEYCODE(3,4), KEYCODE(3,5), KEYCODE(3,6), KEYCODE(3,7),
KEYCODE(4,0), KEYCODE(4,1), KEYCODE(4,2), KEYCODE(4,3), KEYCODE(4,4), KEYCODE(4,5), KEYCODE(4,6), KEYCODE(4,7),
KEYCODE(5,0), KEYCODE(5,1), KEYCODE(5,2), KEYCODE(5,3), KEYCODE(5,4), KEYCODE(5,5), KEYCODE(5,6), KEYCODE(5,7),
KEYCODE(6,0), KEYCODE(6,1), KEYCODE(6,2), KEYCODE(6,3), KEYCODE(6,4), KEYCODE(6,5), KEYCODE(6,6), KEYCODE(6,7),
KEYCODE(7,0), KEYCODE(7,1), KEYCODE(7,2), KEYCODE(7,3), KEYCODE(7,4), KEYCODE(7,5), KEYCODE(7,6), KEYCODE(7,7),

};

static struct max7359_keypad_platform_data max7359_kbd_platform_data = {
    .key_map = &keypad_keycode[0],
    .key_map_size = ARRAY_SIZE(keypad_keycode),
};

static struct i2c_board_info max7359_kbd_i2c_info[] __initdata = {
    {
    	I2C_BOARD_INFO("max7359", 0x70>>1), //for victory, ad0 is grounded, so the slave address is 0x70
    	.platform_data = &max7359_kbd_platform_data,
    	.irq           = IRQ_EINT(27),
    },
};

static	struct	i2c_gpio_platform_data	i2c7_platdata = {
	.sda_pin		= GPIO_KEY_I2C_SDA,
	.scl_pin		= GPIO_KEY_I2C_SCL,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};
#else
static	struct	i2c_gpio_platform_data	i2c7_platdata = {
	.sda_pin		= GPIO_USB_SDA_28V,
	.scl_pin		= GPIO_USB_SCL_28V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};
#endif


static struct platform_device s3c_device_i2c7 = {
	.name				= "i2c-gpio",
	.id					= 7,
	.dev.platform_data	= &i2c7_platdata,
};
#endif

// For FM radio
#if 0
static	struct	i2c_gpio_platform_data	i2c8_platdata = {
	.sda_pin		= GPIO_FM_SDA_28V,
	.scl_pin		= GPIO_FM_SCL_28V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};


static struct platform_device s3c_device_i2c8 = {
	.name				= "i2c-gpio",
	.id					= 8,
	.dev.platform_data	= &i2c8_platdata,
};
#endif

#if 1//CONFIG_ARIES_VER_B1... pfe
static	struct	i2c_gpio_platform_data	i2c9_platdata = {
	.sda_pin		= GPIO_WIMAX_PM_SDA,
	.scl_pin		= GPIO_WIMAX_PM_SCL,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c9 = {
	.name				= "i2c-gpio",
	.id					= 9,
	.dev.platform_data	= &i2c9_platdata,
};
#endif//CONFIG_ARIES_VER_B1... pfe

#ifdef CONFIG_ARIES_VER_B1

static	struct	i2c_gpio_platform_data	i2c11_platdata = {
  .sda_pin		= GPIO_FM_SDA_28V,
  .scl_pin    = GPIO_FM_SCL_28V,
  .udelay     = 2,  /* 250KHz */
  .sda_is_open_drain  = 0,
  .scl_is_open_drain  = 0,
  .scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c11 = {
  .name       = "i2c-gpio",
  .id         = 11,
  .dev.platform_data  = &i2c11_platdata,
};//jihyon82.kim for gp2a

//hojun_kim begin [
static  struct  i2c_gpio_platform_data  i2c13_platdata = {
  .sda_pin    = _3_TOUCH_SDA_28V,
  .scl_pin    = _3_TOUCH_SCL_28V,
  .udelay     = 0,  /* 250KHz */
  .sda_is_open_drain  = 0,
  .scl_is_open_drain  = 0,
  .scl_is_output_only = 0,
};
static struct platform_device s3c_device_i2c13 = {
	.name				= "i2c-gpio",
	.id					= 13,
	.dev.platform_data	= &i2c13_platdata,
};
//hojun_kim end ]

// hanapark begin [
// Fuel Gauge I2C Driver : Change ID 9 -> 14 (fuel_gauge.c)
#if defined CONFIG_ARIES_VER_B1 || !defined ( CONFIG_ARIES_VER_B1 )
static	struct	i2c_gpio_platform_data	i2c14_platdata = {
	.sda_pin		= FUEL_SDA_18V,
	.scl_pin		= FUEL_SCL_18V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c14 = {
	.name				= "i2c-gpio",
	.id					= 14,
	.dev.platform_data	= &i2c14_platdata,
};
// hanapark end ]
#endif

#if 0 //victory.boot
static	struct	i2c_gpio_platform_data	i2c10_platdata = {
	.sda_pin		= _3_TOUCH_SDA_28V,
	.scl_pin		= _3_TOUCH_SCL_28V,
	.udelay			= 0,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c10 = {
	.name				= "i2c-gpio",
	.id					= 10,
	.dev.platform_data	= &i2c10_platdata,
};

static	struct	i2c_gpio_platform_data	i2c11_platdata = {
	.sda_pin		= GPIO_ALS_SDA_28V,
	.scl_pin		= GPIO_ALS_SCL_28V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c12 = {
	.name				= "i2c-gpio",
	.id					= 12,
	.dev.platform_data	= &i2c12_platdata,
};

#endif
#endif

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
};

#if defined(CONFIG_SEC_HEADSET)
static struct sec_jack_port sec_jack_port[] = {
        {
    		{ // HEADSET detect info
    			.eint		=IRQ_EINT6,
    			.gpio		= GPIO_DET_35,
    			.gpio_af	= GPIO_DET_35_AF,
    			.low_active 	= 0
    		},
    		{ // short SEND/END info s
    			.eint		= IRQ_EINT(30),
    			.gpio		= GPIO_EAR_SEND_END_SHORT,
    			.gpio_af	= GPIO_EAR_SEND_END_SHORT_AF,
    			.low_active	= 1
    		},
    		{
    			.eint		= S3C_GPIOINT(D0,0),  //suik_Fix
    			.gpio		= S5PC11X_GPD0(0),
    			.gpio_af	= GPIO_EAR_SEND_END_OPEN_AF,
    			.low_active	= 0
    		}
        }
};

static struct sec_jack_platform_data sec_jack_data = {
        .port           = sec_jack_port,
        .nheadsets      = ARRAY_SIZE(sec_jack_port),
};

static struct platform_device sec_device_jack = {
        .name           = "sec_jack",
        .id             = -1,
        .dev            = {
                .platform_data  = &sec_jack_data,
        },
};
#endif // CONFIG_SEC_HEADSET


static struct platform_device *smdkc110_devices[] __initdata = {
	&s3c_device_fb,
	&s3c_device_mfc,
#ifdef CONFIG_FB_S3C_TL2796
	&s3c_device_spi_gpio,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
        &s3c_device_hsmmc0,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
        &s3c_device_hsmmc1,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
        &s3c_device_hsmmc2,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
        &s3c_device_hsmmc3,
#endif

#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif

#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
#endif
#ifdef CONFIG_RTC_DRV_S3C
	&s3c_device_rtc,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],

#endif
#ifdef CONFIG_SPI_CNTRLR_0
        &s3c_device_spi0,
#endif
#ifdef CONFIG_SPI_CNTRLR_1
        &s3c_device_spi1,
#endif
#ifdef CONFIG_MMC_SPI_GPIO	//for mmc-spi gpio bitbanging
      	&s3c_device_spi_bitbang,
#elif defined(CONFIG_SPI_CNTRLR_2)
	&s3c_device_spi2,
#endif
	&s3c_device_usbgadget,
	&s3c_device_keypad,
#if defined(CONFIG_TOUCHSCREEN_QT602240) || defined(CONFIG_ARIES_VER_B1)
	&s3c_device_qtts,
//#endif
#else
//#if defined(CONFIG_TOUCHSCREEN_MELFAS) //victory.boot
	&s3c_device_melfasts,
#endif
    &s5p_trs_detect, //mkh
	&sec_device_dpram,
	&s3c_device_adc,
	&s3c_device_cfcon,
	&s5p_device_tvout,
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	#if defined(CONFIG_SEC_HEADSET)
	&sec_device_jack,
	#endif
	&s3c_device_csis,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c2,
	&s3c_device_ipc,
	&s3c_device_jpeg,
	&s3c_device_i2c4,
	&s3c_device_i2c5,
	&s3c_device_i2c6,
//	&s3c_device_i2c7,
//	&s3c_device_i2c8,
#if 1//CONFIG_ARIES_VER_B1... pfe
	&s3c_device_i2c9,
#endif//CONFIG_ARIES_VER_B1... pfe
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 )
#if 0 //victory.boot
	&s3c_device_i2c10,
       &s3c_device_i2c12,
#endif
#endif
#ifdef CONFIG_ARIES_VER_B1
	&s3c_device_i2c11,
	&s3c_device_i2c13, //hojun_kim
	&s3c_device_i2c14, // hanapark (fuel gauge i2c driver)
#endif
	&opt_gp2a,
	&s3c_device_8998consumer,
    &s3c_device_tsi,
	&sec_device_rfkill,
	&sec_device_btsleep,
	&sec_device_battery,
	&sec_device_leds_gpio,	// hanapark
#if 1//CONFIG_ARIES_VER_B1... pfe
  &s3c_device_8893consumer,
#endif//CONFIG_ARIES_VER_B1... pfe
};
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
        /* s5pc100 supports 12-bit resolution */
        .delay  = 10000,
        .presc  = 49,
        .resolution = 12,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
};

#if 0//CONFIG_ARIES_VER_B1... pfe
static struct i2c_board_info i2c_devs1[] __initdata = {
};
#endif

static int read_hwversion(void)
{
	int err;
	int hwver = -1;
	int hwver_0 = -1;
	int hwver_1 = -1;
	int hwver_2 = -1;

#if 1 //victory.boot temp
    hwver = 2;
#else
	err = gpio_request(S5PC11X_GPJ0(2), "HWREV_MODE0");

	if (err) {
		printk(KERN_ERR "failed to request GPJ0(2) for "
			"HWREV_MODE0\n");
		return err;
	}
	err = gpio_request(S5PC11X_GPJ0(3), "HWREV_MODE1");

	if (err) {
		printk(KERN_ERR "failed to request GPJ0(3) for "
			"HWREV_MODE1\n");
		return err;
	}
	err = gpio_request(S5PC11X_GPJ0(4), "HWREV_MODE2");

	if (err) {
		printk(KERN_ERR "failed to request GPJ0(4) for "
			"HWREV_MODE2\n");
		return err;
	}

	gpio_direction_input(S5PC11X_GPJ0(2));
	gpio_direction_input(S5PC11X_GPJ0(3));
	gpio_direction_input(S5PC11X_GPJ0(4));

	hwver_0 = gpio_get_value(S5PC11X_GPJ0(2));
	hwver_1 = gpio_get_value(S5PC11X_GPJ0(3));
	hwver_2 = gpio_get_value(S5PC11X_GPJ0(4));

	gpio_free(S5PC11X_GPJ0(2));
	gpio_free(S5PC11X_GPJ0(3));
	gpio_free(S5PC11X_GPJ0(4));

	if((hwver_0 == 0)&&(hwver_1 == 1)&&(hwver_2 == 0)){
		hwver = 1;
		printk("+++++++++[B4 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
	}
	else if((hwver_0 == 1)&&(hwver_1 == 0)&&(hwver_2 == 1)){
		hwver = 2;
		printk("+++++++++[B5 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
	}
	else if((hwver_0 == 0)&&(hwver_1 == 1)&&(hwver_2 == 1)){
		hwver = 2;
		printk("+++++++++[ARIES B5 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
	}
	else{
		hwver = 0;
		//printk("+++++++++[B2, B3 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
	}
#endif

	return hwver;
}


#ifdef CONFIG_VIDEO_CE147
/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/
static void ce147_ldo_en(bool onoff)
{
	int hwver = -1;

	hwver = read_hwversion();

	if(hwver == 2){		//B5 board
		if(onoff){
			Set_MAX8998_PM_OUTPUT_Voltage(BUCK4, VCC_1p200);  //CAM_ISP_1.2V
			Set_MAX8998_PM_REG(EN4, 1);
			Set_MAX8998_PM_OUTPUT_Voltage(LDO11, VCC_2p800);  //CAM_AF_2.8V //SecFeature.Camera aswoogi
			Set_MAX8998_PM_REG(ELDO11, 1);
			Set_MAX8998_PM_OUTPUT_Voltage(LDO12, VCC_1p200);  //CAM_SENSOR_1.2V
			Set_MAX8998_PM_REG(ELDO12, 1);
			Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);  //CAM_SENSOR_A2.8V
			Set_MAX8998_PM_REG(ELDO13, 1);
			Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);  //CAM_ISP_1.8V
			Set_MAX8998_PM_REG(ELDO14, 1);
			Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);  //CAM_ISP_2.8V
			Set_MAX8998_PM_REG(ELDO15, 1);
			Set_MAX8998_PM_OUTPUT_Voltage(LDO5, VCC_1p800);  //CAM_SENSOR_1.8V//SecFeature.Camera aswoogi
			Set_MAX8998_PM_REG(ELDO5, 1);
		} else {
			Set_MAX8998_PM_REG(ELDO5, 0);//SecFeature.Camera aswoogi
			Set_MAX8998_PM_REG(ELDO15, 0);
			Set_MAX8998_PM_REG(ELDO14, 0);
			Set_MAX8998_PM_REG(ELDO13, 0);
			Set_MAX8998_PM_REG(ELDO12, 0);
			Set_MAX8998_PM_REG(ELDO11, 0);
			Set_MAX8998_PM_REG(EN4, 0);
		}
	}
	else{
	if(onoff){
		pmic_ldo_enable(LDO_CAM_CORE);
		pmic_ldo_enable(LDO_CAM_IO);
		pmic_ldo_enable(LDO_CAM_5M);
		pmic_ldo_enable(LDO_CAM_A);
		pmic_ldo_enable(LDO_CAM_CIF);
		pmic_ldo_enable(LDO_CAM_AF);
	} else {
		pmic_ldo_disable(LDO_CAM_IO);
		pmic_ldo_disable(LDO_CAM_5M);
		pmic_ldo_disable(LDO_CAM_A);
		pmic_ldo_disable(LDO_CAM_CIF);
		pmic_ldo_disable(LDO_CAM_AF);
		pmic_ldo_disable(LDO_CAM_CORE);
	}
}
}

static int ce147_cam_en(bool onoff)
{
	int err;
	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(S5PC11X_GPJ0(6), "GPJ0");
        if (err) {
                printk(KERN_ERR "failed to request GPJ0 for camera control\n");
                return err;
        }

	gpio_direction_output(S5PC11X_GPJ0(6), 0);
	msleep(1);
	gpio_direction_output(S5PC11X_GPJ0(6), 1);
	msleep(1);

	if(onoff){
	gpio_set_value(S5PC11X_GPJ0(6), 1);
	} else {
		gpio_set_value(S5PC11X_GPJ0(6), 0);
	}
	msleep(1);

	gpio_free(S5PC11X_GPJ0(6));

	return 0;
}

static int ce147_cam_nrst(bool onoff)
{
	int err;

	/* CAM_MEGA_nRST - GPJ1(5)*/
	err = gpio_request(S5PC11X_GPJ1(5), "GPJ1");
        if (err) {
                printk(KERN_ERR "failed to request GPJ1 for camera control\n");
                return err;
        }

	gpio_direction_output(S5PC11X_GPJ1(5), 0);
	msleep(1);
	gpio_direction_output(S5PC11X_GPJ1(5), 1);
	msleep(1);

	gpio_set_value(S5PC11X_GPJ1(5), 0);
	msleep(1);

	if(onoff){
	gpio_set_value(S5PC11X_GPJ1(5), 1);
		msleep(1);
	}
	gpio_free(S5PC11X_GPJ1(5));

	return 0;
}



static int ce147_power_on(void)
{
	int err;

	printk(KERN_ERR "ce147_power_on\n");

	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");

	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");

		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");

	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");

		return err;
	}

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}

	ce147_ldo_en(TRUE);

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PC11X_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);

	gpio_set_value(GPIO_CAM_VGA_nRST, 1);

	mdelay(1);

	// CAM_VGA_nSTBY  LOW
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	// CAM_MEGA_EN HIGH
	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);

	gpio_set_value(GPIO_CAM_MEGA_EN, 1);

	mdelay(1);

	// CAM_MEGA_nRST HIGH
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);

	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);

	gpio_free(GPIO_CAM_MEGA_EN);

	gpio_free(GPIO_CAM_MEGA_nRST);

	gpio_free(GPIO_CAM_VGA_nSTBY);

	gpio_free(GPIO_CAM_VGA_nRST);

	mdelay(5);

	return 0;
}



static int ce147_power_off(void)
{
	int err;

	printk(KERN_ERR "ce147_power_off\n");

	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");

	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");

		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");

	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}

	// CAM_VGA_nRST  LOW
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);

	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// CAM_MEGA_nRST - GPJ1(5) LOW
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);

	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);

	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	// CAM_MEGA_EN - GPJ0(6) LOW
	gpio_direction_output(GPIO_CAM_MEGA_EN, 1);

	gpio_set_value(GPIO_CAM_MEGA_EN, 0);

 	mdelay(1);

	ce147_ldo_en(FALSE);

	mdelay(1);

	gpio_free(GPIO_CAM_MEGA_EN);

	gpio_free(GPIO_CAM_MEGA_nRST);

	gpio_free(GPIO_CAM_VGA_nRST);

 	return 0;
}



static int ce147_power_en(int onoff)
{
#if 0
	if(onoff){
		ce147_ldo_en(true);
		s3c_gpio_cfgpin(S5PC11X_GPE1(3), S5PC11X_GPE1_3_CAM_A_CLKOUT);
		ce147_cam_en(true);
		ce147_cam_nrst(true);
	} else {
		ce147_cam_en(false);
		ce147_cam_nrst(false);
		s3c_gpio_cfgpin(S5PC11X_GPE1(3), 0);
		ce147_ldo_en(false);
	}

	return 0;
#endif

	if(onoff == 1) {
		ce147_power_on();
	}

	else {
		ce147_power_off();
	}

	return 0;
}

static int smdkc110_cam1_power(int onoff)
{
	int err;
	/* Implement on/off operations */

	/* CAM_VGA_nSTBY - GPB(0) */
	err = gpio_request(S5PC11X_GPB(0), "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	gpio_direction_output(S5PC11X_GPB(0), 0);

	mdelay(1);

	gpio_direction_output(S5PC11X_GPB(0), 1);

	mdelay(1);

	gpio_set_value(S5PC11X_GPB(0), 1);

	mdelay(1);

	gpio_free(S5PC11X_GPB(0));

	mdelay(1);

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(S5PC11X_GPB(2), "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	gpio_direction_output(S5PC11X_GPB(2), 0);

	mdelay(1);

	gpio_direction_output(S5PC11X_GPB(2), 1);

	mdelay(1);

	gpio_set_value(S5PC11X_GPB(2), 1);

	mdelay(1);

	gpio_free(S5PC11X_GPB(2));

	return 0;
}

/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/

/* External camera module setting */
static struct ce147_platform_data ce147_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  ce147_i2c_info = {
	I2C_BOARD_INFO("CE147", 0x78>>1),
	.platform_data = &ce147_plat,
};

static struct s3c_platform_camera ce147 = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
#if defined (CONFIG_JUPITER_VER_B4)
	.i2c_busnum	= 2,
#else
	.i2c_busnum	= 0,
#endif
	.info		= &ce147_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "srclk",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= ce147_power_en,
};
#endif

/* External camera module setting */
#ifdef CONFIG_VIDEO_S5KA3DFX
static void s5ka3dfx_ldo_en(bool onoff)
{
	if(onoff){
		pmic_ldo_enable(LDO_CAM_IO);
		pmic_ldo_enable(LDO_CAM_A);
		pmic_ldo_enable(LDO_CAM_CIF);
	} else {
		pmic_ldo_disable(LDO_CAM_IO);
		pmic_ldo_disable(LDO_CAM_A);
		pmic_ldo_disable(LDO_CAM_CIF);
	}
}

static int s5ka3dfx_cam_stdby(bool onoff)
{
	int err;
	/* CAM_VGA_nSTBY - GPB(0) */
	err = gpio_request(S5PC11X_GPB(0), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPB(0), 0);
	msleep(1);
	gpio_direction_output(S5PC11X_GPB(0), 1);
	msleep(1);

	if(onoff){
		gpio_set_value(S5PC11X_GPB(0), 1);
	} else {
		gpio_set_value(S5PC11X_GPB(0), 0);
	}
	msleep(1);

	gpio_free(S5PC11X_GPB(0));

	return 0;
}

static int s5ka3dfx_cam_nrst(bool onoff)
{
	int err;

	/* CAM_VGA_nRST - GPB(2)*/
	err = gpio_request(S5PC11X_GPB(2), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPB(2), 0);
	msleep(1);
	gpio_direction_output(S5PC11X_GPB(2), 1);
	msleep(1);

	gpio_set_value(S5PC11X_GPB(2), 0);
	msleep(1);

	if(onoff){
		gpio_set_value(S5PC11X_GPB(2), 1);
		msleep(1);
	}
	gpio_free(S5PC11X_GPB(2));

	return 0;
}



static int s5ka3dfx_power_on()
{
	int err;

	printk(KERN_ERR "s5ka3dfx_power_on\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

#if defined(CONFIG_ARIES_VER_B1) || defined(CONFIG_VICTORY_VER_B1)//SecFeature.Camera aswoogi
 	mdelay(1);

	// Turn CAM_SENSOR_A2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_ISP_HOST_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	// Turn CAM_ISP_RAM_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);

	mdelay(1);

//	gpio_free(GPIO_GPB7);
#elif defined CONFIG_JUPITER_VER_B5 || defined CONFIG_ARIES_VER_B0//SecFeature.Camera aswoogi
	// Turn CAM_ISP_1.2V on
	Set_MAX8998_PM_OUTPUT_Voltage(BUCK4, VCC_1p200);
	Set_MAX8998_PM_REG(EN4, 1);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_ISP_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	// Turn CAM_ISP_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);

	mdelay(1);

	// Turn CAM_ISP_1.2V off
	Set_MAX8998_PM_REG(EN4, 0);
#else	/* CONFIG_JUPITER_VER_B5 */
	// Turn CAM_A_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_D_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO11, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO11, 1);

	mdelay(1);

	// Turn CAM_IO_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);
#endif	/* CONFIG_JUPITER_VER_B5 */

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PC11X_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);

	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}



static int s5ka3dfx_power_off()
{
	int err;

	printk(KERN_ERR "s5ka3dfx_power_off\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}


	// CAM_VGA_nRST  LOW
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	// CAM_VGA_nSTBY  LOW
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

#if defined (CONFIG_ARIES_VER_B1) || !defined ( CONFIG_ARIES_VER_B1 )
	/* CAM_IO_EN - GPB(7) */
#if 0 //aswoogi 100322 not used GPIO in victory
	err = gpio_request(GPIO_GPB7, "GPB");

	if(err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}
#endif

	// Turn CAM_ISP_HOST_2.8V off
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_ISP_RAM_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);

#if 0 //aswoogi 100322 not used GPIO in victory
	// Turn CAM_ISP_SYS_2.8V off
	gpio_direction_output(GPIO_GPB7, 1);
	gpio_set_value(GPIO_GPB7, 0);

	gpio_free(GPIO_GPB7);
#endif
#elif defined CONFIG_JUPITER_VER_B5 || defined CONFIG_ARIES_VER_B0//SecFeature.Camera aswoogi
	// Turn CAM_ISP_2.8V off
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_ISP_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);
#else	/* CONFIG_JUPITER_VER_B5 */
	// Turn CAM_D_2.8V off
	Set_MAX8998_PM_REG(ELDO11, 0);

	mdelay(1);

	// Turn CAM_A_2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_IO_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);
#endif	/* CONFIG_JUPITER_VER_B5 */

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}



static int s5ka3dfx_power_en(int onoff)
{
#if 0
	if(onoff){
		s5ka3dfx_ldo_en(true);
		s3c_gpio_cfgpin(S5PC11X_GPE1(3), S5PC11X_GPE1_3_CAM_A_CLKOUT);
		s5ka3dfx_cam_stdby(true);
		s5ka3dfx_cam_nrst(true);
		mdelay(100);
	} else {
		s5ka3dfx_cam_stdby(false);
		s5ka3dfx_cam_nrst(false);
		s3c_gpio_cfgpin(S5PC11X_GPE1(3), 0);
		s5ka3dfx_ldo_en(false);
	}

	return 0;
#endif

	if(onoff){
		s5ka3dfx_power_on();
	} else {
		s5ka3dfx_power_off();
	}

	return 0;
}



static struct s5ka3dfx_platform_data s5ka3dfx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5ka3dfx_i2c_info = {
	I2C_BOARD_INFO("S5KA3DFX", 0xc4>>1),
	.platform_data = &s5ka3dfx_plat,
};

static struct s3c_platform_camera s5ka3dfx = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5ka3dfx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "srclk",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 480,
	//.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= s5ka3dfx_power_en,
};
#endif


/* Interface setting */
static struct s3c_platform_fimc __initdata fimc_plat = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc",
	.clk_rate	= 166000000,
	.default_cam	= CAMERA_PAR_A,
	.camera		= {
#ifdef CONFIG_VIDEO_CE147
		&ce147,
#endif
#ifdef CONFIG_VIDEO_S5KA3DFX
		&s5ka3dfx,
#endif
	},
	.hw_ver			= 0x43,
};


//Thomas Ryu 20100512, recovered from Revision #1.
static int arise_notifier_call(struct notifier_block *this, unsigned long code, void *_cmd)
{

	int mode = REBOOT_MODE_NONE;

		if ((code == SYS_RESTART) && _cmd) {
			if (!strcmp((char *)_cmd, "arm11_fota"))
				mode = REBOOT_MODE_ARM11_FOTA;
			else if (!strcmp((char *)_cmd, "arm9_fota"))
				mode = REBOOT_MODE_ARM9_FOTA;
			else if (!strcmp((char *)_cmd, "recovery")){

  			    mode = REBOOT_MODE_RECOVERY;
#ifdef CONFIG_KERNEL_DEBUG_SEC
			    //etinum.factory.reboot disable uart msg in bootloader for
			    // factory reset 2nd ack
			    kernel_sec_set_upload_cause(BLK_UART_MSG_FOR_FACTRST_2ND_ACK);
#endif
            }
			else if (!strcmp((char *)_cmd, "download"))
				mode = REBOOT_MODE_DOWNLOAD;

#ifdef CONFIG_KERNEL_DEBUG_SEC
		    //etinum.factory.reboot disable uart msg in bootloader for
		    // factory reset 2nd ack
		    else if (!strcmp((char *)_cmd, "factory_reboot")) {
			    mode = REBOOT_MODE_NONE;
			    kernel_sec_set_upload_cause(BLK_UART_MSG_FOR_FACTRST_2ND_ACK);
		    }
#endif
		}

		if(code != SYS_POWER_OFF)
		sec_set_param_value(__REBOOT_MODE, &mode);

	return NOTIFY_DONE;
}


static struct notifier_block arise_reboot_notifier = {
	.notifier_call = arise_notifier_call,
};


static void jupiter_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

};


#define S3C_GPIO_SETPIN_ZERO         0
#define S3C_GPIO_SETPIN_ONE          1
#define S3C_GPIO_SETPIN_NONE	     2  // dont set the data pin.


//#define S3C_GPIO_SLP_INPUT    3
/*
 *
 * GPIO Initialization table. It has the following format
 * { pin number, pin configuration, pin value, pullup/down config,
 * 		driver strength, slew rate, sleep mode pin conf, sleep mode pullup/down config }
 *
 * The table can be modified with the appropriate value for each pin.
 */
#if 1
static unsigned int jupiter_gpio_table[][8] = {
	/* Off part */
	// GPA0 ~ GPA1 : is done by UART driver early, so not modifying.
#if 0
	{S5PC11X_GPA0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	// uart2 rx and tx..  done by uboot. So not modifying
	//{S5PC11X_GPA1(0), 2, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
	//		 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//{S5PC11X_GPA1(1), 2, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
	//		 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPA1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPA1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif
	{S5PC11X_GPB(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPB(1), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPB(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#ifdef CONFIG_ARIES_VER_B1
        {S5PC11X_GPB(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
#else
        {S5PC11X_GPB(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif
        {S5PC11X_GPB(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPB(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPB(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPB(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPC0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_I2C_CON
        {S5PC11X_GPC0(3), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPC1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPD0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
 	    { S5PC11X_GPD0(1), GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE,
	          S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //jihyon82.kim
        {S5PC11X_GPD0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPD1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPE0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPE1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_2X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#if 0
	{S5PC11X_GPF0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPF1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF1(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPF2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPF3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif
        {S5PC11X_GPF3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPF3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPG0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	//cky 20100507 WIMAX SDIO
	{S5PC11X_GPG1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG1(2), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPG2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG2(2), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPG3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPG3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	/* Alive part */
        {S5PC11X_GPH0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},

        {S5PC11X_GPH1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
	//cky 20100503 WIMAX_RESET_N
        {S5PC11X_GPH1(1), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
	//cky 20100503 WIMAX_EN
        {S5PC11X_GPH1(4), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},

#ifdef CONFIG_ARIES_VER_B1
        {S5PC11X_GPH1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
        	 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
        {S5PC11X_GPH1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
#endif
	//cky 20100503 WIMAX_INT
        {S5PC11X_GPH1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH1(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},

        {S5PC11X_GPH2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},

        {S5PC11X_GPH3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},
        {S5PC11X_GPH3(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0},


	/* Alive part ending and off part start*/
	{S5PC11X_GPI(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPJ0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPJ1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPJ2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ2(4), GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE,
	          S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //jihyon82.kim
        {S5PC11X_GPJ2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{S5PC11X_GPJ2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN, //hojun_kim //hojun_kim S3C_GPIO_PULL_DOWN->S3C_GPIO_PULL_UP
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPJ3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ3(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPJ4(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ4(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ4(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ4(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ4(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/* memory part */
	{S5PC11X_MP01(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP02(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP02(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP02(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP02(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP03(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP04(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP04(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP04(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP04(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_CON2
        {S5PC11X_MP04(4), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_WAKEUP
        {S5PC11X_MP04(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_CON0
        {S5PC11X_MP04(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_CON1
        {S5PC11X_MP04(7), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP05(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_IF_MODE0
        {S5PC11X_MP05(1), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	//cky 20100503 WIMAX_IF_MODE1
        {S5PC11X_MP05(2), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO, S3C_GPIO_PULL_NONE,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP05(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP05(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

        //victory.boot GPIO_MLCD_RST
        {S5PC11X_MP05(5), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

        {S5PC11X_MP05(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP05(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP06(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP07(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	/* Memory part ending and off part ending */

};


void s3c_config_gpio_table(int array_size, unsigned int (*gpio_table)[8])
{
        u32 i, gpio;
        for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		/* Off part */
		if((gpio <= S5PC11X_GPG3(6)) ||
		   ((gpio <= S5PC11X_GPJ4(7)) && (gpio >= S5PC11X_GPI(0)))) {

        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

                	//s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
                	//s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);

		}
#if 0
		/* Alive part */
		else if((gpio <= S5PC11X_GPH3(7)) && (gpio >= S5PC11X_GPH0(0))) {
        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);
		}
#endif
#if 0
		/* Memory part */
		else if((gpio >  S5PC11X_GPJ4(4)) && (gpio <= S5PC11X_MP07(7))) {
        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

                	//s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
                	//s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);

		}
#endif
	}

    //Thomas Ryu, 20100401 to set 4mA of this GPIO
    s3c_gpio_set_drvstrength(S5PC11X_GPH3(7), S3C_GPIO_DRVSTR_2X);


}
#else
static int jupiter_gpio_table[][8] = {
	/** OFF PART **/
	/* GPA */
	{ GPIO_BT_UART_RXD, GPIO_BT_UART_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BT_UART_TXD, GPIO_BT_UART_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BT_UART_CTS, GPIO_BT_UART_CTS_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BT_UART_RTS, GPIO_BT_UART_RTS_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_UART_RXD, GPIO_GPS_UART_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_UART_TXD, GPIO_GPS_UART_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_UART_CTS, GPIO_GPS_UART_CTS_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_UART_RTS, GPIO_GPS_UART_RTS_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_RXD, GPIO_AP_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_TXD, GPIO_AP_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_FLM_RXD, GPIO_AP_FLM_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_FLM_TXD, GPIO_AP_FLM_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/* GPB */
	{ GPIO_CAM_VGA_nSTBY, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MSENSE_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_VGA_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BT_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BOOT_MODE, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_BT_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPC*/
	{ GPIO_REC_PCM_CLK, GPIO_REC_PCM_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_REC_PCM_SYNC, GPIO_REC_PCM_SYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_REC_PCM_IN, GPIO_REC_PCM_IN_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_REC_PCM_OUT, GPIO_REC_PCM_OUT_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPD*/
	{ GPIO_VIBTONE_PWM1, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_SDA_29V, GPIO_CAM_SDA_29V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_SCL_29V, GPIO_CAM_SCL_29V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_HDMI_SDA, GPIO_AP_HDMI_SDA_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_HDMI_SCL, GPIO_AP_HDMI_SCL_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_SDA_29V, GPIO_AP_SDA_29V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_SCL_29V, GPIO_AP_SCL_29V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPE*/
	{ GPIO_CAM_PCLK, GPIO_CAM_PCLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_VSYNC, GPIO_CAM_VSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_HSYNC, GPIO_CAM_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D0, GPIO_CAM_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D1, GPIO_CAM_D1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D2, GPIO_CAM_D2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D3, GPIO_CAM_D3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D4, GPIO_CAM_D4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D5, GPIO_CAM_D5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D6, GPIO_CAM_D6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_D7, GPIO_CAM_D7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_MCLK, GPIO_CAM_MCLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GYRO_HP, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPF*/
	{ GPIO_DISPLAY_HSYNC, GPIO_DISPLAY_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DISPLAY_VSYNC, GPIO_DISPLAY_VSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DISPLAY_DE, GPIO_DISPLAY_DE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DISPLAY_PCLK, GPIO_DISPLAY_PCLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D0, GPIO_LCD_D0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D1, GPIO_LCD_D1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D2, GPIO_LCD_D2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D3, GPIO_LCD_D3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D4, GPIO_LCD_D4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D5, GPIO_LCD_D5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D6, GPIO_LCD_D6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D7, GPIO_LCD_D7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D8, GPIO_LCD_D8_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D9, GPIO_LCD_D9_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D10, GPIO_LCD_D10_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE,S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D11, GPIO_LCD_D11_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D12, GPIO_LCD_D12_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D13, GPIO_LCD_D13_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D14, GPIO_LCD_D14_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D15, GPIO_LCD_D15_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D16, GPIO_LCD_D16_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D17, GPIO_LCD_D17_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D18, GPIO_LCD_D18_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D19, GPIO_LCD_D19_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D20, GPIO_LCD_D20_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D21, GPIO_LCD_D21_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D22, GPIO_LCD_D22_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_D23, GPIO_LCD_D23_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPG*/
	{ GPIO_NAND_CLK, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_NAND_CMD, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_NAND_D0, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_NAND_D1, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_NAND_D2, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_NAND_D3, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{ GPIO_T_FLASH_CLK, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_T_FLASH_CMD, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_T_FLASH_D0, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_T_FLASH_D1, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_T_FLASH_D2, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_T_FLASH_D3, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{ GPIO_WLAN_SDIO_CLK, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_SDIO_CMD, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_nRST, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_SDIO_D0, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_SDIO_D1, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_SDIO_D2, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_SDIO_D3, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{ GPIO_GPS_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_PWR_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_CLK_INT, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_TA_CURRENT_SEL_AP, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BT_WAKE, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_NWLAN_WAKE, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_TOUCH_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPH*/
	//{ GPIO_AP_PS_HOLD, GPIO_OUTPUT, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_ACC_INT, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BUCK_2_EN_A, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BUCK_2_EN_B, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_BUCK_3_EN, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GYRO_PD, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DET_35, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_PMIC_IRQ, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_PS_VOUT, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_nINT_ONEDRAM_AP, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HDMI_CEC, GPIO_HDMI_CEC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HDMI_HPD, GPIO_HDMI_HPD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_PHONE_ACTIVE, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBC0, GPIO_KBC0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBC1, GPIO_KBC1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBC2, GPIO_KBC2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBC3, GPIO_KBC3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_WLAN_HOST_WAKE, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ BT_HOST_WAKE, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_nPOWER, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_JACK_nINT, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBR0, GPIO_KBR0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBR1, GPIO_KBR1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBR2, GPIO_KBR2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KBR3, GPIO_KBR3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_T_FLASH_DETECT, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MSENSE_IRQ, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_EAR_SEND_END, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CP_RST, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPI*/
	{ GPIO_CODEC_I2S_CLK, GPIO_CODEC_I2S_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CODEC_I2S_WS, GPIO_CODEC_I2S_WS_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CODEC_I3S_DI, GPIO_CODEC_I3S_DI_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CODEC_I3S_DO, GPIO_CODEC_I3S_DO_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*GPJ*/
	{ GPIO_PDA_ACTIVE, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HWREV_MODE0, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HWREV_MODE1, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HWREV_MODE2, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_TOUCH_INT, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_MEGA_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_USB_SEL30, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_PHONE_ON, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HAPTIC_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MLCD_ON, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_EAR_SEL, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_MEGA_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_FLASH_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CAM_FLASH_SET, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_HDMI_EN1, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CODEC_XTAL_EN, GPIO_OUTPUT, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_FM_INT, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_FM_BUS_nRST, GPIO_OUTPUT, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_UP, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_EAR35_SW, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_IrDA_SHUTDOWN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_ALS_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_I2C_SDA_28V, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_I2C_SCL_28V, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_PMIC_SDA, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_KEY_LED_ON, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MICBIAS_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_PMIC_SCL, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_TV_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	/*MP*/
	{ GPIO_OLED_DET, GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DISPLAY_CS, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_SUB_DISPLAY_CS, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_OLED_ID, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_NANDCS, GPIO_AP_NANDCS_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DIC_ID, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_VCC_19V_PDA, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_VCC_18V_PDA, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CP_nRST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_CODEC_LDO_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_USB_SEL, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DISPLAY_CLK, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_DISPLAY_SI, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LVDS_RST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_GPS_CLK_EN, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MHL_RST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MHL_USB_SEL, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_LCD_BACKLIGHT_EN, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_SCL_18V, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_AP_SDA_18V, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_MLCD_RST, GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ GPIO_UART_SEL, GPIO_OUTPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
};

void s3c_config_gpio_table(int array_size, int (*gpio_table)[6])
{
	u32 i, gpio;

	pr_debug("%s: ++\n", __func__);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
	#if 0
		if (gpio < S3C64XX_GPIO_ALIVE_PART_BASE) { /* Off Part */
			pr_debug("%s: Off gpio=%d,%d\n", __func__, gpio,
					S3C64XX_GPIO_ALIVE_PART_BASE);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_slp_cfgpin(gpio, gpio_table[i][4]);
			s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][5]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		} else if (gpio < S3C64XX_GPIO_MEM_PART_BASE) { /* Alive Part */
			pr_debug("%s: Alive gpio=%d\n", __func__, gpio);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		} else { /* Memory Part */
			pr_debug("%s: Memory gpio=%d\n", __func__, gpio);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_slp_cfgpin(gpio, gpio_table[i][4]);
			s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][5]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		}
	#else
			printk("gpio num: %d\n",i);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
			s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

	//      	s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
	//    	s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);
	#endif
	}
	pr_debug("%s: --\n", __func__);
}
#endif


#define S5PC11X_PS_HOLD_CONTROL_REG (S3C_VA_SYS+0xE81C)

#if 1 //20100429_inchul.im   To fix the leakage current issue in power off state
extern int max8893_ldo_disable_direct(int);
extern int max8893_ldo_is_enabled_direct(int);
#endif

static void smdkc110_power_off (void)
{
		int mode = REBOOT_MODE_NONE;
		char reset_mode = 'r';
		int phone_wait_cnt = 0;
#if 1 //20100429_inchul.im   To fix the leakage current issue in power off state
   int i;
#endif


		kernel_sec_clear_upload_magic_number();

		gpio_direction_input(GPIO_N_POWER);
		gpio_direction_input(GPIO_PHONE_ACTIVE);

		gpio_set_value(GPIO_PHONE_ON, 0); //prevent phone reset when AP off

#if 1 //20100429_inchul.im   To fix the leakage current issue in power off state
   	for (i=1; i<=6; i++) { //from MAX8893_LDO1(1) to MAX8893_BUCK(6)
     if(max8893_ldo_is_enabled_direct(i)){
        max8893_ldo_disable_direct(i);
     }
   	}
#endif

		// confirm phone off
		while(1)
		{
			if (gpio_get_value(GPIO_PHONE_ACTIVE))
			{
				//if (phone_wait_cnt > 3)
				{
					printk(KERN_EMERG "%s: Try to Turn Phone Off by CP_RST\n", __func__);
					gpio_set_value(GPIO_CP_RST, 0);
				}
				//if (phone_wait_cnt > 5)
				if (phone_wait_cnt > 1)					
				{
					printk(KERN_EMERG "%s: PHONE OFF Failed\n", __func__);
					break;
				}
				phone_wait_cnt++;
				mdelay(1000);
			}
			else
			{
				printk(KERN_EMERG "%s: PHONE OFF Success\n", __func__);
				break;
			}
		}

#if 0 // check JIG connection
		// never watchdog reset at JIG. It cause the unwanted reset at final IMEI progress
		// infinite delay is better than reset because jig is not the user case.
		if (get_usb_cable_state() & (JIG_UART_ON | JIG_UART_OFF | JIG_USB_OFF | JIG_USB_ON))
		{
			/* Watchdog Reset */
			printk(KERN_EMERG "%s: JIG is connected, rebooting...\n", __func__);
			arch_reset(reset_mode);
			printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
			while(1);
		}
#endif

		while(1)
		{
			// Reboot Charging
#if 1 //suik_Check -> Hana gonna do it!
			if (maxim_chg_status())
			{
				mode = REBOOT_MODE_CHARGING;
				if (sec_set_param_value)
					sec_set_param_value(__REBOOT_MODE, &mode);
				/* Watchdog Reset */
				printk(KERN_EMERG "%s: TA or USB connected, rebooting...\n", __func__);
				kernel_sec_hw_reset(TRUE);
				printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
				while(1);
			}
#endif
			// wait for power button release
			if (gpio_get_value(GPIO_N_POWER))
			{
				printk(KERN_EMERG "%s: set PS_HOLD low.\n", __func__);

				/*PS_HOLD high	PS_HOLD_CONTROL, R/W, 0xE010_E81C*/
				writel(readl(S5PC11X_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF, S5PC11X_PS_HOLD_CONTROL_REG);

				printk(KERN_EMERG "%s: should not reach here!\n", __func__);
			}

			// if power button is not released, wait for a moment. then check TA again.
			printk(KERN_EMERG "%s: PowerButton is not released.\n", __func__);
			mdelay(1000);
		}
	}



/* this table only for B4 board */

static unsigned int jupiter_sleep_gpio_table[][3] = {

	{S5PC11X_GPA0(0),
			 S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
	{S5PC11X_GPA0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100503_inchul.im... input pull none in sleep mode(for victory)
	{S5PC11X_GPA0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
	{S5PC11X_GPA0(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{S5PC11X_GPA0(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
	{S5PC11X_GPA0(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
	{S5PC11X_GPA0(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
	{S5PC11X_GPA0(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)

	{S5PC11X_GPA1(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100611_inchul... input pull down(from HW)
	{S5PC11X_GPA1(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100611_inchul... output low pull none(from HW)
	{S5PC11X_GPA1(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
	{S5PC11X_GPA1(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)

	{S5PC11X_GPB(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPB(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPB(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPB(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100430_inchul... BUCK_1_EN_B : input pull down in sleep mode(for victory)
        {S5PC11X_GPB(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100722_inchul... input pull down(from HW)
        {S5PC11X_GPB(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPB(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#if defined CONFIG_ARIES_VER_B0 ||!defined ( CONFIG_ARIES_VER_B1 )	 || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPB(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#else
		{S5PC11X_GPB(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif

	{S5PC11X_GPC0(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPC0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPC0(2),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},  //20100506_inchul... GPIO_WIMAX_PM_SDA : Input & Pull-None in Sleep Mode(for victory)
	//cky 20100503 WIMAX_I2C_CON
        {S5PC11X_GPC0(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //hojun_kim from HW req
        {S5PC11X_GPC0(4),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100506_inchul... GPIO_WIMAX_PM_SCL : Input & Pull-None in Sleep Mode(for victory)
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 )	 || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_GPC1(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#else
	{S5PC11X_GPC1(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif
        {S5PC11X_GPC1(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 )|| defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPC1(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPC1(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPC1(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#else
		{S5PC11X_GPC1(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
		{S5PC11X_GPC1(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
		{S5PC11X_GPC1(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif

	{S5PC11X_GPD0(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 )  || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPD0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100503_inchul.im... Input Pull-None in Sleep Mode(for victory)
        {S5PC11X_GPD0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
		{S5PC11X_GPD0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPD0(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_GPD0(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPD1(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},   //20100426_inchul... GPIO_CAM_SDA_29V : Input & Pull-Down in Sleep Mode(for victory)
        {S5PC11X_GPD1(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},   //20100426_inchul... GPIO_CAM_SCL_29V : Input & Pull-Down in Sleep Mode(for victory)
        {S5PC11X_GPD1(2),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},   //20100506_inchul... AP_SDA_30V : output high & Pull-None in Sleep Mode(for victory)
        {S5PC11X_GPD1(3),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},   //20100506_inchul... AP_SCL_30V : output high & Pull-None in Sleep Mode(for victory)
        {S5PC11X_GPD1(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},  //20100506_inchul... GPIO_TSP_SDA_28V : Input & Pull-Down in Sleep Mode(for victory)
        {S5PC11X_GPD1(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},  //20100506_inchul... GPIO_TSP_SCL_28V : Input & Pull-Down in Sleep Mode(for victory)

	{S5PC11X_GPE0(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE0(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_GPE1(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPE1(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 ||!defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPE1(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
        {S5PC11X_GPE1(4),
		  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
	{S5PC11X_GPF0(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF0(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},

	{S5PC11X_GPF1(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF1(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},

	{S5PC11X_GPF2(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF2(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},

	{S5PC11X_GPF3(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF3(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF3(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPF3(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{S5PC11X_GPF3(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100426_inchul... CODEC_LDO_EN is Low in Sleep Mode(for victory)
        {S5PC11X_GPF3(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},


	{S5PC11X_GPG0(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPG0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#else
        {S5PC11X_GPG0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif
        {S5PC11X_GPG0(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG0(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG0(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG0(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)

	{S5PC11X_GPG1(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100611_inchul.im... input/ pull-down in sleep mode(from HW)
        {S5PC11X_GPG1(1),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100505_inchul.im... output high/ pull-none  in sleep mode(for victory)
        {S5PC11X_GPG1(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100505_inchul.im... output low/ pull-none  in sleep mode(for victory)
        {S5PC11X_GPG1(3),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100505_inchul.im... output high/ pull-none in sleep mode(for victory)
        {S5PC11X_GPG1(4),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100505_inchul.im... output high/ pull-none  in sleep mode(for victory)
        {S5PC11X_GPG1(5),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100505_inchul.im... output high/ pull-none  in sleep mode(for victory)
        {S5PC11X_GPG1(6),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100505_inchul.im... output high/ pull-none  in sleep mode(for victory)

	{S5PC11X_GPG2(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG2(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPG2(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
#else
        {S5PC11X_GPG2(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_GPG2(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG2(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG2(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPG2(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)

	{S5PC11X_GPG3(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100506_inchul... _3_TOUCH_SDA_28V : Input & Pull-Down in Sleep Mode(for victory)
        {S5PC11X_GPG3(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPG3(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100506_inchul... _3_TOUCH_SCL_28V : Input & Pull-Down in Sleep Mode(for victory)
#else
        {S5PC11X_GPG3(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_GPG3(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100426_inchul... GPIO_MICBIAS_EN : Output Low & Pull-None in Sleep Mode(for victory)
        {S5PC11X_GPG3(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPG3(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPG3(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#else
		{S5PC11X_GPG3(6),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},
#endif


	/* Alive part ending and off part start*/
#if 1
	{S5PC11X_GPI(0),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPI(1),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPI(2),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPI(3),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPI(4),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPI(5),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPI(6),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
#else


	{S5PC11X_GPI(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPI(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},


#endif
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_GPJ0(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP}, //20100722_inchul... input pull up(from HW)
        {S5PC11X_GPJ0(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
#else
	{S5PC11X_GPJ0(0),
				  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
			{S5PC11X_GPJ0(1),
				  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif
        {S5PC11X_GPJ0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ0(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100506_inchul.im... output low/pull-none in sleep mode(for victiory)
        {S5PC11X_GPJ0(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100506_inchul.im... output low/pull-none in sleep mode(for victiory)
        {S5PC11X_GPJ0(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_GPJ0(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPJ0(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
#else
		{S5PC11X_GPJ0(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif

	{S5PC11X_GPJ1(0),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ1(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ1(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#if !defined ( CONFIG_ARIES_VER_B1 ) || CONFIG_ARIES_VER_B4
        {S5PC11X_GPJ1(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#else
        {S5PC11X_GPJ1(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, //20100426_inchul... TOUCH_EN is Low in Sleep Mode(for victory)
#endif
        {S5PC11X_GPJ1(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100611_inchul... input pull down(from HW)
        {S5PC11X_GPJ1(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},

	{S5PC11X_GPJ2(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ2(1),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100507_inchul... output high/pull-none in sleep mode(for victory)
        {S5PC11X_GPJ2(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPJ2(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},  //20100517_inchul... input pull down(from HW)
        {S5PC11X_GPJ2(4),
              S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},  //jihyon82.kim for proximity sensor
        {S5PC11X_GPJ2(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
		{S5PC11X_GPJ2(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //hojun_kim S3C_GPIO_PULL_NONE ->S3C_GPIO_PULL_DOWN
        {S5PC11X_GPJ2(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100430_inchul... CAM_FLASH_SET : Input & Pull-Down in Sleep Mode(for victory)
#else
		{S5PC11X_GPJ2(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
		{S5PC11X_GPJ2(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#endif


#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_GPJ3(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100611_inchul... input pull down(from HW)
        {S5PC11X_GPJ3(1),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ3(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ3(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100426_inchul... _3_GPIO_TOUCH_INT : Input & Pull-None in Sleep Mode(for victory)
		{S5PC11X_GPJ3(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... GPIO_FM_SCL_28V : input & Pull-None in Sleep Mode(for victory)
        {S5PC11X_GPJ3(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... GPIO_FM_SDA_28V : input & Pull-None in Sleep Mode(for victory)
#else
	{S5PC11X_GPJ3(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
		{S5PC11X_GPJ3(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_GPJ3(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
		{S5PC11X_GPJ3(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
		{S5PC11X_GPJ3(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ3(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif

#ifdef CONFIG_ARIES_VER_B1
        {S5PC11X_GPJ3(6),
        	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... GPIO_AP_PMIC_SCL : input & Pull-None in Sleep Mode(for victory)
        {S5PC11X_GPJ3(7),
        	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... GPIO_AP_PMIC_SDA : input & Pull-None in Sleep Mode(for victory)


	    {S5PC11X_GPJ4(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input/pull none in sleep mode(for victory)

        {S5PC11X_GPJ4(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input/pull none in sleep mode(for victory)

	    {S5PC11X_GPJ4(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input/pull none in sleep mode(for victory)
        {S5PC11X_GPJ4(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input/pull none in sleep mode(for victory)
        {S5PC11X_GPJ4(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)


#else
        {S5PC11X_GPJ3(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ3(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},

	{S5PC11X_GPJ4(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 )|| defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_GPJ4(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
#else
        {S5PC11X_GPJ4(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
	{S5PC11X_GPJ4(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input/pull none in sleep mode(for victory)
        {S5PC11X_GPJ4(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
        {S5PC11X_GPJ4(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},

#endif

	/* memory part */

	{S5PC11X_MP01(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_MP01(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
        {S5PC11X_MP01(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_MP01(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100510_inchul... pull none in sleep mode(for victory) //20100718_hyunjooakim down
        {S5PC11X_MP01(4),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP01(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_MP01(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP01(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
		{S5PC11X_MP01(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP01(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif


	{S5PC11X_MP02(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP02(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP02(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP02(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_MP03(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP03(2),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, //20100429_inchul.im... OneNand RP(Reset) : Output Low/Pull-None in Sleep Mode(for victory)
#else
	{S5PC11X_MP03(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP03(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP03(2),
        	  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_MP03(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_DOWN}, //20100504_inchul... out low in sleep mode(for victory)
        {S5PC11X_MP03(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP03(5),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
        {S5PC11X_MP03(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
        {S5PC11X_MP03(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_MP03(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_MP04(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input & Pull-None in Sleep Mode(for victory)
#else
	{S5PC11X_MP04(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
        {S5PC11X_MP04(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP04(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //20100510_inchul... input & Pull-None in Sleep Mode(for victory)
        {S5PC11X_MP04(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	//cky 20100503 WIMAX_CON2
        {S5PC11X_MP04(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	//cky 20100503 WIMAX_WAKEUP
        {S5PC11X_MP04(5),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
#else
		{S5PC11X_MP04(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
	//cky 20100503 WIMAX_CON0
        {S5PC11X_MP04(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE},
#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	//cky 20100503 WIMAX_CON1
        {S5PC11X_MP04(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#else
		{S5PC11X_MP04(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif

#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_MP05(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)
	//cky 20100503 WIMAX_IF_MODE0
        {S5PC11X_MP05(1),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
#else
	{S5PC11X_MP05(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP05(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif
	//cky 20100503 WIMAX_IF_MODE1
        {S5PC11X_MP05(2),
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP05(3),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},  //20100506_inchul... AP_I2C_SCL : output high & Pull-None in Sleep Mode(for victory)
        {S5PC11X_MP05(4),
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},  //20100506_inchul... AP_I2C_SDA : output high & Pull-None in Sleep Mode(for victory)
        {S5PC11X_MP05(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PC11X_MP05(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP05(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //20100517_inchul... input pull down(from HW)

#if defined CONFIG_ARIES_VER_B0 || !defined ( CONFIG_ARIES_VER_B1 ) || defined (CONFIG_ARIES_VER_B1)
	{S5PC11X_MP06(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP06(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{S5PC11X_MP07(0),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(1),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(4),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(5),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(6),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PC11X_MP07(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
#else
	{S5PC11X_MP06(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP06(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},

	{S5PC11X_MP07(0),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(4),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(6),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
		{S5PC11X_MP07(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
#endif

	/* Memory part ending and off part ending */

};

void s3c_config_sleep_gpio_table(int array_size, unsigned int (*gpio_table)[3])
{
        u32 i, gpio;

        for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
               	s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
               	s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
	}
}
// just for ref..
//
void s3c_config_sleep_gpio(void)
{

	// setting the alive mode registers

#ifdef CONFIG_ARIES_VER_B1
	s3c_gpio_cfgpin(S5PC11X_GPB(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPB(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPB(3), 0);
#endif

    if(system_rev < 0x0A) //proximity sensor disable in rev 10
    {
        s3c_gpio_cfgpin(S5PC11X_GPH0(1), S3C_GPIO_INPUT);
	    s3c_gpio_setpull(S5PC11X_GPH0(1), S3C_GPIO_PULL_DOWN);
      	//s3c_gpio_setpin(S5PC11X_GPH0(1), 0);
    }

#if 0 //20100504_inchul...
	s3c_gpio_cfgpin(S5PC11X_GPH0(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH0(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH0(2), 0);
#else
	//s3c_gpio_cfgpin(S5PC11X_GPH0(2), S3C_GPIO_INPUT);  // seonha TF -> 3P_Send_End
	//s3c_gpio_setpull(S5PC11X_GPH0(2), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH0(2), 0);
#endif

#if 0//SecFeature.Victory - WIFI
#ifdef CONFIG_ARIES_VER_B1
	s3c_gpio_cfgpin(S5PC11X_GPH0(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH0(3), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH2(4), 0);
#else
	s3c_gpio_cfgpin(S5PC11X_GPH0(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH0(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH0(3), 0);
#endif
#endif

#if 0 //20100506_inchul...
	s3c_gpio_cfgpin(S5PC11X_GPH0(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH0(4), S3C_GPIO_PULL_NONE);
	//c_gpio_setpin(S5PC11X_GPH0(4), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH0(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH0(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH0(4), 0);
#endif

	if(system_rev >= 0x08){ //20100526_inchul
	  //add the hall_sw config to here in sleep mode
    printk("[s3c_config_sleep_gpio] system_rev = %x \n", system_rev);  //temp.. will be deleted
	}
 else{
    printk("[s3c_config_sleep_gpio] system_rev = %x \n", system_rev);  //temp.. will be deleted
	   s3c_gpio_cfgpin(S5PC11X_GPH0(5), S3C_GPIO_OUTPUT);
	   s3c_gpio_setpull(S5PC11X_GPH0(5), S3C_GPIO_PULL_NONE);
	   s3c_gpio_setpin(S5PC11X_GPH0(5), 0);
 }

	//s3c_gpio_cfgpin(S5PC11X_GPH0(6), S3C_GPIO_INPUT);   // seonha
	//s3c_gpio_setpull(S5PC11X_GPH0(6), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH0(6), 0);

	//s3c_gpio_cfgpin(S5PC11X_GPH0(7), S3C_GPIO_INPUT);
	//s3c_gpio_setpull(S5PC11X_GPH0(7), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH0(0), 0);

	//s3c_gpio_cfgpin(S5PC11X_GPH1(0), S3C_GPIO_INPUT);  //seonha TF
	//s3c_gpio_setpull(S5PC11X_GPH1(0), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH1(1), 0);

#if 1	// cky 20100414
	// WIMAX_RESET_N: keep the current level

	//s3c_gpio_cfgpin(S5PC11X_GPH1(1), S3C_GPIO_INPUT);
	//s3c_gpio_setpull(S5PC11X_GPH1(1), S3C_GPIO_PULL_DOWN);
	//s3c_gpio_setpin(S5PC11X_GPH1(1), 0);
#endif

	//
	s3c_gpio_cfgpin(S5PC11X_GPH1(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH1(2), S3C_GPIO_PULL_NONE); //20100504_inchul...
	//s3c_gpio_setpin(S5PC11X_GPH1(2), 0);

#if 0	// kt.hur on 100104
	s3c_gpio_cfgpin(S5PC11X_GPH1(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH1(3), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH1(3), 0);
#endif

#ifdef CONFIG_ARIES_VER_B1	// cky 20100414
	// WIMAX_EN: keep the current level
#else
	s3c_gpio_cfgpin(S5PC11X_GPH1(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH1(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH1(4), 0);
#endif

#ifdef CONFIG_ARIES_VER_B1
//   s3c_gpio_cfgpin(S5PC11X_GPH1(5), S3C_GPIO_OUTPUT);
//   s3c_gpio_setpull(S5PC11X_GPH1(5), S3C_GPIO_PULL_NONE);
#else
	s3c_gpio_cfgpin(S5PC11X_GPH1(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH1(5), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH1(5),0);
#endif

#ifdef CONFIG_ARIES_VER_B1	// cky 20100414
	// WIMAX_INT

	//s3c_gpio_cfgpin(S5PC11X_GPH1(6), S3C_GPIO_INPUT);
	//s3c_gpio_setpull(S5PC11X_GPH1(6), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH1(6), 0);
#else
	// WIMAX_EN: keep the current level
#endif

	s3c_gpio_cfgpin(S5PC11X_GPH1(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH1(7), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH1(7), 0);

#if 0
	s3c_gpio_cfgpin(S5PC11X_GPH2(0), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(0), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(1), 0);
#if 0 //open_send_end pin
	s3c_gpio_cfgpin(S5PC11X_GPH2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(2), 0);
#endif
	s3c_gpio_cfgpin(S5PC11X_GPH2(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(3), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(4), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH2(4), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(5), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH2(5), 0);
#else //20100510_inchul...(for victory)
	s3c_gpio_cfgpin(S5PC11X_GPH2(0), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(0), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(1), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(2), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(3), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(4), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH2(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH2(5), 0);
#endif

	//s3c_gpio_cfgpin(S5PC11X_GPH2(6), S3C_GPIO_INPUT);
	//s3c_gpio_setpull(S5PC11X_GPH2(6), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH2(6), 0);

#if 0 //microusb.. 20100517_inchul
	s3c_gpio_cfgpin(S5PC11X_GPH2(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH2(7), S3C_GPIO_PULL_UP); //20100510_inchul... pull up in sleep mode(for victory)
	//s3c_gpio_setpin(S5PC11X_GPH2(7), 0);
#endif

#if 0
#if 0 // keypad
	s3c_gpio_cfgpin(S5PC11X_GPH3(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(0), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PC11X_GPH3(0), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH3(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(1), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PC11X_GPH3(1), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH3(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(2), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PC11X_GPH3(2), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(3), S3C_GPIO_PULL_UP);
//	s3c_gpio_setpin(S5PC11X_GPH3(3), 0);

#endif
	s3c_gpio_cfgpin(S5PC11X_GPH3(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(4), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(4), 0);

	s3c_gpio_cfgpin(S5PC11X_GPH3(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(5), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(5), 0);
#else
#if 0
	s3c_gpio_cfgpin(S5PC11X_GPH3(0), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(0), S3C_GPIO_PULL_NONE);
 s3c_gpio_setpin(S5PC11X_GPH3(0), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH3(1), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH3(2), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH3(3), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH3(4), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH3(5), 1);
#else //20100510_inchul...(for victory)
//	s3c_gpio_cfgpin(S5PC11X_GPH3(0), S3C_GPIO_INPUT);
//	s3c_gpio_setpull(S5PC11X_GPH3(0), S3C_GPIO_PULL_NONE);
 //s3c_gpio_setpin(S5PC11X_GPH3(0), 1);

//	s3c_gpio_cfgpin(S5PC11X_GPH3(1), S3C_GPIO_INPUT);
//	s3c_gpio_setpull(S5PC11X_GPH3(1), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(1), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(2), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(2), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(3), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(3), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(4), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(4), 1);

	s3c_gpio_cfgpin(S5PC11X_GPH3(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(5), S3C_GPIO_PULL_NONE);
	//s3c_gpio_setpin(S5PC11X_GPH3(5), 1);
#endif
#endif

#if 0 //short_sendend pin
	s3c_gpio_cfgpin(S5PC11X_GPH3(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpin(S5PC11X_GPH3(6), 0);
#endif
	s3c_gpio_cfgpin(S5PC11X_GPH3(7), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PC11X_GPH3(7), S3C_GPIO_PULL_NONE); //20100510_inchul... pull none in sleep mode (for victory)
	s3c_gpio_setpin(S5PC11X_GPH3(7), 1);


    //s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table),
    //                jupiter_sleep_gpio_table);
}

EXPORT_SYMBOL(s3c_config_sleep_gpio);


unsigned int HWREV=0;
EXPORT_SYMBOL(HWREV);
extern void set_pmic_gpio(void);
void jupiter_init_gpio(void)
{
        s3c_config_gpio_table(ARRAY_SIZE(jupiter_gpio_table),
                        jupiter_gpio_table);

        s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table),
                        jupiter_sleep_gpio_table);

	if(system_rev >= 0x08){ //20100526_inchul
       printk("[jupiter_init_gpio] system_rev = %x \n", system_rev);  //temp.. will be deleted
        s3c_gpio_slp_cfgpin(S5PC11X_GPB(7), S3C_GPIO_SLP_OUT0); //tflash_clk
        s3c_gpio_slp_setpull_updown(S5PC11X_GPB(7), S3C_GPIO_PULL_NONE);
	}

    if(system_rev >= 0x0A) //jihyon82.kim for ACC_INT for rev10
    {
      s3c_gpio_slp_cfgpin(S5PC11X_GPJ0(1), S3C_GPIO_SLP_INPUT);
      s3c_gpio_slp_setpull_updown(S5PC11X_GPJ0(1), S3C_GPIO_PULL_DOWN);
    }

	/*Adding pmic gpio(GPH2, GPH3, GPH4) initialisation*/
	set_pmic_gpio();
}

/* temporally used */

void _hw_version_check();

static void __init smdkc110_machine_init(void)
{

	int hwver = -1;

 #if 1 //20100525_inchul
 	printk("\n");
 	printk("--------------------------------------\n");
 	printk("HW Board revision :  rev %x \n", system_rev);
 	printk("--------------------------------------\n");
 	printk("\n");
#endif

	_hw_version_check();


	jupiter_init_gpio();
	/* i2c */
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
#if 1//CONFIG_ARIES_VER_B1... pfe
	i2c_register_board_info(9, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#else
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif
	//pmic
	i2c_register_board_info(6, i2c_devs2, ARRAY_SIZE(i2c_devs2));

    #if 0 //suik_Check //victory.boot.keyboard
	i2c_register_board_info(7, max7359_kbd_i2c_info, ARRAY_SIZE(max7359_kbd_i2c_info));//victory.boot
    s3c_gpio_cfgpin( GPIO_KEY_INT, GPIO_KEY_INT_AF );
	s3c_gpio_setpull( GPIO_KEY_INT, S3C_GPIO_PULL_NONE);
    #endif

	pm_power_off = smdkc110_power_off;

        /* spi */
#if defined(CONFIG_SPI_CNTRLR_0)
	s3cspi_set_slaves(BUSNUM(0), ARRAY_SIZE(s3c_slv_pdata_0), s3c_slv_pdata_0);
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	s3cspi_set_slaves(BUSNUM(1), ARRAY_SIZE(s3c_slv_pdata_1), s3c_slv_pdata_1);
#endif
#if defined(CONFIG_SPI_CNTRLR_2)
	s3cspi_set_slaves(BUSNUM(2), ARRAY_SIZE(s3c_slv_pdata_2), s3c_slv_pdata_2);
#endif
	spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));
#ifdef CONFIG_FB_S3C_LTE480WV
	s3cfb_set_platdata(&lte480wv_data);
#endif
#if defined (CONFIG_TOUCHSCREEN_S3C)
	s3c_ts_set_platdata(&s3c_ts_platform);
#endif

#if defined(CONFIG_S5PC11X_ADC)
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif


//	universal_wm8994_init();
	universal_sdhci2_set_platdata();

#ifdef CONFIG_FB_S3C_TL2796
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
#endif
#ifdef CONFIG_FB_S3C_LVDS
	s3cfb_set_platdata(&lvds_data);
#endif
	/* pmem */
#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif
	/* fimc */
	hwver = read_hwversion();

	if(hwver == 1)
		ce147.i2c_busnum = 2;//if B4, i2c_busnum = 2
	else
		ce147.i2c_busnum = 0;//if not B4, i2c_busnum = 0

	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
	s3c_csis_set_platdata(NULL);


	/* ipc */
	s3c_ipc_set_platdata(NULL);

	platform_add_devices(smdkc110_devices, ARRAY_SIZE(smdkc110_devices));

#if defined(CONFIG_PM)
	s5pc11x_pm_init();
#endif

#if defined(CONFIG_HAVE_PWM)
	smdk_backlight_register();
#endif

#if 0 //etinum.victory.boot
	s3c_gpio_cfgpin( AP_I2C_SCL_28V, 1 );
	s3c_gpio_setpull( AP_I2C_SCL_28V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin( AP_I2C_SDA_28V, 1 );
	s3c_gpio_setpull( AP_I2C_SDA_28V, S3C_GPIO_PULL_UP);
#endif

//Thomas Ryu 20100512, recovered from Revision #1.
register_reboot_notifier(&arise_reboot_notifier);

	jupiter_switch_init();

	//MSENSE reset high
	gpio_direction_output(GPIO_MSENSE_nRST, 1);

#if 0 //victory.boot.temp
	s3c_gpio_cfgpin(GPIO_HWREV_MODE0, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE0, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE1, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE1, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE2, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE2, S3C_GPIO_PULL_NONE);
	HWREV = gpio_get_value(GPIO_HWREV_MODE0);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE1) <<1);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE2) <<2);
	#ifndef CONFIG_JUPITER_VER_B4
	s3c_gpio_cfgpin(GPIO_HWREV_MODE3, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE3, S3C_GPIO_PULL_NONE);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE3) <<3);
	#endif
	printk("HWREV is 0x%x\n", HWREV);
	msleep(100);
#endif
}


MACHINE_START(SMDKC110, "SPH-D700")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC11X_PA_SDRAM + 0x100,
	.fixup		= smdkc110_fixup,
	.init_irq	= s5pc110_init_irq,
	.map_io		= smdkc110_map_io,
	.init_machine	= smdkc110_machine_init,
	#ifndef CONFIG_HIGH_RES_TIMERS
	.timer		= &s5pc11x_timer,
	#else
	.timer		=&sec_timer,
	#endif
MACHINE_END


/* this function are used to detect s5pc110 chip version temporally */

int s5pc110_verion ;

void _hw_version_check()
{
	void __iomem * phy_address ;
	int temp;

	phy_address = ioremap (0x40,1);

	temp = __raw_readl(phy_address);


	if (temp == 0xE59F010C)
	{
		s5pc110_verion = 0;
	}
	else
	{
		s5pc110_verion=1 ;
	}
	printk("S5PC110 Hardware version : EVT%d \n",s5pc110_verion);

	iounmap(phy_address);

}

/* Temporally used
 * return value 0 -> EVT 0
 * value 1 -> evt 1
 */

int hw_version_check()
{
	return s5pc110_verion ;
}


#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;
#if 0 //Garnett do nothing..
	end = S5PC11X_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5PC11X_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));

//		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}

	end = S5PC11X_GPH2(columns);

	/* Set all the necessary GPH pins to special-function 0 */
	for (gpio = S5PC11X_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
#endif
}

//void	s5pc11x_config_gpio_table ( int array_size, int **gpio_table

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#elif defined(CONFIG_ARIES_VER_B1) //hojun_kim for keypad
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;


	/* Key Sense GPIO Set all the necessary GPH pins to special-function 0 */
	for (gpio = S5PC11X_GPH3(0); gpio <= S5PC11X_GPH3(5); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));

		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	for (gpio = S5PC11X_GPJ4(0); gpio <= S5PC11X_GPJ4(3); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));

		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* Key Scan GPIO Set all the necessary GPH2 pins to special-function 0 */

	for (gpio = S5PC11X_GPH2(0); gpio <= S5PC11X_GPH2(5); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

//void	s5pc11x_config_gpio_table ( int array_size, int **gpio_table

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	switch(port)
	{
	case 0:
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_SFN(GPIO_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_SFN(GPIO_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_SFN(GPIO_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_SFN(GPIO_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_TXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_CTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 1:
        #if 1 //victory.boot
		s3c_gpio_cfgpin(GPIO_ATSC_UART_RXD, S3C_GPIO_SFN(GPIO_ATSC_UART_RXD_AF));
		s3c_gpio_setpull(GPIO_ATSC_UART_RXD, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_ATSC_UART_TXD, S3C_GPIO_SFN(GPIO_ATSC_UART_TXD_AF));
		s3c_gpio_setpull(GPIO_ATSC_UART_TXD, S3C_GPIO_PULL_NONE);
        #else
		s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
		s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
		s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
		s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
		s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_NONE);
        #endif
		break;
	case 2:
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
		break;
	case 3:
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);
EXPORT_SYMBOL(s3c_config_gpio_table);
EXPORT_SYMBOL(hw_version_check);
