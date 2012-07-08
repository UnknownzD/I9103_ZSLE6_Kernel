/*  drivers/misc/sec_jack.c
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
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
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/sec_jack.h>
#ifdef CONFIG_MACH_BOSE_ATT
#include <linux/mfd/stmpe.h>
#endif
#ifndef CONFIG_MACH_BOSE_ATT
#include <mach/suspend.h>
#endif

#define MAX_ZONE_LIMIT		10
#define SEND_KEY_CHECK_TIME_MS	30		/* 30ms */
#define DET_CHECK_TIME_MS	200		/* 200ms */
#define WAKE_LOCK_TIME		(HZ * 5)	/* 5 sec */

#ifdef CONFIG_MACH_BOSE_ATT
extern struct stmpe *g_stmpe;
extern int stmpe_reg_read(struct stmpe *stmpe, u8 reg);
#endif

struct sec_jack_info {
	struct sec_jack_platform_data *pdata;
	struct delayed_work jack_detect_work;
	struct work_struct buttons_work;
	struct workqueue_struct *queue;
	struct input_dev *input_dev;
	struct wake_lock det_wake_lock;
	struct sec_jack_zone *zone;
	struct input_handler handler;
	struct input_handle handle;
	struct input_device_id ids;
	int det_irq;
	int dev_id;
	int pressed;
	int pressed_code;
	struct platform_device *send_key_dev;
	unsigned int cur_jack_type;
	int boot_state;
#ifndef CONFIG_MACH_BOSE_ATT
	struct work_struct buttons_work_det; /* HACK DET worker from SEND_END intr */
	struct workqueue_struct *queue_det;  /* HACK DET queue from SEND_END intr */
#endif
};
#ifdef CONFIG_MACH_BOSE_ATT
struct sec_jack_info *g_jack_info;
#endif
/* with some modifications like moving all the gpio structs inside
 * the platform data and getting the name for the switch and
 * gpio_event from the platform data, the driver could support more than
 * one headset jack, but currently user space is looking only for
 * one key file and switch for a headset so it'd be overkill and
 * untestable so we limit to one instantiation for now.
 */
static atomic_t instantiated = ATOMIC_INIT(0);

/* sysfs name HeadsetObserver.java looks for to track headset state
 */
struct switch_dev switch_jack_detection = {
	.name = "h2w",
};

/* To support AT+FCESTEST=1 */
struct switch_dev switch_sendend = {
		.name = "send_end",
};

static struct gpio_event_direct_entry sec_jack_key_map[] = {
	{
		.code	= KEY_UNKNOWN,
	},
};

static struct gpio_event_input_info sec_jack_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.type = EV_KEY,
	.debounce_time.tv.nsec = SEND_KEY_CHECK_TIME_MS * NSEC_PER_MSEC,
	.keymap = sec_jack_key_map,
	.keymap_size = ARRAY_SIZE(sec_jack_key_map)
};

static struct gpio_event_info *sec_jack_input_info[] = {
	&sec_jack_key_info.info,
};

static struct gpio_event_platform_data sec_jack_input_data = {
	.name = "sec_jack",
	.info = sec_jack_input_info,
	.info_count = ARRAY_SIZE(sec_jack_input_info),
};

#ifndef CONFIG_MACH_BOSE_ATT
int sec_jack_detect_on_buttons_filter(struct work_struct *work);
#endif

/* gpio_input driver does not support to read adc value.
 * We use input filter to support 3-buttons of headset
 * without changing gpio_input driver.
 */
static bool sec_jack_buttons_filter(struct input_handle *handle,
				    unsigned int type, unsigned int code,
				    int value)
{
	struct sec_jack_info *hi = handle->handler->private;

	if (type != EV_KEY || code != KEY_UNKNOWN)
		return false;

	hi->pressed = value;

	/* This is called in timer handler of gpio_input driver.
	 * We use workqueue to read adc value.
	 */
	queue_work(hi->queue, &hi->buttons_work);

	return true;
}

