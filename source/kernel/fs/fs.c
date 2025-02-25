#include "fs.h"
#include "console.h"
#include "dev.h"
#include "file.h"
#include "task/task.h"
#include "debug.h"
uint8_t tmp_app_buffer[512 * 1024];
uint8_t *tmp_pos;
int shell_fd = 9;

/**
 * 使用LBA48位模式读取磁盘
 */
static void read_disk(int sector, int sector_count, uint8_t *buf)
{
    outb(0x1F6, (uint8_t)(0xE0));

    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24)); // LBA参数的24~31位
    outb(0x1F4, (uint8_t)(0));            // LBA参数的32~39位
    outb(0x1F5, (uint8_t)(0));            // LBA参数的40~47位

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));       // LBA参数的0~7位
    outb(0x1F4, (uint8_t)(sector >> 8));  // LBA参数的8~15位
    outb(0x1F5, (uint8_t)(sector >> 16)); // LBA参数的16~23位

    outb(0x1F7, (uint8_t)0x24);

    // 读取数据
    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        // 每次扇区读之前都要检查，等待数据就绪
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }

        // 读取并将数据写入到缓存中
        for (int i = 0; i < DISK_SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
}
/**
 * @brief 判断文件描述符是否正确
 */
static int is_fd_bad (int file) {
	if ((file < 0) && (file >= TASK_OFILE_NR)) {
		return 1;
	}

	return 0;
}
int sys_open(const char *path, int flags, mode_t mode)
{
    int ret;

    ret = strncmp(path, "shell.elf", 9);
    // 路径解析正确
    if (ret == 0)
    {
        read_disk(10240, 512 * 1024 / 512, tmp_app_buffer);
        tmp_pos = tmp_app_buffer;
        return shell_fd;
    }

    ret = strncmp(path, "tty", 3);
    if (ret == 0)
    {
        int minor = -1, fd = -1;
        char cminor = path[strlen(path) - 1];
        if (cminor >= '0' && cminor <= '9')
        {
            minor = cminor - '0'; // 获取次设备号

            file_t *file = file_alloc();
            if (!file)
                return -1;
            file->dev_id = dev_open(DEV_TTY, minor, NULL);
            fd = task_alloc_fd(file);
            if (fd >= 0)
            {
                return fd;
            }
            else
            {
                return -1;
            }
        }
    }
    return 0;
}

int sys_read(int fd, char *buf, int len)
{
    if (fd == shell_fd)
    {
        memcpy(buf, tmp_pos, len);
        tmp_pos += len;
        return len;
    }
    else
    {
        file_t *p_file = task_file(fd);
        if (!p_file)
        {
            
            return -1;
        }
        return dev_read(p_file->dev_id,0,buf,len);
    }
    return 0;
}

int sys_write(int fd, const char *buf, int len)
{
    file_t *file = task_file(fd);
    if (!file)
    {
        return -1;
    }
    dev_write(file->dev_id, 0, buf, len);
    return 0;
}

int sys_lseek(int fd, int offset, int whence)
{
    if (fd == shell_fd)
    {
        tmp_pos = tmp_app_buffer + offset;
    }
    return 0;
}

int sys_close(int fd)
{
    file_t *file = task_file(fd);
    file_desc_ref(file);
    if(file->ref==0){
        //彻底关闭
        if(file->type == FILE_NORMAL || file->type == FILE_DIR){
            ;
        }else{
            dev_close(file->dev_id);
        }
    }
    return 0;
}
/**
 * 复制一个文件描述符
 */
int sys_dup (int fd) {
	// 超出进程所能打开的全部，退出
	if (is_fd_bad(fd)) {
        dbg_error("file(%d) is not valid.", fd);
		return -1;
	}

	file_t * p_file = task_file(fd);
	if (!p_file) {
		dbg_error("file not opened");
		return -1;
	}

	int new_fd = task_alloc_fd(p_file);	// 新fd指向同一描述符
	if (new_fd >= 0) {
		file_inc_ref(p_file);
		return fd;
	}

	dbg_error("No task file avaliable");
    return -1;
}
void fs_init(void)
{
    file_table_init();
}
