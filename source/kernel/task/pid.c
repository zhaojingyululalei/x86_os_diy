#include "task.h"
#include "debug.h"

pidalloc_t pidallocter;
/**
 * 初始化 PID 分配器
 * @param alloc PID 分配器
 */
void pidalloc_init(pidalloc_t *alloc) {
    for (int i = 0; i < BITMAP_SIZE; i++) {
        alloc->bitmap[i] = 0;  // 将所有 PID 初始化为未分配
    }
}

/**
 * 检查指定 PID 是否已被分配
 * @param alloc PID 分配器
 * @param pid 待检查的 PID
 * @return true - 已分配，false - 未分配
 */
int pid_is_allocated(pidalloc_t *alloc, int pid) {
    if (pid <= 0 || pid > PID_MAX_NR) {
        return false;  // PID 无效
    }

    int byte_index = (pid - 1) / 8;
    int bit_index = (pid - 1) % 8;
    return (alloc->bitmap[byte_index] & (1 << bit_index)) != 0;
}

/**
 * 分配一个 PID
 * @param alloc PID 分配器
 * @return 分配的 PID，-1 表示没有可用的 PID
 */
int pid_alloc(pidalloc_t *alloc) {
    for (int i = 0; i < PID_MAX_NR; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;

        // 如果当前位未被占用
        if ((alloc->bitmap[byte_index] & (1 << bit_index)) == 0) {
            // 设置该位为 1，表示 PID 被占用
            alloc->bitmap[byte_index] |= (1 << bit_index);
            return i + 1;  // 返回分配的 PID（PID 从 1 开始）
        }
    }

    return -1;  // 如果没有空闲 PID，返回 -1
}

/**
 * 释放指定的 PID
 * @param alloc PID 分配器
 * @param pid 待释放的 PID
 */
void pid_free(pidalloc_t *alloc, int pid) {
    if (pid <= 0 || pid > PID_MAX_NR) {
        return;  // 无效的 PID
    }

    int byte_index = (pid - 1) / 8;
    int bit_index = (pid - 1) % 8;

    // 将该位设置为 0，表示 PID 被释放
    alloc->bitmap[byte_index] &= ~(1 << bit_index);
}

/**
 * 打印所有被占用的 PID
 * @param alloc PID 分配器
 */
void pidalloc_print(pidalloc_t *alloc) {
    dbg_info("Allocated PIDs: ");
    for (int i = 0; i < PID_MAX_NR; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if ((alloc->bitmap[byte_index] & (1 << bit_index)) != 0) {
            dbg_info("%d ", i + 1);
        }
    }
    dbg_info("\n");
}