/*
 * a simple kernel module: fs
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "simple.h"

static struct kmem_cache *sfs_inode_cachep;

int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
	
}

static struct dentry *simplefs_mount(struct file_system_type *fs_type,
			int flags, const char *dev_name, void *data)
{
	struct dentry *ret;

	printk(KERN_INFO "%s start\n", __func__);
	ret = mount_bdev(fs_type, flag, dev_name, data, simplefs_fill_super);

	if (unlikely(IS_ERR(ret)))
		printk(KERN_ERR "Error mounting simplefs");
	else
		printk(KERN_INFO "simplefs is successfully mount on [%s]", dev_name);

	return ret;
}

static void simplefs_kill_superblock(struct super_block *sb)
{
	printk(KERN_INFO "simplefs superblock is destroyed unmount successful.\n");
	
	kill_block_super(sb);
}

struct file_system_type simplefs_fs_type = {
	.owner = THIS_MODULE,
	.name = "simplefs",
	.mount = simplefs_mount,
	.kill_sb = simplefs_kill_superblock,
	.fs_flags = FS_REQUIRES_DEV,
};

static int __init simplefs_init(void)
{
	int ret;

	printk(KERN_INFO "simplefs init\n");

	sfs_inode_cachep = kmem_cache_create("sfs_inode_cache",
				sizeof(struct simplefs_inode),
				0,
				(SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD),
				NULL);

	if (!sfs_inode_cachep) {
		return -ENOMEM;
	}

	ret = register_filesystem(&simplefs_fs_type);
	if (likely(ret == 0))
		printk(KERN_INFO "register filesystem\n");
	else
		printk(KERN_ERR "fail\n");

	return 0;
}


static void __exit simplefs_exit(void)
{
	int ret;
	printk(KERN_INFO "simplefs exit\n");

	ret = unregister_filesystem(&simplefs_fs_type);
	kmem_cache_destroy(sfs_inode_cachep);
}

module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a simple fs module");
MODULE_ALIAS("a simple fs module");
