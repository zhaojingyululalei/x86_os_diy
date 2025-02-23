#include "mouse.h"
#include "irq/irq.h"
#include "cpu_instr.h"
#include "debug.h"
extern void exception_handler_mouse(void);


/**
 * https://wiki.osdev.org/Mouse_Input#USB_Mouse
 * */
void mouse_init(void) {
    // 禁用 PS/2 端口
    outb(0x64, 0xAD); // 禁用键盘端口
    outb(0x64, 0xA7); // 禁用鼠标端口

    // 清空输出缓冲区
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    // 启用鼠标端口
    outb(0x64, 0xA8); // 启用鼠标端口
    outb(0x64, 0x20); // 读取控制器配置字节
    uint8_t config = inb(0x60); // 获取当前配置
    config |= 0x02; // 启用鼠标中断
    config &= ~0x20; // 禁用键盘中断
    outb(0x64, 0x60); // 写入新的配置字节
    outb(0x60, config);

    // 启用键盘端口
    outb(0x64, 0xAE);

    // 发送启用鼠标命令
    outb(0x64, 0xD4); // 告诉控制器下一个字节是发送给鼠标的
    outb(0x60, 0xF4); // 启用鼠标

    // 等待鼠标响应
    while (!(inb(0x64) & 0x01)); // 等待数据可用
    uint8_t response = inb(0x60);
    if (response != 0xFA) {
        dbg_error("Mouse failed to enable\r\n");
        return;
    }

    // 启用滚轮功能
    outb(0x64, 0xD4); // 告诉控制器下一个字节是发送给鼠标的
    outb(0x60, 0xF2); // 发送“读取设备ID”命令
    while (!(inb(0x64) & 0x01)); // 等待数据可用
    response = inb(0x60); // 读取设备ID
    if (response != 0xFA) {
        dbg_error("Failed to read mouse device ID\r\n");
        return;
    }

    // 如果设备ID是0x03，表示鼠标支持滚轮
    if (response == 0x03) {
        dbg_info("Mouse supports scroll wheel\r\n");

        // 设置采样率为200（启用滚轮模式）
        outb(0x64, 0xD4); // 告诉控制器下一个字节是发送给鼠标的
        outb(0x60, 0xE8); // 发送“设置采样率”命令
        while (!(inb(0x64) & 0x01)); // 等待数据可用
        response = inb(0x60); // 读取响应
        if (response != 0xFA) {
            dbg_error("Failed to set mouse sample rate\r\n");
            return;
        }

        outb(0x64, 0xD4); // 告诉控制器下一个字节是发送给鼠标的
        outb(0x60, 0x0A); // 发送采样率200
        while (!(inb(0x64) & 0x01)); // 等待数据可用
        response = inb(0x60); // 读取响应
        if (response != 0xFA) {
            dbg_error("Failed to set mouse sample rate to 200\r\n");
            return;
        }
    } else {
        dbg_info("Mouse does not support scroll wheel\r\n");
    }

    // 安装鼠标中断处理程序
    interupt_install(IRQ12_MOUSE, exception_handler_mouse);
    irq_enable(IRQ12_MOUSE);

    dbg_info("Mouse initialized successfully\r\n");
}
// void mouse_init(void) {
//     // 禁用 PS/2 端口
//     outb(0x64, 0xAD); // 禁用键盘端口
//     outb(0x64, 0xA7); // 禁用鼠标端口

//     // 清空输出缓冲区
//     while (inb(0x64) & 0x01) {
//         inb(0x60);
//     }

//     // 启用鼠标端口
//     outb(0x64, 0xA8); // 启用鼠标端口
//     outb(0x64, 0x20); // 读取控制器配置字节
//     uint8_t config = inb(0x60); // 获取当前配置
//     config |= 0x02; // 启用鼠标中断
//     config &= ~0x20; // 禁用键盘中断
//     outb(0x64, 0x60); // 写入新的配置字节
//     outb(0x60, config);

//     // 启用键盘端口
//     outb(0x64, 0xAE);

//     // 发送启用鼠标命令
//     outb(0x64, 0xD4); // 告诉控制器下一个字节是发送给鼠标的
//     outb(0x60, 0xF4); // 启用鼠标

//     // 等待鼠标响应
//     while (!(inb(0x64) & 0x01)); // 等待数据可用
//     uint8_t response = inb(0x60);
//     if (response != 0xFA) {
//         dbg_error("Mouse failed to enable\r\n");
//         return;
//     }

//     // 安装鼠标中断处理程序
//     interupt_install(IRQ12_MOUSE, exception_handler_mouse);
//     irq_enable(IRQ12_MOUSE);

//     dbg_info("Mouse initialized successfully\r\n");
// }

void do_handler_mouse(exception_frame_t *frame)
{
    // 检查鼠标数据端口是否准备好
    uint8_t status = inb(MOUSE_PORT_CMD);
    if (!(status & 0x01))
    { // 如果没有数据，直接返回
        pic_send_eoi(IRQ12_MOUSE);
        return;
    }

    // 读取鼠标数据
    uint8_t data1 = inb(MOUSE_PORT_DATA);
    uint8_t data2 = inb(MOUSE_PORT_DATA);
    uint8_t data3 = inb(MOUSE_PORT_DATA);


    // 鼠标数据格式：按键 + X 轴变化 + Y 轴变化 + 滚轮步数
    mouse_event_t mouse_event = {
        .buttons = data1 & 0x07,        // 按键状态
        .x_movement = data2,    // X轴移动
        .y_movement = data3,    // Y轴移动
    };

    dbg_info("recv a mouse**********************\r\n");
    dbg_info("data1:%d\r\n",data1);
    dbg_info("data2:%d\r\n",data2);
    dbg_info("data3:%d\r\n",data3);
    dbg_info("\r\n");
    
    /*滚轮上下分辨不出来*/
    /*左键向上翻一行，右键向下翻一行*/
    // 完成后发送 EOI 信号
    pic_send_eoi(IRQ12_MOUSE);
}