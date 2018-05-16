#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0xa2f7d132, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xe9423d3f, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x495ded38, __VMLINUX_SYMBOL_STR(usb_kill_urb) },
	{ 0xeae3dfd6, __VMLINUX_SYMBOL_STR(__const_udelay) },
	{ 0x84cc4934, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x9e88526, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xfd91fb91, __VMLINUX_SYMBOL_STR(usb_deregister) },
	{ 0xc277280a, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x4c9d28b0, __VMLINUX_SYMBOL_STR(phys_base) },
	{ 0x43ded46b, __VMLINUX_SYMBOL_STR(ldev_remove) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0x8460cfc, __VMLINUX_SYMBOL_STR(usb_control_msg) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0x55701cc3, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xdf656215, __VMLINUX_SYMBOL_STR(usb_free_coherent) },
	{ 0xa0ef33de, __VMLINUX_SYMBOL_STR(usb_submit_urb) },
	{ 0xf2882a90, __VMLINUX_SYMBOL_STR(usb_get_dev) },
	{ 0x93fca811, __VMLINUX_SYMBOL_STR(__get_free_pages) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xcc8decdf, __VMLINUX_SYMBOL_STR(usb_bulk_msg) },
	{ 0xa1ff976b, __VMLINUX_SYMBOL_STR(usb_put_dev) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0xa202a8e5, __VMLINUX_SYMBOL_STR(kmalloc_order_trace) },
	{ 0xcd19f8de, __VMLINUX_SYMBOL_STR(usb_clear_halt) },
	{ 0x33c5d52c, __VMLINUX_SYMBOL_STR(usb_find_interface) },
	{ 0x380724e9, __VMLINUX_SYMBOL_STR(ldev_add) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0x85d4c68, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x4302d0eb, __VMLINUX_SYMBOL_STR(free_pages) },
	{ 0xa6bbd805, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x2207a57f, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xcea7e1d5, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x1aa1a54, __VMLINUX_SYMBOL_STR(ldev_register) },
	{ 0x74b83f6a, __VMLINUX_SYMBOL_STR(usb_register_driver) },
	{ 0xf08242c2, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x3113e264, __VMLINUX_SYMBOL_STR(usb_alloc_coherent) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xfd808436, __VMLINUX_SYMBOL_STR(usb_free_urb) },
	{ 0x77d19e0f, __VMLINUX_SYMBOL_STR(usb_alloc_urb) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=ldevice";

MODULE_ALIAS("usb:v0471p0440d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0471p0140d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0471p2010d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0471p0154d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "B69AD5D4B5DC5D823F4FF46");
