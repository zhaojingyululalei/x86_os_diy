#ifndef __MOUSE_H
#define __MOUSE_H

#include "types.h"

#define MOUSE_PORT_DATA         0x60   // 鼠标数据端口
#define MOUSE_PORT_CMD          0x64   // 鼠标命令端口

#define MOUSE_CMD_ENABLE        0xF4   // 启用鼠标
#define MOUSE_CMD_DISABLE       0xF5   // 禁用鼠标
#define MOUSE_CMD_SET_SAMPLE_RATE 0xF3 // 设置采样率



// 鼠标按键和移动信息
#define MOUSE_BUTTON_LEFT       0x1    // 左键按下
#define MOUSE_BUTTON_RIGHT      0x2    // 右键按下
#define MOUSE_BUTTON_MIDDLE     0x4    // 中键按下

typedef struct {
    uint8_t buttons;       // 按键状态
    uint8_t x_movement;     // X轴移动
    uint8_t y_movement;     // Y轴移动
    uint8_t wheel_movement; //滚轮步数
} mouse_event_t;

void mouse_init(void);
#endif