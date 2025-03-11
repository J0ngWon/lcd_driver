#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
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
__used __section("__versions") = {
	{ 0xacad428b, "i2c_register_driver" },
	{ 0xbe726d7c, "device_remove_file" },
	{ 0x2f19d425, "_dev_info" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x873a069b, "i2c_del_driver" },
	{ 0xb52b8d37, "i2c_smbus_write_byte" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x4829a47e, "memcpy" },
	{ 0x122c3a7e, "_printk" },
	{ 0xe914e41e, "strcpy" },
	{ 0xf9a482f9, "msleep" },
	{ 0x37a0cba, "kfree" },
	{ 0xf61c822d, "devm_kmalloc" },
	{ 0x40ae8b3c, "device_create_file" },
	{ 0x773354b7, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cmycomp,lcd_i2c");
MODULE_ALIAS("of:N*T*Cmycomp,lcd_i2cC*");
MODULE_ALIAS("i2c:lcd_i2c");

MODULE_INFO(srcversion, "F647A7E9DBAAC05EA108F0D");
