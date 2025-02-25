

#include "tty.h"

#include "dev/console.h"
#include "dev/kbd.h"
#include "dev/dev.h"
#include "debug.h"
#include "cpu.h"

static tty_t tty_devices[TTY_NR];
static int active_tty = 0;

/**
 * @brief 初始化FIFO缓冲区
 */
void init_fifo(tty_fifo_t * fifo, char * buffer, int buffer_size) {
    fifo->buf = buffer;
    fifo->count = 0;
    fifo->size = buffer_size;
    fifo->read = fifo->write = 0;
}

/**
 * @brief 从FIFO缓冲区读取一个字节
 */
int get_char_from_fifo(tty_fifo_t * fifo, char * character) {
    if (fifo->count <= 0) {
        return -1;  // 没有数据可读
    }

    irq_state_t irq_state = irq_enter_protection();
    *character = fifo->buf[fifo->read++];
    if (fifo->read >= fifo->size) {
        fifo->read = 0;
    }
    fifo->count--;
    irq_leave_protection(irq_state);
    return 0;  // 成功读取
}

/**
 * @brief 向FIFO缓冲区写入一个字节
 */
int put_char_to_fifo(tty_fifo_t * fifo, char c) {
    if (fifo->count >= fifo->size) {
        return -1;  // FIFO已满
    }

    irq_state_t state = irq_enter_protection();
    fifo->buf[fifo->write++] = c;
    if (fifo->write >= fifo->size) {
        fifo->write = 0;
    }
    fifo->count++;
    irq_leave_protection(state);

    return 0;  // 成功写入
}

/**
 * @brief 判断tty设备是否有效
 */
static inline tty_t * get_tty (device_t * dev) {
    int tty = dev->minor;
    if ((tty < 0) || (tty >= TTY_NR) || (!dev->open_count)) {
        dbg_info("tty is not opened. tty = %d", tty);
        return (tty_t *)0;
    }

    return tty_devices + tty;
}

/**
 * @brief 打开tty设备
 */
int open_tty_device(device_t * device) {
    int tty_id = device->minor;
    if ((tty_id < 0) || (tty_id >= TTY_NR)) {
        dbg_info("Failed to open tty device. Invalid tty id = %d", tty_id);
        return -1;
    }

    tty_t * tty = tty_devices + tty_id;
    init_fifo(&tty->output_fifo, tty->output_buffer, TTY_OBUF_SIZE);
    sys_sem_init(&tty->output_semaphore, TTY_OBUF_SIZE);
    init_fifo(&tty->input_fifo, tty->input_buffer, TTY_IBUF_SIZE);
    sys_sem_init(&tty->input_semaphore, 0);

    tty->input_flags = TTY_INLCR | TTY_IECHO;
    tty->output_flags = TTY_OCRLF;

    tty->console_index = tty_id;

    console_init(tty_id);
    return 0;
}

/**
 * @brief 向tty设备写入数据
 */
int write_to_tty(device_t * dev, int addr, char * buffer, int length) {
    if (length < 0) {
        return -1;
    }

    tty_t * tty = get_tty(dev);
    int written = 0;

    while (length) {
        char current_char = *buffer++;

        // 如果遇到\n，根据配置决定是否转换成\r\n
        if (current_char == '\n' && (tty->output_flags & TTY_OCRLF)) {
            sys_sem_wait(&tty->output_semaphore);
            int result = put_char_to_fifo(&tty->output_fifo, '\r');
            if (result < 0) {
                break;
            }
        }

        // 写入当前字符
        sys_sem_wait(&tty->output_semaphore);
        int result = put_char_to_fifo(&tty->output_fifo, current_char);
        if (result < 0) {
            break;
        }

        written++;
        length--;

        // 启动输出, 这里是直接由console直接输出，无需中断
        console_write(tty);
    }

    return written;
}


/**
 * @brief 从tty设备读取数据
 */
int read_from_tty(device_t * dev, int addr, char * buffer, int size) {
    if (size < 0) {
        return -1;
    }

    tty_t * tty = get_tty(dev);
    char * pbuf = buffer;
    int len = 0;

    while (len < size) {
        sys_sem_wait(&tty->input_semaphore);
        char character;
        get_char_from_fifo(&tty->input_fifo, &character);

        switch (character) {
            case ASCII_DEL:
                if (len == 0) {
                    continue;
                }
                len--;
                pbuf--;
                break;
            case '\n':
                if ((tty->input_flags & TTY_INLCR) && (len < size - 1)) { // \n变成\r\n
                    *pbuf++ = '\r';
                    len++;
                }
                *pbuf++ = '\n';
                len++;
                break;
            default:
                *pbuf++ = character;
                len++;
                break;
        }

        // 回显字符，如果启用了回显标志
        if (tty->input_flags & TTY_IECHO) {
            write_to_tty(dev, 0, &character, 1);
        }

        // 遇到一行结束也直接跳出
        if ((character == '\r') || (character == '\n')) {
            break;
        }
    }

    return len;
}


/**
 * @brief 向tty设备发送命令
 */
int tty_control (device_t * dev, int cmd, int arg0, int arg1) {
    tty_t * tty = get_tty(dev);

    switch (cmd) {
    case TTY_CMD_ECHO:
        if (arg0) {
            tty->input_flags |= TTY_IECHO;
            console_set_cursor(tty->console_index, 1);
        } else {
            tty->input_flags &= ~TTY_IECHO;
            console_set_cursor(tty->console_index, 0);
        }
        break;
    case TTY_CMD_IN_COUNT:
        if (arg0) {
            *(int *)arg0 = sem_count(&tty->input_semaphore);
        }
        break;
    default:
        break;
    }
    return 0;
}

/**
 * @brief 关闭tty设备
 */
void tty_close (device_t * dev) {

}

/**
 * @brief 输入tty字符
 */
void tty_in (char ch) {
    tty_t * tty = tty_devices + active_tty;

    // 辅助队列要有空闲空间可代写入
    if (sys_sem_count(&tty->input_semaphore) >= TTY_IBUF_SIZE) {
        return;
    }

    // 写入辅助队列，通知数据到达
    put_char_to_fifo(&tty->input_fifo, ch);
    sys_sem_notify(&tty->input_semaphore);
}

/**
 * @brief 选择tty
 */
void tty_select (int tty) {
    if (tty != active_tty) {
        console_select(tty);
        active_tty = tty;
    }
}

// 设备描述表: 描述一个设备所具备的特性
dev_desc_t dev_tty_desc = {
    .name = "tty",
    .major = DEV_TTY,
    .open = open_tty_device,
    .read = read_from_tty,
    .write = write_to_tty,
    .control = tty_control,
    .close = tty_close,
};
