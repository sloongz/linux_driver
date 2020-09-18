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
#include <linux/slab.h>
#include <linux/version.h>

#include "simplefs.h"

static DEFINE_MUTEX(simplefs_inodes_mgmt_lock);

static struct kmem_cache *sfs_inode_cachep;

static void simplefs_sb_sync(struct super_block *sb)
{
	struct buffer_head *bh;
	struct simplefs_super_block *sfs_sb = sb->s_fs_info;

	bh = sb_bread(sb, SIMPLEFS_SUPERBLOCK_BLOCK_NUMBER);
	bh->b_data = (char *)sfs_sb;
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);
}

static struct simplefs_inode *simplefs_get_inode(struct super_block *sb, uint64_t inode_no)
{
	struct simplefs_super_block *sfs_sb = sb->s_fs_info;
	struct simplefs_inode *sfs_inode = NULL;
	struct simplefs_inode *inode_buffer = NULL;
	int i;
	struct buffer_head *bh;

	bh = (struct buffer_head *)sb_bread(sb, SIMPLEFS_INODESTORE_BLOCK_NUMBER);
	sfs_inode = (struct simplefs_inode *)bh->b_data;
	
	for (i=0; i<sfs_sb->inodes_count; i++) {
		if (sfs_inode->inode_no == inode_no) {
			inode_buffer = kmem_cache_alloc(sfs_inode_cachep, GFP_KERNEL);
			memcpy(inode_buffer, sfs_inode, sizeof(struct simplefs_inode));
			break;
		}
		sfs_inode++;
	}

	brelse(bh);
	return inode_buffer;
}

static int simplefs_save_inode(struct super_block *sb, struct simplefs_inode *sfs_inode)
{
	struct simplefs_inode *changed_inode = NULL;
	struct simplefs_inode *inodep = NULL;
	struct simplefs_super_block *sfs_sb = NULL;
	struct buffer_head *bh = NULL;
	uint64_t i;
	
	printk(KERN_INFO "%s() start\n", __func__);

	sfs_sb = sb->s_fs_info;
	bh = sb_bread(sb, SIMPLEFS_INODESTORE_BLOCK_NUMBER);

	//find inode in inode table
	inodep = (struct simplefs_inode *)bh->b_data;
	for (i=0; i<sfs_sb->inodes_count; i++) {
		if (sfs_inode->inode_no == inodep->inode_no) {
			changed_inode = inodep;	
			break;
		}
		inodep++;
	}
	if (changed_inode) {
		memcpy(changed_inode, sfs_inode, sizeof(struct simplefs_inode));
		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
	} else {
		printk(KERN_ERR "not find inode in inode table\n");
		return -EIO;
	}
 
	brelse(bh);
	printk(KERN_INFO "%s() end\n", __func__);

	return 0;
}

static ssize_t simplefs_read(struct file * filp, char __user * buf, size_t len, loff_t * ppos)
{
	struct super_block *sb;
	struct inode *inode = NULL;
	struct simplefs_inode *sfs_inode = NULL;
	struct buffer_head *bh;
	int nbytes;
	char *buffer;
	ssize_t ret;

	printk(KERN_INFO "%s() start\n", __func__);

	inode = filp->f_path.dentry->d_inode;
	sfs_inode = inode->i_private;
	sb = inode->i_sb;

	printk(KERN_INFO "inode:%u blocknum:%u\n", 
				sfs_inode->inode_no, sfs_inode->data_block_number);

	if (*ppos >= sfs_inode->file_size) {
		printk(KERN_ERR "Read request with offset beyond the filesize\n");
		return 0;
	}

	bh = sb_bread(sb, sfs_inode->data_block_number);
	buffer = (char *)bh->b_data;

	nbytes = min((size_t) sfs_inode->file_size, len);
	printk(KERN_INFO "len: %d info:%s\n", nbytes, buffer);

	ret = copy_to_user(buf, buffer, nbytes);
	if (ret < 0) {
		printk(KERN_ERR "copy_to_user() fail\n");
		brelse(bh);
		return -EFAULT;
	}

	*ppos += nbytes; 
	printk("copy %d bytes\n", nbytes);

	brelse(bh);

	printk(KERN_INFO "%s() end\n", __func__);
	return nbytes;
}

