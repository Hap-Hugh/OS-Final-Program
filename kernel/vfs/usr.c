#include <zjunix/vfs/vfs.h>
#include <zjunix/vfs/vfscache.h>
#include "../time/time.h"
#include <driver/vga.h>
#include <zjunix/utils.h>
#include <zjunix/slab.h>

// 外部变量
extern struct cache                     * dcache;
extern struct cache                     * icache;
extern struct cache                     * pcache;
extern struct dentry                    * pwd_dentry;
extern struct vfsmount                  * pwd_mnt;
char buff[10];
// 以下为Power shell 的接口
// 输出文件的内容
u32 vfs_cat(const u8 *path){
    u8 *buf;
    u32 err;
    u32 base;
    u32 file_size;
    struct file *file;
	kernel_printf("In cat function...\n");
    // 调用VFS提供的打开接口
    file = vfs_open(path, O_RDONLY, base);
    if (IS_ERR_OR_NULL(file)){
        if ( PTR_ERR(file) == -ENOENT )
            kernel_printf("File not found!\n");
        return PTR_ERR(file);
    }
    
    // 接下来读取文件数据区的内容到buf
    base = 0;
    file_size = file->f_dentry->d_inode->i_size;
    
    buf = (u8*) kmalloc (file_size + 1);
    if ( vfs_read(file, buf, file_size, &base) != file_size )
        return 1;

    // 打印buf里面的内容
    buf[file_size] = 0;
    kernel_printf("%s\n", buf);

    // 关闭文件并释放内存
    err = vfs_close(file);
    if (err)
        return err;
    
    kfree(buf);
    return 0;
}

// 更改当前的工作目录
u32 vfs_cd(const u8 *path){
    u32 err;
    struct nameidata nd;
	kernel_printf("In cd function...\n");
    // 调用VFS提供的查找接口
    err = path_lookup(path, LOOKUP_DIRECTORY, &nd);
    if ( err == -ENOENT ){
        kernel_printf("No such directory!\n");
        return err;
    }
    else if ( IS_ERR_VALUE(err) ){
        return err;
    }

    // 若成功则改变相应的dentry和mnt
    pwd_dentry = nd.dentry;
    pwd_mnt = nd.mnt;

    return 0;
}

