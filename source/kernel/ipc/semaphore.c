#include "semaphor.h"
#include "string.h"
void sys_sem_init(sem_t *sem, int init_count)
{
    memset(sem, 0, sizeof(sem_t));
    sem->count = init_count;
    list_init(&sem->wait_list);
}

void sys_sem_wait(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (sem->count > 0)
    {
        sem->count--;
    }
    else
    {
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t *curr = get_cur_task();
        curr->state = TASK_WAITING;
        list_insert_last(&sem->wait_list, &curr->node);
        set_cur_task(NULL);
        schedul();
    }

    irq_leave_protection(irq_state);
}
int sys_sem_trywait(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (sem->count > 0)
    {
        sem->count--;
        irq_leave_protection(irq_state);
        return 0;
    }
    else
    {
        irq_leave_protection(irq_state);
        return -1;
    }
}
#include "rtc.h"
list_t sem_timewait_list;
/*超时唤醒，timer中调用*/
void sem_time_check(void)
{
    time_t sec, sem_sec;
    tm_t time;
    //获取当前时间
    sys_get_clocktime(&time);
    sec = sys_mktime(&time);
    list_node_t *cur = sem_timewait_list.first;
    while (cur)
    {
        
        list_node_t *next = cur->next;
        sem_t *sem = list_node_parent(cur, sem_t, node);
        if(!sem->tmo)
        {
            continue;
        }
        sem_sec = sys_mktime(sem->tmo);
        //检测sem中的时间是否超时
        if (sec >= sem_sec)
        {
            //已经过了时间就唤醒
            list_remove(&sem_timewait_list, cur);
            sem->state = SEM_IN_LIST_NONE;
            sys_sem_notify(sem);
        }
        cur = next;
    }
}
void task_ipc_init(void)
{
    list_init(&sem_timewait_list);
}
/*传入超时截止时间*/
int sys_sem_timedwait(sem_t *sem, tm_t *tmo)
{
    sem->tmo = tmo;
    time_t dead_line;
    tm_t now_time;time_t now_sec;
    irq_state_t irq_state = irq_enter_protection();
    dead_line = sys_mktime(tmo);
    if (sem->count > 0)
    {
        sem->count--;
        return 0;
    }
    else
    {
        if(sem->state==SEM_IN_LIST_NONE)
        {
            list_insert_last(&sem_timewait_list, &sem->node);
            sem->state = SEM_IN_TMO_WAIT_LIST;
        }
        
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t *curr = get_cur_task();
        curr->state = TASK_WAITING;
        list_insert_last(&sem->wait_list, &curr->node);
        set_cur_task(NULL);
        schedul(); // 调度到其他线程去执行，该线程放等待队列去了
        //唤醒后检查时间，是否超时
        sys_get_clocktime(&now_time);
        now_sec = sys_mktime(&now_time);
        if (now_sec<dead_line)
        {
            // 这就是被其他线程唤醒的,没超时
            irq_leave_protection(irq_state);
            return 0;
        }
        else
        {
            // 这就是超时了，自动唤醒的
            irq_leave_protection(irq_state);
            return 1;
        }
    }

    irq_leave_protection(irq_state);
}
void sys_sem_notify(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (list_count(&sem->wait_list))
    {
        // 有进程等待，则唤醒加入就绪队列
        list_node_t *fnode = list_remove_first(&sem->wait_list);
        task_t *task = list_node_parent(fnode, task_t, node);
        set_task_to_ready_list(task);

        schedul();
    }
    else
    {
        sem->count++;
    }

    irq_leave_protection(irq_state);
}

int sys_sem_count(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();
    int count = sem->count;
    irq_leave_protection(irq_state);
    return count;
}