ssize_t simplefs_write(struct file *filp, const char __user * buf, size_t len, loff_t *ppos)
{
	struct super_block *sb;
	struct inode *inode = NULL;
	struct simplefs_inode *sfs_inode = NULL;
	struct buffer_head *bh;
	char *buffer;
	ssize_t ret;

	printk(KERN_INFO "%s() start\n", __func__);

	inode = filp->f_path.dentry->d_inode;
	sfs_inode = inode->i_private;
	sb = inode->i_sb;

	printk(KERN_INFO "inode:%u blocknum:%u\n", 
				sfs_inode->inode_no, sfs_inode->data_block_number);
	printk(KERN_INFO "write len:%lu ppos:%lld\n", len, *ppos);

	bh = sb_bread(sb, sfs_inode->data_block_number);
	buffer = bh->b_data;
	
	buffer += *ppos;

	ret = copy_from_user(buffer, buf, len);
	if (ret < 0) {
		printk(KERN_ERR "Error copying file contents from the userspace buffer to the kernel space\n");
		brelse(bh);
		return -EFAULT;
	}
	*ppos += len;

	mark_buffer_dirty(bh); 
	sync_dirty_buffer(bh); 

	brelse(bh);

	if (mutex_lock_interruptible(&simplefs_inodes_mgmt_lock)) {
		printk(KERN_ERR "mutex lock fail\n");
		return -EINTR;
	}
	sfs_inode->file_size = *ppos;
	ret = simplefs_save_inode(sb, sfs_inode);
	if (ret < 0) {
		printk("save inode table error\n");
		return ret;
	}
	mutex_unlock(&simplefs_inodes_mgmt_lock);
	
	printk(KERN_INFO "%s() end\n", __func__);
	return len;
}

static ssize_t simplefs_read_iter(struct kiocb *k, struct iov_iter *iter)
{
	printk(KERN_INFO "%s()\n", __func__);
	return 0;
}

static ssize_t simplefs_write_iter(struct kiocb *k, struct iov_iter *iter)
{
	printk(KERN_INFO "%s()\n", __func__);
	return 0;
}

static int simplefs_iterate(struct file *filp, struct dir_context *ctx)
{
	loff_t pos;
	struct inode *inode;
	struct buffer_head *bh;
	struct super_block *sb;
	struct simplefs_inode *sfs_inode;
	struct simplefs_dir *dir;
	int i;

	printk(KERN_INFO "%s() start\n", __func__);

	pos = ctx->pos;
	inode = filp->f_path.dentry->d_inode; 
	printk(KERN_INFO "file inode:%lu\n", inode->i_ino);
	sb = inode->i_sb;

	if (pos) {
		return 0;
	}

	sfs_inode = inode->i_private; //simplefs_inode
	printk(KERN_INFO "inode_no:%u, data_block_number:%u",
				sfs_inode->inode_no, 
				sfs_inode->data_block_number);

	if (!S_ISDIR(sfs_inode->mode)) {
		printk(KERN_ERR "not a dir\n");
		return -ENOTDIR;
	}

	bh = sb_bread(sb, sfs_inode->data_block_number);
	dir = (struct simplefs_dir *)bh->b_data;
	printk(KERN_INFO "dir count;%u\n", sfs_inode->dir_children_count);
	//find dir inode and filldir
	for (i=0; i<sfs_inode->dir_children_count; i++) {
		//static inline bool dir_emit(struct dir_context *ctx, const char *name, 
		//			int namelen, u64 ino, unsigned type)
		// this function do fill dir
		dir_emit(ctx, dir->filename, SIMPLEFS_FILENAME_MAXLEN, 
					dir->inode_no, DT_UNKNOWN);
		ctx->pos += sizeof(struct simplefs_dir);
		printk(KERN_INFO "filename:%s inode:%llu\n", dir->filename, dir->inode_no);
		dir++;
	}

	brelse(bh);

	printk(KERN_INFO "%s() end\n", __func__);
	return 0;
}