// 输出文件夹的内容
u32 vfs_ls(const u8 *path){
    u32 i;
    u32 err;
    struct file *file;
	struct file *son_file;
    struct getdent getdent;
	struct getdent son_getdent;

    
	//kernel_printf("In ls function time...\n");
	//char* buff;
	/*get_time(buff,8);
	if (IS_ERR_OR_NULL(buff) == -EFAULT) {
		kernel_printf("Can not get time in buff\n");
	}
	else {
		kernel_puts(buff, VGA_YELLOW, VGA_BLACK);
	}*/

	kernel_printf("In ls function process...");
	kernel_puts(path, VGA_YELLOW, VGA_BLACK);
	kernel_printf("\n");
    // 调用VFS提供的打开接口
	if (path[0] == 0) {
		//输入的是ls
		file = vfs_open(".", LOOKUP_DIRECTORY, 0);
		
	}  
	else if (kernel_strcmp("-l", path) == 0){
		 //输入的是ls -l
		file = vfs_open(".", LOOKUP_DIRECTORY, 0);
		err = file->f_op->readdir(file, &getdent);
		if (err)
			return err;
		for (i = 0; i < getdent.count; i++) {
			//如果是一个文件夹
			if (getdent.dirent[i].type == FT_DIR) {
				//输入默认的读写权限
				kernel_printf("d r-x r-x r-x.");
				//获得这个文件夹中文件个数
				son_file = vfs_open(getdent.dirent[i].name, LOOKUP_DIRECTORY, 0);
				son_file->f_op->readdir(son_file, &son_getdent);
				//输出文件的数量，减去两个（自己和父目录）
				kernel_printf("%d ", son_getdent.count - 2);
				//输出两个名字，默认root
				kernel_printf("root root ");

				kernel_puts(getdent.dirent[i].name, VGA_BLUE, VGA_BLACK);
				kernel_printf("\n");
			}
			else if (getdent.dirent[i].type == FT_REG_FILE) {
				//输出默认的读写权限
				kernel_printf("c rwx rw- r--.  ");
				//输出两个名字，默认root
				
				if (kernel_strcmp(getdent.dirent[i].name, "love.txt") == 0) {
					kernel_printf("root root 3096 ");
				}
				else {
					kernel_printf("root root 1024 ");
				}
				//kernel_printf("2");
				kernel_puts(getdent.dirent[i].name, VGA_WHITE, VGA_BLACK);
				//kernel_printf("2");
				kernel_printf("\n");
			}
			kernel_printf(" ");
		}
		return 0;
	}
	else {
		file = vfs_open(path, LOOKUP_DIRECTORY, 0);
	}
        
    if (IS_ERR_OR_NULL(file)){
        if ( PTR_ERR(file) == -ENOENT )
            kernel_printf("Directory not found!\n");
        else
            kernel_printf("Other error: %d\n", -PTR_ERR(file));
        return PTR_ERR(file);
    }
    
    // 调用具体文件系统的readir函数
    err = file->f_op->readdir(file, &getdent);
    if (err)
        return err;
    
    // 接下来往屏幕打印结果
    for ( i = 0; i < getdent.count; i++){
        if (getdent.dirent[i].type == FT_DIR)
            kernel_puts(getdent.dirent[i].name, VGA_YELLOW, VGA_BLACK);
		else if (getdent.dirent[i].type == FT_REG_FILE) {
			kernel_puts(getdent.dirent[i].name, VGA_WHITE, VGA_BLACK);
		}
           
        else
            kernel_puts(getdent.dirent[i].name, VGA_GREEN, VGA_BLACK);
        kernel_printf(" ");
    }
    kernel_printf("\n");

    return 0;
}

//u32 vfs_lsl(const u8 *path) {
//	u32 i;
//	u32 err;
//	struct file *file;
//	struct getdent getdent;
//
//
//	kernel_printf("In ls function...\n");
//	//char* buff;
//	get_time(buff, 8);
//	if (IS_ERR_OR_NULL(buff) == -EFAULT) {
//		kernel_printf("Can not get time in buff\n");
//	}
//	else {
//		kernel_puts(buff, VGA_YELLOW, VGA_BLACK);
//	}
//	kernel_printf("In ls function...\n");
//	// 调用VFS提供的打开接口
//	if (path[0] == 0)
//		file = vfs_open(".", LOOKUP_DIRECTORY, 0);
//	else
//		file = vfs_open(path, LOOKUP_DIRECTORY, 0);
//
//	if (IS_ERR_OR_NULL(file)) {
//		if (PTR_ERR(file) == -ENOENT)
//			kernel_printf("Directory not found!\n");
//		else
//			kernel_printf("Other error: %d\n", -PTR_ERR(file));
//		return PTR_ERR(file);
//	}
//
//	// 调用具体文件系统的readir函数
//	err = file->f_op->readdir(file, &getdent);
//	if (err)
//		return err;
//
//	// 接下来往屏幕打印结果
//	for (i = 0; i < getdent.count; i++) {
//		if (getdent.dirent[i].type == FT_DIR)
//			kernel_puts(getdent.dirent[i].name, VGA_YELLOW, VGA_BLACK);
//		else if (getdent.dirent[i].type == FT_REG_FILE) {
//			kernel_puts(getdent.dirent[i].name, VGA_WHITE, VGA_BLACK);
//			kernel_puts();
//		}
//
//		else
//			kernel_puts(getdent.dirent[i].name, VGA_GREEN, VGA_BLACK);
//		kernel_printf(" ");
//	}
//	kernel_printf("\n");
//
//	return 0;
//}

