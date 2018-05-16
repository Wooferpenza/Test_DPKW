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
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0x6bf1c17f, __VMLINUX_SYMBOL_STR(pv_lock_ops) },
	{ 0xbd573d7b, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x3eea1a02, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x6a241051, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0x9e1b2ca4, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xfd5cd3ae, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xe259ae9e, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0x23e1714d, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x9ff9d0c, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0xad99899c, __VMLINUX_SYMBOL_STR(try_module_get) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "F1A82076A2E784096725794");