static int sec_jack_buttons_connect(struct input_handler *handler,
				    struct input_dev *dev,
				    const struct input_device_id *id)
{
	struct sec_jack_info *hi;
	struct sec_jack_platform_data *pdata;
	struct sec_jack_buttons_zone *btn_zones;
	int err;
	int i;

	/* bind input_handler to input device related to only sec_jack */
	if (dev->name != sec_jack_input_data.name)
		return -ENODEV;

	hi = handler->private;
	pdata = hi->pdata;
	btn_zones = pdata->buttons_zones;

	hi->input_dev = dev;
	hi->handle.dev = dev;
	hi->handle.handler = handler;
	hi->handle.open = 0;
	hi->handle.name = "sec_jack_buttons";

	err = input_register_handle(&hi->handle);
	if (err) {
		pr_err("%s: Failed to register sec_jack buttons handle, "
			"error %d\n", __func__, err);
		goto err_register_handle;
	}

	err = input_open_device(&hi->handle);
	if (err) {
		pr_err("%s: Failed to open input device, error %d\n",
			__func__, err);
		goto err_open_device;
	}

	for (i = 0; i < pdata->num_buttons_zones; i++)
		input_set_capability(dev, EV_KEY, btn_zones[i].code);

	return 0;

err_open_device:
	input_unregister_handle(&hi->handle);
err_register_handle:

	return err;
}

static void sec_jack_buttons_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
}

extern void wm8994_jack_changed(void);

static void sec_jack_set_type(struct sec_jack_info *hi, int jack_type)
{
	struct sec_jack_platform_data *pdata = hi->pdata;

	/* this can happen during slow inserts where we think we identified
	 * the type but then we get another interrupt and do it again
	 */
	if (jack_type == hi->cur_jack_type)
		return;

	if (jack_type == SEC_HEADSET_4POLE) {
		/* for a 4 pole headset, enable detection of send/end key */
		if (hi->send_key_dev == NULL)
			/* enable to get events again */
			hi->send_key_dev = platform_device_register_data(NULL,
					GPIO_EVENT_DEV_NAME,
					hi->dev_id,
					&sec_jack_input_data,
					sizeof(sec_jack_input_data));
	} else {
		/* for all other jacks, disable send/end key detection */
		if (hi->send_key_dev != NULL) {
			/* disable to prevent false events on next insert */
			platform_device_unregister(hi->send_key_dev);
			hi->send_key_dev = NULL;
		}
		/* micbias is left enabled for 4pole and disabled otherwise */
#ifdef CONFIG_MACH_BOSE_ATT
		pdata->set_micbias_state(false);
#endif
	}

	hi->cur_jack_type = jack_type;
	hi->pdata->jack_status = jack_type;
	pr_info("%s : jack_type = %d\n", __func__, jack_type);

	/* prevent suspend to allow user space to respond to switch */
	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME);

	switch_set_state(&switch_jack_detection, jack_type);
#if defined(CONFIG_MACH_N1_CHN)
	g_headset_status = jack_type;
	pr_err("%s: g_headset_status =%d !!\n", __func__, g_headset_status);
#endif
#if defined(CONFIG_MACH_N1_CHN)
	g_headset_status = jack_type;
	//pr_err("%s: g_headset_status =%d !!\n", __func__, g_headset_status);
#endif
	if(jack_type)
		wm8994_jack_changed();
}

static void handle_jack_not_inserted(struct sec_jack_info *hi)
{
#if defined(CONFIG_MACH_N1_CHN)
	pr_err("%s: sec_jack_set_type(hi, SEC_JACK_NO_DEVICE) !!\n", __func__);
#endif
#if defined(CONFIG_MACH_N1_CHN)
	//pr_err("%s: sec_jack_set_type(hi, SEC_JACK_NO_DEVICE) !!\n", __func__);
#endif
	sec_jack_set_type(hi, SEC_JACK_NO_DEVICE);
#ifndef CONFIG_MACH_BOSE_ATT
	if (!hi->boot_state) {
		msleep(500);
	} else
		hi->boot_state = 0;
#endif
	hi->pdata->set_micbias_state(false);
}