const struct file_operations simplefs_file_operations = { 
	.read = simplefs_read,
	.write = simplefs_write,
};

static const struct file_operations simplefs_dir_operations = {
	.owner = THIS_MODULE,
	//ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	.read = simplefs_read,
	//ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	.write = simplefs_write,
	//ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	.read_iter = simplefs_read_iter,
	//ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	.write_iter = simplefs_write_iter,
	//int (*iterate) (struct file *, struct dir_context *);
	.iterate = simplefs_iterate,
};

static struct dentry *simplefs_lookup(struct inode *parent_inode,
				struct dentry *child_dentry, unsigned int flags);
static int simplefs_create(struct inode *i, struct dentry *d, umode_t mode, bool b);
static int simplefs_mkdir(struct inode *i, struct dentry *d, umode_t mode);
static int simplefs_link(struct dentry *d0, struct inode *i, struct dentry *d1);
static int simplefs_unlink(struct inode *i, struct dentry *dirent);

static struct inode_operations simplefs_inode_ops = {
	//struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
	.lookup = simplefs_lookup,
	//int (*create) (struct inode *,struct dentry *, umode_t, bool);
	.create = simplefs_create, 
	//int (*mkdir) (struct inode *,struct dentry *,umode_t);
	.mkdir = simplefs_mkdir,
	//int (*link) (struct dentry *,struct inode *,struct dentry *);
	.link = simplefs_link,
	//int (*unlink) (struct inode *,struct dentry *);
	.unlink = simplefs_unlink,
};

struct dentry *simplefs_lookup(struct inode *parent_inode,
				struct dentry *child_dentry, unsigned int flags)
{
	struct buffer_head *bh;
	struct super_block *sb;
	struct simplefs_inode *parent_i;
	struct simplefs_dir *dir;
	struct inode *inode = NULL;
	struct simplefs_inode *sfs_inode = NULL;
	int i;

	printk(KERN_INFO "%s() start\n", __func__);

	parent_i = parent_inode->i_private;
	sb = parent_inode->i_sb;
	bh = sb_bread(sb, parent_i->data_block_number);

	dir = (struct simplefs_dir *)bh->b_data;
	printk(KERN_INFO "parent_inode:%ld, child file name:%s\n", 
				parent_inode->i_ino, child_dentry->d_name.name);
	printk(KERN_INFO "dir->inode_no:%lld dir->filename:%s\n", dir->inode_no, dir->filename);
	printk(KERN_INFO "dir count:%d\n", parent_i->dir_children_count);
	for (i=0; i<parent_i->dir_children_count; i++) {
		if (!strcmp(dir->filename, child_dentry->d_name.name)) {
			printk(KERN_INFO "find file:%s\n", child_dentry->d_name.name);	
			
			//all inode memory for dir
			sfs_inode = simplefs_get_inode(sb, dir->inode_no);
			inode = new_inode(sb);
			inode->i_ino = dir->inode_no;
			inode_init_owner(inode, parent_inode, sfs_inode->mode);
			inode->i_sb = sb;
			inode->i_op = &simplefs_inode_ops;

			if (S_ISDIR(inode->i_mode)) {
				inode->i_fop = &simplefs_dir_operations;	
			} else if (S_ISREG(inode->i_mode)) {
				inode->i_fop = &simplefs_file_operations;
			} else {
				printk(KERN_ERR "err type\n");	
			}
			inode->i_atime = inode->i_mtime = inode->i_ctime = current_kernel_time();
			inode->i_private = sfs_inode;

			d_add(child_dentry, inode);
			break;
		}
		dir++;
	}
	
