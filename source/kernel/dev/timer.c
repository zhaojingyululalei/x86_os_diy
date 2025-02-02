#include "timer.h"
#include "types.h"
#include "irq/irq.h"
#include "cpu_instr.h"
#include "task/task.h"
#include "ipc/semaphor.h"
#include "rtc.h"
static uint32_t sys_tick=0;

/**
 * 定时器中断处理函数
 */
void do_handler_timer (exception_frame_t *frame) {
    sys_tick+=10;
    // 发送EOI
    pic_send_eoi(IRQ0_TIMER);
    sem_time_check();

    task_time_tick();
   
}
uint32_t get_cur_time_ms(void)
{
    return sys_tick;
}
void timer_init(void)
{
    sys_tick = 0;
    uint32_t reload_count = PIT_OSC_FREQ / (1000.0 / OS_TICK_MS);

    // outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE0);
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);        // 加载低8位
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF); // 再加载高8位

    interupt_install(IRQ0_TIMER, (irq_handler_t)exception_handler_timer);
    irq_enable(IRQ0_TIMER);
}