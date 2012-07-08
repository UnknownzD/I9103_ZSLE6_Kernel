#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x9025e135, "module_layout" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0xc7bb28f, "i2c_master_send" },
	{ 0xeef3277a, "kmalloc_caches" },
	{ 0xf9a482f9, "msleep" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x20b07433, "dev_set_drvdata" },
	{ 0xa687712d, "i2c_del_driver" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x4aabc7c4, "__tracepoint_kmalloc" },
	{ 0xc777d3e0, "queue_work" },
	{ 0x432fd7f6, "__gpio_set_value" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x2883a4b1, "mutex_unlock" },
	{ 0xaf8aa518, "system_rev" },
	{ 0xa394bb91, "nonseekable_open" },
	{ 0xc8c71d87, "s3c_gpio_setpull" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x52e7113e, "__init_waitqueue_head" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0x56eabd1a, "misc_register" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x5f754e5a, "memset" },
	{ 0xde75b689, "set_irq_type" },
	{ 0xe2470572, "kmem_cache_alloc_notrace" },
	{ 0x5b933b01, "__mutex_init" },
	{ 0xea147363, "printk" },
	{ 0xa8f59416, "gpio_direction_output" },
	{ 0x2918ea29, "mutex_lock" },
	{ 0x4e0e78c9, "destroy_workqueue" },
	{ 0x859c6dc7, "request_threaded_irq" },
	{ 0x7f8bf9c9, "__create_workqueue_key" },
	{ 0x43b0c9c3, "preempt_schedule" },
	{ 0xd27273a, "i2c_register_driver" },
	{ 0x11f447ce, "__gpio_to_irq" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x1000e51, "schedule" },
	{ 0x9362f620, "i2c_master_recv" },
	{ 0xcd11e130, "__wake_up" },
	{ 0x6d4d6df5, "s3c_gpio_cfgpin" },
	{ 0xfe990052, "gpio_free" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x37a0cba, "kfree" },
	{ 0xa4c0d492, "prepare_to_wait" },
	{ 0x675b24bc, "finish_wait" },
	{ 0xb32505f3, "dev_get_drvdata" },
	{ 0x923bcc93, "misc_deregister" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("i2c:Si4709");
