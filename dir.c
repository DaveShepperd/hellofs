#include "khellofs.h"

int hellofs_readdir(struct file *dirp, struct dir_context *ctx)
{
	uint64_t i;
	loff_t pos;
	struct super_block *sb;
        struct hellofs_superblock *sbi;
	struct buffer_head *bh;
	struct hellofs_dir_record *dir_record;
	struct inode *inode;
	struct hellofs_inode *hellofs_inode;

	printk(KERN_INFO "hellofs_readdir(): dirp=%p, ctx=%p, ctx->pos=%lld, ctx->actor=%p\n",
               dirp, ctx, ctx?ctx->pos:0, ctx?ctx->actor:NULL);

	inode = file_inode(dirp);
	if ( !inode )
	{
		printk(KERN_ERR "hellofs_readdir(): Could not get inode from dirp\n");
		return -EINVAL;
	}
	hellofs_inode = HELLOFS_INODE(inode);
	if ( !hellofs_inode )
	{
		printk(KERN_ERR "hellofs_readdir(): Could not get hellofs_inode from file_inode(dirp)\n");
		return -EINVAL;
	}
	if ( !ctx )
	{
		printk(KERN_ERR "hellofs_readdir(): ctx is NULL. pos=%lld, inode=%p\n", pos, inode);
		return -EINVAL;
	}
	if ( !ctx->actor )
	{
		printk(KERN_ERR "hellofs_readdir(): ctx->actor is NULL. pos=%lld, ctx->pos=%lld\n", pos, ctx->pos);
		return -EINVAL;
	}
	sb = inode->i_sb;
        sbi = sb->s_fs_info;
        printk(KERN_INFO "hellofs_readdir(): sb=%p, sbi=%p, hellofs_inode->inode_no=%llu, pos=%lld, dir=%s\n",
                   sb, sbi, hellofs_inode->inode_no, pos, file_dentry(dirp)->d_name.name);

        pos = ctx->pos;
	if ( pos )
	{
		// TODO @Sankar: we use a hack of reading pos to figure if we have filled in data.
		return 0;
	}

	if ( unlikely(!S_ISDIR(hellofs_inode->mode)) )
	{
		printk(KERN_ERR
			   "Inode %llu of dentry %s is not a directory\n",
			   hellofs_inode->inode_no,
			   file_dentry(dirp)->d_name.name); /*dirp->f_dentry->d_name.name);*/
		return -ENOTDIR;
	}

        printk(KERN_INFO "hellofs_readdir(): outputting %lld filenames from %s\n", hellofs_inode->dir_children_count, file_dentry(dirp)->d_name.name);

         if ( !dir_emit_dots(dirp, ctx) )
            return 0;

	bh = sb_bread(sb, hellofs_inode->data_block_no);
	BUG_ON(!bh);
        dir_record = (struct hellofs_dir_record *)bh->b_data;

	for ( i = 0; i < hellofs_inode->dir_children_count; i++ )
	{
            int type = DT_REG;
            printk(KERN_INFO "hellofs_readdir():       %lld: %lld filename %s, type=%d\n", i, dir_record->inode_no, dir_record->filename, type);
            ++ctx->pos;
            if ( !dir_emit(ctx, dir_record->filename, HELLOFS_FILENAME_MAXLEN, dir_record->inode_no, type) )
                break;
            dir_record++;
	}
	printk(KERN_INFO "hellofs_readdir(): Finished with f_pos=%lld, misc=%d\n", ctx->pos, sbi->misc);
	brelse(bh);
        ++sbi->misc;
	return sbi->misc < 3 ? 0 : -EINVAL;
}
