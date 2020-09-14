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
#include <linux/time.h>
#include <linux/buffer_head.h>

#include "simplefs.h"

struct inode *simplefs_get_inode(struct super_block *sb,
			const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode *inode = new_inode(sb);
	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_kernel_time();
		printk(KERN_INFO "i_ino:%lu mode:0x%x\n", inode->i_ino, inode->i_mode);
		switch (inode->i_mode & S_IFMT) {
		case S_IFLNK:
			printk(KERN_INFO "S_IFLNK:0x%x\n", S_IFLNK);
			break;
		case S_IFDIR:
			printk(KERN_INFO "S_IFDIR:0x%x\n", S_IFDIR);
			inc_nlink(inode);
			break;
		case S_IFREG:
			printk(KERN_INFO "S_IFREG:0x%x\n", S_IFREG);
			break;
		case S_IFBLK:
		case S_IFCHR:
		case S_IFSOCK:
		case S_IFIFO:
			printk(KERN_INFO "S_IFBLK S_IFCHR S_IFSOCK S_IFIFO\n");
			break;
		default:
			printk(KERN_INFO "%s(): Bogus i_mode %o for ino %lu\n",
				__func__, inode->i_mode, (unsigned long)inode->i_ino);
		}
	}
	return inode;
}

static ssize_t simplefs_read(struct file * filp, char __user * buf, size_t len, loff_t * ppos)
{
	printk(KERN_INFO "%s()\n", __func__);
	return 0;
}

static const struct file_operations simplefs_dir_operations = {
	.owner = THIS_MODULE,
	.read = simplefs_read,
};

static struct dentry *simplefs_lookup(struct inode *parent_inode,
				struct dentry *child_dentry, unsigned int flags)
{
	printk(KERN_INFO "%s()\n", __func__);
	return NULL;
}


static struct inode_operations simplefs_inode_ops = {
	.lookup = simplefs_lookup,
};

static int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct buffer_head *bh;
	struct simplefs_super_block *sb_disk;

	bh = (struct buffer_head *)sb_bread(sb, 0);	
	sb_disk = (struct simplefs_super_block *)bh->b_data;

	printk(KERN_INFO "The magic number obtained in disk is: [%d]\n",sb_disk->magic);
	printk(KERN_INFO "version: %d\n", sb_disk->version);
	printk(KERN_INFO "block size: %d\n", sb_disk->block_size);
	printk(KERN_INFO "free block: %d\n", sb_disk->free_blocks);

	sb->s_magic = SIMPLEFS_MAGIC;
	sb->s_fs_info = sb_disk;

	root_inode = new_inode(sb);
	root_inode->i_ino = SIMPLEFS_ROOT_INODE_NUMBER;
	inode_init_owner(root_inode, NULL, S_IFDIR);
	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_kernel_time();
	root_inode->i_op = &simplefs_inode_ops;
	root_inode->i_fop = &simplefs_dir_operations;
	root_inode->i_sb = sb;
	root_inode->i_private = &(sb_disk->root_inode);
	printk(KERN_INFO "i_ino:%lu mode:0x%x\n", root_inode->i_ino, root_inode->i_mode);

	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root) {
		printk("make fs root failed\n");
		return -ENOMEM;
	}

	return 0;
}

static struct dentry *simplefs_mount(struct file_system_type *fs_type,
						int flags, const char *dev_name, void *data)
{
	struct dentry *ret;

	ret = mount_bdev(fs_type, flags, dev_name, data, simplefs_fill_super);
	if (IS_ERR(ret)) {
		printk(KERN_ERR "error mount simplefs\n");	
	} else {
		printk(KERN_INFO "successfully mount %s\n", dev_name);
	}

	return ret;
}

static void simplefs_kill_superblock(struct super_block *s)
{
	printk(KERN_INFO "simplefs superblock is destroyed. Unmount succesful.\n");
	return;
}

struct file_system_type simplefs_fs_type = {
	.owner = THIS_MODULE,
	.name = "simplefs",
	.mount = simplefs_mount,
	.kill_sb = simplefs_kill_superblock,
};

static int __init simplefs_init(void)
{
	int ret;
	printk(KERN_INFO "simplefs enter\n");

	ret = register_filesystem(&simplefs_fs_type);
	if (!ret) {
		printk(KERN_INFO "successfully register simplefs\n");	
	} else {
		printk(KERN_ERR "failed register simplefs, err:%d\n", ret);	
	}
	return 0;
}


static void __exit simplefs_exit(void)
{
	int ret;

	printk(KERN_INFO "simplefs exit\n");

	ret = unregister_filesystem(&simplefs_fs_type);
	if (!ret) {
		printk(KERN_INFO "successfully unregister simplefs\n");
	} else {
		printk(KERN_INFO "failed unregister simplefs, err:%d\n", ret);
	}
}

module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
