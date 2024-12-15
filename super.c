#include "khellofs.h"

#define HELLOFS_MNT_OPT_RO 1
#define HELLOFS_MNT_OPT_VERBOSE 2

static const match_table_t tokens = {
    {HELLOFS_MNT_OPT_RO, "ro"},
	{HELLOFS_MNT_OPT_VERBOSE, "verbose"},
	{0,NULL}
};

static int hellofs_parse_options(struct super_block *sb, char *options)
{
    substring_t args[MAX_OPT_ARGS];
    int token;
    char *p;

    pr_info("hellofs_parse_options: parsing options '%s'\n", options);

    while ((p = strsep(&options, ","))) {
        if (!*p)
            continue;

        args[0].to = args[0].from = NULL;
        token = match_token(p, tokens, args);
		if ( token == HELLOFS_MNT_OPT_RO )
		{
			pr_info("hellofs_parse_options: found read-only. Current flags: 0x%lX\n", sb->s_flags);
			sb->s_flags |= SB_RDONLY;
		}
		else if ( token == HELLOFS_MNT_OPT_VERBOSE )
		{
			struct hellofs_superblock *sbi = (struct hellofs_superblock *)sb->s_fs_info;
			pr_info("hellofs_parse_options: found verbose. Current flags: 0x%X\n", sbi->flags);
			sbi->flags |= HELLOFS_SB_FLAG_VERBOSE;
		}
	}
	return 0;
}

static int hellofs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode *root_inode;
    struct hellofs_inode *root_hellofs_inode;
    struct buffer_head *bh;
    struct hellofs_superblock *hellofs_sb;
    int ret = 0;

    bh = sb_bread(sb, HELLOFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);
    hellofs_sb = (struct hellofs_superblock *)bh->b_data;
    if (unlikely(hellofs_sb->magic != HELLOFS_MAGIC)) {
        printk(KERN_ERR
               "The filesystem being mounted is not of type hellofs. "
               "Magic number mismatch: %llu != %llu\n",
               hellofs_sb->magic, (uint64_t)HELLOFS_MAGIC);
        goto release;
    }
    if (unlikely(sb->s_blocksize != hellofs_sb->blocksize)) {
        printk(KERN_ERR
               "hellofs seem to be formatted with mismatching blocksize: %lu\n",
               sb->s_blocksize);
        goto release;
    }

    sb->s_magic = hellofs_sb->magic;
    sb->s_fs_info = hellofs_sb;
    sb->s_maxbytes = hellofs_sb->blocksize;
    sb->s_op = &hellofs_sb_ops;

    root_hellofs_inode = hellofs_get_hellofs_inode(sb, HELLOFS_ROOTDIR_INODE_NO);
    root_inode = new_inode(sb);
    if (!root_inode || !root_hellofs_inode) {
        ret = -ENOMEM;
        goto release;
    }
    hellofs_fill_inode(sb, root_inode, root_hellofs_inode);
    inode_init_owner(&nop_mnt_idmap, root_inode, NULL, root_inode->i_mode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto release;
    }

    ret = hellofs_parse_options(sb, data);
    if (ret) {
        pr_err("hellofs_fill_super: Failed to parse options, error code: %d\n",
               ret);
    }
	if ( (hellofs_sb->flags & HELLOFS_SB_FLAG_VERBOSE) )
	{
		pr_info("hellofs_fill_super: Finished. sb->flags=0x%lX, sbi->flags=0x%X\n", sb->s_flags, hellofs_sb->flags);
	}
release:
    brelse(bh);
    return ret;
}

struct dentry *hellofs_mount(struct file_system_type *fs_type,
                             int flags, const char *dev_name,
                             void *data) {
    struct dentry *ret;
    ret = mount_bdev(fs_type, flags, dev_name, data, hellofs_fill_super);

    if (unlikely(IS_ERR(ret))) {
        printk(KERN_ERR "Error mounting hellofs.\n");
    } else {
        printk(KERN_INFO "hellofs is succesfully mounted on: %s\n",
               dev_name);
    }

    return ret;
}

void hellofs_kill_superblock(struct super_block *sb) {
    printk(KERN_INFO
           "hellofs superblock is destroyed. Unmount succesful.\n");
    kill_block_super(sb);
}

void hellofs_put_super(struct super_block *sb) {
    return;
}

void hellofs_save_sb(struct super_block *sb) {
    struct buffer_head *bh;
    struct hellofs_superblock *hellofs_sb = HELLOFS_SB(sb);

    bh = sb_bread(sb, HELLOFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);

    bh->b_data = (char *)hellofs_sb;
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}

int hellofs_statfs(struct dentry *dirp, struct kstatfs *statp)
{
    struct super_block *sb = dirp->d_sb;
    struct hellofs_superblock *sbi = HELLOFS_SB(sb);

    statp->f_type = HELLOFS_MAGIC;
    statp->f_bsize = HELLOFS_DEFAULT_BLOCKSIZE;
    statp->f_blocks = sbi->fs_size/HELLOFS_DEFAULT_BLOCKSIZE;
	statp->f_bfree = statp->f_blocks - sbi->data_block_count - sbi->inode_count;
    statp->f_bavail = sbi->data_block_table_size-sbi->data_block_count;
    statp->f_files = sbi->inode_table_size;
	statp->f_ffree = sbi->inode_table_size - sbi->inode_count;
    statp->f_namelen = HELLOFS_FILENAME_MAXLEN;

    return 0;
}