static void determine_jack_type(struct sec_jack_info *hi)
{
	struct sec_jack_zone *zones = hi->pdata->zones;
	struct sec_jack_platform_data *pdata = hi->pdata;
	int size = hi->pdata->num_zones;
	int count[MAX_ZONE_LIMIT] = {0};
	int adc;
	int i;
	unsigned npolarity = !hi->pdata->det_active_high;

	/* set mic bias to enable adc */
	pdata->set_micbias_state(true);
#ifndef CONFIG_MACH_BOSE_ATT
	msleep(300);
#else
	msleep(200);
#endif
	while (gpio_get_value(hi->pdata->det_gpio) ^ npolarity) {
		adc = hi->pdata->get_adc_value();
		pr_info("%s: adc = %d\n", __func__, adc);

		/* determine the type of headset based on the
		 * adc value.  An adc value can fall in various
		 * ranges or zones.  Within some ranges, the type
		 * can be returned immediately.  Within others, the
		 * value is considered unstable and we need to sample
		 * a few more types (up to the limit determined by
		 * the range) before we return the type for that range.
		 */
		for (i = 0; i < size; i++) {
			if (adc <= zones[i].adc_high) {
				if (++count[i] > zones[i].check_count) {
					sec_jack_set_type(hi,
							  zones[i].jack_type);
					hi->boot_state = 0;
					return;
				}
				msleep(zones[i].delay_ms);
				break;
			}
		}
	}
	/* jack removed before detection complete */
	pr_info("%s : jack removed before detection complete\n", __func__);
	handle_jack_not_inserted(hi);
}
#ifdef CONFIG_MACH_BOSE_ATT
static void determine_jack_type_bose(struct sec_jack_info *hi)
{

	struct sec_jack_zone *zones = hi->pdata->zones;
	struct sec_jack_platform_data *pdata = hi->pdata;
	int size = hi->pdata->num_zones;
	int count[MAX_ZONE_LIMIT] = {0};
	int adc;
	int i;
	unsigned npolarity = !hi->pdata->det_active_high;

	/* set mic bias to enable adc */
	pdata->set_micbias_state(true);

	while(( (stmpe_reg_read(g_stmpe,0x18) & 0x02 ) ^ npolarity )){
		adc = hi->pdata->get_adc_value();
		pr_info("%s: adc = %d\n", __func__, adc);

		/* determine the type of headset based on the
		 * adc value.  An adc value can fall in various
		 * ranges or zones.  Within some ranges, the type
		 * can be returned immediately.  Within others, the
		 * value is considered unstable and we need to sample
		 * a few more types (up to the limit determined by
		 * the range) before we return the type for that range.
		 */
		for (i = 0; i < size; i++) {
			if (adc <= zones[i].adc_high) {
				if (++count[i] > zones[i].check_count) {
					sec_jack_set_type(hi,
							  zones[i].jack_type);
					return;
				}
				msleep(zones[i].delay_ms);
				break;
			}
		}
	}
	/* jack removed before detection complete */
	pr_info("%s : jack removed before detection complete\n", __func__);
	handle_jack_not_inserted(hi);
}
#endif
/* thread run whenever the headset detect state changes (either insertion
 * or removal).
 */
static irqreturn_t sec_jack_detect_irq_thread(int irq, void *dev_id)
{
	struct sec_jack_info *hi = dev_id;
	int time_left_ms = DET_CHECK_TIME_MS;
	unsigned npolarity = !hi->pdata->det_active_high;

	pr_info("%s : cur_jack_type=%d\n", __func__, hi->cur_jack_type);

	/* debounce headset jack.  don't try to determine the type of
	 * headset until the detect state is true for a while.
	 */
	while (time_left_ms > 0) {
		if (!(gpio_get_value(hi->pdata->det_gpio) ^ npolarity)) {
			/* jack not detected. */
			handle_jack_not_inserted(hi);
			return IRQ_HANDLED;
		}
		usleep_range(10000, 20000);
		time_left_ms -= 10;
	}
	/* jack presence was detected the whole time, figure out which type */
	determine_jack_type(hi);
	return IRQ_HANDLED;
}

#ifdef CONFIG_MACH_BOSE_ATT
int sec_jack_detect_irq(void)
{

	struct sec_jack_info *hi = g_jack_info;
	int time_left_ms = DET_CHECK_TIME_MS; 
	unsigned npolarity = !hi->pdata->det_active_high;
	while (time_left_ms > 0) {

		if(! ((stmpe_reg_read(g_stmpe,0x18) & 0x02 ) ^ npolarity)) {

			handle_jack_not_inserted(hi); 
			return 1;
		}
		usleep_range(10000, 20000);
		time_left_ms -= 10;
	}

	determine_jack_type_bose(hi); 

	return 1;
}
EXPORT_SYMBOL( sec_jack_detect_irq );
#endif

/* thread run whenever the button of headset is pressed or released */
void sec_jack_buttons_work(struct work_struct *work)
{
	struct sec_jack_info *hi =
		container_of(work, struct sec_jack_info, buttons_work);
	struct sec_jack_platform_data *pdata = hi->pdata;
	struct sec_jack_buttons_zone *btn_zones = pdata->buttons_zones;
	int adc;
	int i;

	/* when button is released */
	if (hi->pressed == 0) {
		input_report_key(hi->input_dev, hi->pressed_code, 0);
		if(hi->pressed_code == KEY_MEDIA)
			switch_set_state(&switch_sendend, 0);
		input_sync(hi->input_dev);
		pr_info("%s: keycode=%d, is released\n", __func__,
			hi->pressed_code);
		return;
	}

	/* when button is pressed */
	adc = pdata->get_adc_value();

	for (i = 0; i < pdata->num_buttons_zones; i++)
		if (adc >= btn_zones[i].adc_low &&
		    adc <= btn_zones[i].adc_high) {
			hi->pressed_code = btn_zones[i].code;
			input_report_key(hi->input_dev, btn_zones[i].code, 1);
			if(hi->pressed_code == KEY_MEDIA)
				switch_set_state(&switch_sendend, 1);
			input_sync(hi->input_dev);
			pr_info("%s: keycode=%d, is pressed\n", __func__,
				btn_zones[i].code);
			return;
		}

	pr_warn("%s: key is skipped. ADC value is %d\n", __func__, adc);
}