	brelse(bh);

	if (!inode) {
		printk(KERN_INFO "this dir %s not file\n", child_dentry->d_name.name);
	}

	printk(KERN_INFO "%s() end\n", __func__);
	return NULL;
}

static int simplefs_create_fs_object(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct super_block *sb;
	struct simplefs_super_block *sfs_sb;
	struct simplefs_inode *sfs_inode;
	struct simplefs_inode *parent_dir_inode;
	struct simplefs_dir *dir_data;
	struct buffer_head *bh;

	printk(KERN_INFO "%s() start\n", __func__);
	printk(KERN_INFO "create file name: %s\n", dentry->d_name.name);
	printk(KERN_INFO "paraent's dir inode number is %lu\n", dir->i_ino);

	sb = dir->i_sb;
	sfs_sb = sb->s_fs_info;

	if (sfs_sb->inodes_count >= SIMPLEFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED) {
		printk(KERN_ERR "file reach the maximum number\n");	
		return -ENOSPC;
	}
	if (!S_ISDIR(mode) && !S_ISREG(mode)) {
		printk(KERN_ERR "create file not a file or directory\n");
		return -EINVAL;	
	}



	printk(KERN_INFO "%s() end\n", __func__);
	return 0;
}

int simplefs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	printk(KERN_INFO "%s()\n", __func__);

	return simplefs_create_fs_object(dir, dentry, mode);
}

int simplefs_mkdir(struct inode *i, struct dentry *d, umode_t mode)
{
	printk(KERN_INFO "%s()\n", __func__);
	return 0;
}

int simplefs_link(struct dentry *d0, struct inode *i, struct dentry *d1)
{
	printk(KERN_INFO "%s()\n", __func__);
	return 0;
}

int simplefs_unlink(struct inode *i, struct dentry *dirent)
{
	printk(KERN_INFO "%s()\n", __func__);
	return 0;
}

void simplefs_destory_inode(struct inode *inode)
{  
	struct simplefs_inode *sfs_inode = inode->i_private;

	printk(KERN_INFO "%s(), Freeing private data of inode %p (%lu)\n",
				__func__, sfs_inode, inode->i_ino);      
	kmem_cache_free(sfs_inode_cachep, sfs_inode);
} 

static const struct super_operations simplefs_sops = {
	.destroy_inode = simplefs_destory_inode,
};

static int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct buffer_head *bh;
	struct simplefs_super_block *sb_disk;

	bh = (struct buffer_head *)sb_bread(sb, 0);	
	sb_disk = (struct simplefs_super_block *)bh->b_data;

	printk(KERN_INFO "The magic number obtained in disk is: [0x%llx]\n",sb_disk->magic);
	printk(KERN_INFO "version: %llu\n", sb_disk->version);
	printk(KERN_INFO "block size: %llu\n", sb_disk->block_size);
	printk(KERN_INFO "free block: %llu\n", sb_disk->free_blocks);

	sb->s_magic = SIMPLEFS_MAGIC;
	sb->s_fs_info = sb_disk;
	sb->s_maxbytes = SIMPLEFS_DEFAULT_BLOCK_SIZE;
	sb->s_op = &simplefs_sops;

	root_inode = new_inode(sb);
	root_inode->i_ino = SIMPLEFS_ROOTDIR_INODE_NUMBER;
	inode_init_owner(root_inode, NULL, S_IFDIR); //no parent inode
	root_inode->i_sb = sb;
	root_inode->i_op = &simplefs_inode_ops;
	root_inode->i_fop = &simplefs_dir_operations;
	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_kernel_time();

	root_inode->i_private = simplefs_get_inode(sb, SIMPLEFS_ROOTDIR_INODE_NUMBER);
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

	sfs_inode_cachep = kmem_cache_create("sfs_inode_cache", sizeof(struct simplefs_inode),
									0, (SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD), NULL);

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

	kmem_cache_destroy(sfs_inode_cachep);
}

module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