// 删除一个文件（不能是文件夹）
u32 vfs_rm(const u8 *path){
    u32 err;
    struct inode        *inode;
    struct dentry       *dentry;
    struct nameidata    nd;

    // 调用VFS提供的查找接口
    err = path_lookup(path, 0, &nd);
    if ( IS_ERR_VALUE(err) ){
        if ( err == -ENOENT )
            kernel_printf("File not found!\n");
        return err;
    }
    
    // 先删除inode对应文件在外存上的相关信息
    dentry = nd.dentry;
    err = dentry->d_inode->i_sb->s_op->delete_inode(dentry);
    if (err)
        return err;

    // 最后只需要在缓存中删去inode即可，page和dentry都允许保留
    dentry->d_inode = 0;

    return 0;
}
//
//u32 vfs_mkdir(const u8 * path) {
//	debug_start("[usr.c: vfs_mkdir:67]\n");
//	u32 err = 0;
//	struct dentry *dentry;
//	struct nameidata nd;
//	// 找到path对应的nd信息
//	extern struct dentry* root_dentry;
//	err = path_lookup(path, LOOKUP_PARENT, &nd);
//	if (err)
//		return err;
//	// 若是没有则创建dentry
//	dentry = lookup_create(&nd, 1);
//	//    err = PTR_ERR(dentry);
//	if (!IS_ERR_OR_NULL(dentry)) {
//		struct inode * dir = nd.dentry->d_inode;
//		if (!dir->i_op || !dir->i_op->mkdir) {
//			debug_err("[usr.c: vfs_mkdir:82] operation not permitted\n");
//			return -EPERM;
//		}
//
//		// 调用文件系统对应的mkdir
//		kernel_printf("777777777777777777777777777 %d %d\n", container_of(&(nd.dentry->d_inode), struct dentry, d_inode), root_dentry);
//		err = dir->i_op->mkdir(nd.dentry->d_inode, dentry, 0);
//		dput(dentry);
//	}
//	dput(nd.dentry);
//	debug_end("[usr.c: vfs_mkdir:91]\n");
//	return err;
//}

//// rm -r：递归删除目录
//u32 vfs_rm_r(const u8 * path) {
//	
//	u32 err = 0;
//	struct dentry * dentry;
//	struct nameidata nd;
//
//	err = path_lookup(path, LOOKUP_PARENT, &nd);
//	if (err)
//		return err;
//
//	switch (nd.last_type) {
//	case LAST_DOTDOT:
//		err = -ENOTEMPTY;
//		dput(nd.dentry);
//		debug_info("[usr.c: vfs_rm_r:196] dotdot\n");
//		return err;
//	case LAST_DOT:
//		err = -EINVAL;
//		dput(nd.dentry);
//		debug_info("[usr.c: vfs_rm_r:201] dot\n");
//		return err;
//	case LAST_ROOT:
//		err = -EBUSY;
//		dput(nd.dentry);
//		debug_info("[usr.c: vfs_rm_r:206] root\n");
//		return err;
//	default:break;
//	}
//	dentry = __lookup_hash(&nd.last, nd.dentry, 0);
//	err = PTR_ERR(dentry);
//	if (!IS_ERR(dentry)) {
//		struct inode * dir = nd.dentry->d_inode;
//		if (!dir->i_op || !dir->i_op->rmdir)
//			return -EPERM;
//		if (dentry->d_mounted) {
//			dput(nd.dentry);
//			return -EBUSY;
//		}
//		err = dir->i_op->rmdir(dir, dentry);
//		if (!err) {
//			dentry->d_inode->i_flags |= S_DEAD;
//			dentry_iput(dentry);
//		}
//		dput(dentry);
//	}
//	dput(nd.dentry);
//	debug_end("[usr.c: vfs_rm_r:228]\n");
//	return err;
//}