#ifndef CONFIG_MACH_BOSE_ATT
int sec_jack_detect_on_buttons_filter(struct work_struct *work)
{
	struct sec_jack_info *hi = container_of(work, struct sec_jack_info, buttons_work_det);
	if (gpio_get_value(hi->pdata->det_gpio) && hi->cur_jack_type) {
		pr_info("%s : headset removed\n", __func__);
		determine_jack_type(hi);
	}
	return 1;
}
#endif

static int sec_jack_probe(struct platform_device *pdev)
{
	struct sec_jack_info *hi;
	struct sec_jack_platform_data *pdata = pdev->dev.platform_data;
	int ret;

	pr_info("%s : Registering jack driver\n", __func__);
	if (!pdata) {
		pr_err("%s : pdata is NULL.\n", __func__);
		return -ENODEV;
	}

	if (!pdata->get_adc_value || !pdata->zones ||
	    !pdata->set_micbias_state || pdata->num_zones > MAX_ZONE_LIMIT) {
		pr_err("%s : need to check pdata\n", __func__);
		return -ENODEV;
	}

	if (atomic_xchg(&instantiated, 1)) {
		pr_err("%s : already instantiated, can only have one\n",
			__func__);
		return -ENODEV;
	}

	sec_jack_key_map[0].gpio = pdata->send_end_gpio;

	hi = kzalloc(sizeof(struct sec_jack_info), GFP_KERNEL);
	if (hi == NULL) {
		pr_err("%s : Failed to allocate memory.\n", __func__);
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	hi->pdata = pdata;

	/* make the id of our gpi_event device the same as our platform device,
	 * which makes it the responsiblity of the board file to make sure
	 * it is unique relative to other gpio_event devices
	 */
	hi->dev_id = pdev->id;

	ret = gpio_request(pdata->det_gpio, "ear_jack_detect");
	if (ret) {
		pr_err("%s : gpio_request failed for %d\n",
		       __func__, pdata->det_gpio);
		goto err_gpio_request;
	}

	ret = switch_dev_register(&switch_jack_detection);
	if (ret < 0) {
		pr_err("%s : Failed to register switch device\n", __func__);
		goto err_switch_dev_register;
	}

	ret = switch_dev_register(&switch_sendend);
	if (ret < 0) {
		printk(KERN_ERR "SEC JACK: Failed to register switch device\n");
		goto err_switch_dev_register_send_end;
	}
	wake_lock_init(&hi->det_wake_lock, WAKE_LOCK_SUSPEND, "sec_jack_det");
#ifndef CONFIG_MACH_BOSE_ATT
	INIT_WORK(&hi->buttons_work_det, sec_jack_detect_on_buttons_filter);
	hi->queue_det = create_singlethread_workqueue("sec_jack_wq_det");
	if (hi->queue_det == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to create workqueue\n", __func__);
		goto err_create_wq_failed;
	}
#endif
	INIT_WORK(&hi->buttons_work, sec_jack_buttons_work);
	hi->queue = create_singlethread_workqueue("sec_jack_wq");
	if (hi->queue == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to create workqueue\n", __func__);
		goto err_create_wq_failed;
	}

	hi->det_irq = gpio_to_irq(pdata->det_gpio);

	set_bit(EV_KEY, hi->ids.evbit);
	hi->ids.flags = INPUT_DEVICE_ID_MATCH_EVBIT;
	hi->handler.filter = sec_jack_buttons_filter;
	hi->handler.connect = sec_jack_buttons_connect;
	hi->handler.disconnect = sec_jack_buttons_disconnect;
	hi->handler.name = "sec_jack_buttons";
	hi->handler.id_table = &hi->ids;
	hi->handler.private = hi;
	hi->boot_state = 1;

	ret = input_register_handler(&hi->handler);
	if (ret) {
		pr_err("%s : Failed to register_handler\n", __func__);
		goto err_register_input_handler;
	}
#ifdef CONFIG_MACH_BOSE_ATT
	if(system_rev < 0x09){	
		ret = request_threaded_irq(hi->det_irq, NULL,
					   sec_jack_detect_irq_thread,
					   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
					   IRQF_ONESHOT, "sec_headset_detect", hi);
		if (ret) {
			pr_err("%s : Failed to request_irq.\n", __func__);
			goto err_request_detect_irq;
		}

		enable_irq(hi->det_irq);
	}
#else
	ret = request_threaded_irq(hi->det_irq, NULL,
				   sec_jack_detect_irq_thread,
				   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
				   IRQF_ONESHOT, "sec_headset_detect", hi);
	if (ret) {
		pr_err("%s : Failed to request_irq.\n", __func__);
		goto err_request_detect_irq;
	}
	/* to handle insert/removal when we're sleeping in a call */
	ret = enable_irq_wake(hi->det_irq);
	if (ret) {
		pr_err("%s : Failed to enable_irq_wake.\n", __func__);
		goto err_enable_irq_wake;
	}
#endif
	dev_set_drvdata(&pdev->dev, hi);

#ifdef CONFIG_MACH_BOSE_ATT
	g_jack_info = hi;
#endif
	pdata->set_micbias_state(true);

#ifdef CONFIG_MACH_BOSE_ATT
	if(system_rev >= 0x09)
		determine_jack_type_bose(hi);
	else
		determine_jack_type(hi);
#else
	determine_jack_type(hi);
#endif
	return 0;

err_enable_irq_wake:
	free_irq(hi->det_irq, hi);
err_request_detect_irq:
	input_unregister_handler(&hi->handler);
err_register_input_handler:
	destroy_workqueue(hi->queue);
err_create_wq_failed:
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
err_switch_dev_register_send_end:
	switch_dev_unregister(&switch_jack_detection);
err_switch_dev_register:
	gpio_free(pdata->det_gpio);
err_gpio_request:
	kfree(hi);
err_kzalloc:
	atomic_set(&instantiated, 0);

	return ret;
}

static int sec_jack_remove(struct platform_device *pdev)
{

	struct sec_jack_info *hi = dev_get_drvdata(&pdev->dev);

	pr_info("%s :\n", __func__);
#ifdef CONFIG_MACH_BOSE_ATT	
	g_jack_info = NULL;
	if(system_rev < 0x09)
		disable_irq_wake(hi->det_irq);
#else
	disable_irq_wake(hi->det_irq);
#endif
	free_irq(hi->det_irq, hi);
	destroy_workqueue(hi->queue);
	if (hi->send_key_dev) {
		platform_device_unregister(hi->send_key_dev);
		hi->send_key_dev = NULL;
	}
	input_unregister_handler(&hi->handler);
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
	switch_dev_unregister(&switch_jack_detection);
	gpio_free(hi->pdata->det_gpio);
	kfree(hi);
	atomic_set(&instantiated, 0);

	return 0;
}

#ifndef CONFIG_MACH_BOSE_ATT
static int sec_jack_suspend(struct platform_device *pdev)
{
	struct sec_jack_info *hi = dev_get_drvdata(&pdev->dev);

	return 0;
}

static int sec_jack_resume(struct platform_device *pdev)
{
	struct sec_jack_info *hi = dev_get_drvdata(&pdev->dev);

	pr_info("%s: Current_suspend_mode = %d \n",
			__func__, tegra_get_current_suspend_mode());     
	if (tegra_get_current_suspend_mode() == TEGRA_SUSPEND_LP0) {
		queue_work(hi->queue_det, &hi->buttons_work_det);
	}

	return 0;
}
#endif

static struct platform_driver sec_jack_driver = {
	.probe = sec_jack_probe,
	.remove = sec_jack_remove,
#ifndef CONFIG_MACH_BOSE_ATT
	.suspend = sec_jack_suspend,
	.resume = sec_jack_resume,
#endif
	.driver = {
			.name = "sec_jack",
			.owner = THIS_MODULE,
		   },
};
static int __init sec_jack_init(void)
{
	return platform_driver_register(&sec_jack_driver);
}

static void __exit sec_jack_exit(void)
{
	platform_driver_unregister(&sec_jack_driver);
}

late_initcall(sec_jack_init);
module_exit(sec_jack_exit);

MODULE_AUTHOR("ms17.kim@samsung.com");
MODULE_DESCRIPTION("Samsung Electronics Corp Ear-Jack detection driver");
MODULE_LICENSE("GPL");
