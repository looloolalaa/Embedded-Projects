#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xad1a7def, "module_layout" },
	{ 0x37a0cba, "kfree" },
	{ 0xfe990052, "gpio_free" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x1d11aa3a, "cdev_del" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xd6b8e852, "request_threaded_irq" },
	{ 0x72c365ef, "gpiod_to_irq" },
	{ 0x350a004, "gpio_to_desc" },
	{ 0x403f9529, "gpio_request_one" },
	{ 0x88647e86, "cdev_add" },
	{ 0x7a3acc89, "cdev_init" },
	{ 0xb4673642, "cdev_alloc" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x9ec6ca96, "ktime_get_real_ts64" },
	{ 0x8899dc97, "kmem_cache_alloc_trace" },
	{ 0x61a76852, "kmalloc_caches" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0x39a12ca7, "_raw_spin_unlock_irqrestore" },
	{ 0x5f849a69, "_raw_spin_lock_irqsave" },
	{ 0x7c32d0f0, "printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1820800C4BF9FAE28945801");
