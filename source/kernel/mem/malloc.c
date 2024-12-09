#include "malloc.h"
#include "debug.h"
#include "math.h"
#include "mmu/mmu.h"
#include "mm_block.h"
static free_block_t fblock[MAX_FREE_BLOCK_SIZE];
static flmallocor_t g_flmlk;
void fmlk_debug_block(free_block_t *block)
{
    dbg_info("++++++++++++++++++\r\n");
    // 打印 free_block_t 结构体中的所有信息
    dbg_info("Block Start Address: %x\r\n", (void *)block->start);
    dbg_info("Block Capcity: %x \r\n", block->capcity);
    dbg_info("Block remain size:%x \r\n", block->remain_size);
    dbg_info("Buddy Block Address: %x\r\n", (void *)block->buddy);
    dbg_info("Parent Block Address: %x\r\n", (void *)block->parent);
    dbg_info("Block current in %d list\r\n", block->list_idx);
    dbg_info("++++++++++++++++++\r\n");
}
void fmlk_debug_list(int idx,flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
    list_t *list = &flmlk->free_list[idx];
    list_node_t *cur = list_first(list);
    int count = 0;
    while (cur)
    {
        free_block_t *block = list_node_parent(cur, free_block_t, node);
        dbg_info("cur block is:\r\n");
        fmlk_debug_block(block);
        if(block->buddy)
        {
            dbg_info("has buddy is:\r\n");
            fmlk_debug_block(block->buddy);
        }
        if(block->parent)
        {
            dbg_info("has parent is:\r\n");
            fmlk_debug_block(block->parent);
        }
        count++;
        cur = cur->next;
    }
    dbg_info("list total has %d node\r\n", count);
    
}
void fmlk_debug_alllist(flmallocor_t* f)
{
    dbg_info("          the whole list\r\n");
    for (int i = 0; i < HEAP_FREE_LIST_NR; i++)
    {
        dbg_info("the %d list:*\r\n", i);
        fmlk_debug_list(i,f);
        dbg_info("------------------------------\r\n");
    }
}
free_block_t *mlk_alloc_block(void)
{
    int i;
    for (i = 0; i < MAX_FREE_BLOCK_SIZE; ++i)
    {
        if (fblock[i].state == F_BLOCK_TYPE_NONE)
        {
            fblock[i].list_idx = -1;
            fblock[i].state = F_BLOCK_TYPE_CREATE;
            return &fblock[i];
        }
    }
    if (i == MAX_FREE_BLOCK_SIZE)
    {
        // 没找到
        return NULL;
    }
    return NULL;
}
void mlk_free_block(free_block_t *block)
{
    memset(block, 0, sizeof(free_block_t));
    block->list_idx = -1;
}

int mlk_put_block_into_list(free_block_t *block, int idx,flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
    list_t *list = &flmlk->free_list[idx];
    if (block->list_idx == -1)
    {
        block->list_idx = idx;
        block->remain_size = block->capcity = (uint32_t)power(2, idx);
    }
    block->state = F_BLOCK_TYPE_EFFCTIVE;
    list_node_t *cnode = &block->node;
    list_insert_last(list, cnode);
    return 0;
}
int mlk_remove_block_from_list(free_block_t *block, int idx,flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
    block->state = F_BLOCK_TYPE_CREATE;
    list_t* list = &flmlk->free_list[idx];
    list_node_t* cnode = &block->node;
    list_remove(list,cnode);
    return 0;
}
free_block_t *mlk_get_first_block_from_list(int idx,flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
    list_t* list = &flmlk->free_list[idx];
    free_block_t* ret = list_node_parent(list->first,free_block_t,node);
    if(ret)
    {
        return ret;
    }else{
        return NULL;
    }
}
#include "task/task.h"
void sbrk(uint32_t size,flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
  
    task_t *cur_task = get_cur_task();
    ph_addr_t old_base = cur_task->heap_e;
    size = align_up_to(size, MEM_PAGE_SIZE);
    int page_count = size / MEM_PAGE_SIZE;
    mmblock(cur_task, cur_task->heap_e, page_count);
    cur_task->heap_e += size;

    for (int i = 0; i < page_count; ++i)
    {

        free_block_t *block = mlk_alloc_block();
        if (block == NULL)
        {
            dbg_error("free_block_t struct not enough\r\n");

            return;
        }
        block->buddy = NULL;
        block->parent = NULL;
        block->capcity = MEM_PAGE_SIZE;
        block->remain_size = MEM_PAGE_SIZE;
        block->start = old_base;
        block->list_idx =  HEAP_FREE_LIST_NR -1;
        block->state = F_BLOCK_TYPE_EFFCTIVE;
        list_insert_last(&flmlk->free_list[HEAP_FREE_LIST_NR - 1], &block->node);
        old_base += MEM_PAGE_SIZE;
    }
}

void flmallocor_init(flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
    for (int i = 0; i < HEAP_FREE_LIST_NR; i++)
    {
        list_init(&flmlk->free_list[i]);
        flmlk->size[i] = power(2,i);
    }
}


int mlk_split(free_block_t* block,int size,flmallocor_t* f)
{
    if(block->capcity / 2 < size)
    {
        return 0;
    }
    if(!block)
    {
        dbg_error("block is NULL\r\n");
        return -1;
    }
    if(block->list_idx == -1)
    {
        dbg_error("block not in one list,can not split\r\n");
        return -1;
    }

    if(block->remain_size != block->capcity)
    {
        dbg_error("the block has stuff,can not split\r\n");
        return -1;
    }

    free_block_t* son = mlk_alloc_block();
    free_block_t* buddy = mlk_alloc_block();
    free_block_t* parent = block;

    son->start = parent->start;
    buddy->start = parent->start + parent->capcity / 2;
    son->buddy = buddy;
    buddy->buddy = son;
    son->parent = parent;
    buddy->parent = parent;
    mlk_put_block_into_list(son,parent->list_idx -1,f);
    mlk_put_block_into_list(buddy,parent->list_idx -1,f);
    mlk_remove_block_from_list(parent,parent->list_idx,f);
    parent->state = F_BLOCK_TYPE_SPLIT;

    int ret = mlk_split(son,size,f);
    return ret;
}

int mlk_collect(free_block_t* block,flmallocor_t* f)
{
    if(block->state != F_BLOCK_TYPE_EFFCTIVE)
    {
        dbg_error("block state is not right\r\n");
        return -1;
    }
    //最上面那个特殊处理
    if(block->capcity == MEM_PAGE_SIZE &&block->remain_size == MEM_PAGE_SIZE )
    {
        task_t* cur_task = get_cur_task();
        mlk_remove_block_from_list(block,block->list_idx,f);
        mmfree(cur_task,block->start,1);
        cur_task->heap_e-=MEM_PAGE_SIZE;
        mlk_free_block(block);
        return 0;
    }
    free_block_t* cur = block;
    free_block_t* buddy = cur->buddy;
    free_block_t* parent = cur->parent;
    if(buddy->parent!=cur->parent)
    {
        dbg_error("parent not the same\r\n");
        return -1;
    }

    if(!buddy && cur->capcity != MEM_PAGE_SIZE)
    {
        dbg_error("buddy dont know when free\r\n");
        return -2;
    }
    if(cur->list_idx!=buddy->list_idx || cur->capcity!=buddy->capcity)
    {
        dbg_error("cur block and buddy block not matching,not in the same list\r\n");
        return -3;
    }
    if(!parent)
    {
        dbg_error("parent not exsit,collec fail\r\n");
        return -4;
    }
    if(parent->capcity!=parent->remain_size)
    {
        dbg_error("parent block have things remain,can not split\r\n");
        return -5;
    }
    if(cur->state != F_BLOCK_TYPE_EFFCTIVE)
    {
        dbg_error("cur block state not effctive,can not collect\r\n");
        return -7;
    }

    //如果这两个block都空闲，才能回收
    if(cur->remain_size == cur->capcity && buddy->remain_size == buddy->capcity && buddy->state == F_BLOCK_TYPE_EFFCTIVE)
    {   
        int ret;
        mlk_remove_block_from_list(cur,cur->list_idx,f);
        cur->state = F_BLOCK_TYPE_SPLIT;
        mlk_free_block(cur);
        mlk_remove_block_from_list(buddy,buddy->list_idx,f);
        buddy->state = F_BLOCK_TYPE_SPLIT;
        mlk_free_block(buddy);
        mlk_put_block_into_list(parent,parent->list_idx,f);
        ret = mlk_collect(parent,f);
        if(ret == -9 || ret == 0)
        {
            return ret;
        }else{
            dbg_error("clloec process something wrong\r\n");
            return -8;
        }
    }
    else
    {
        if(buddy->remain_size<buddy->capcity)
        {
            return -9;
        }
        return -6;
    }

    return 0;
}

ph_addr_t mlk_alloc(uint32_t size,free_block_t** blk,flmallocor_t* f)
{
    flmallocor_t* flmlk = f;
    ph_addr_t ret;
    int i;
    for ( i = 0; i < HEAP_FREE_LIST_NR; i++)
    {
        //找到第一个比size大的
        if(flmlk->size[i] >= size)
        {
            break;
        }
    }
    //找到后，遍历链表，看看哪个block可用放数据
    list_t* target_list = &flmlk->free_list[i];
    list_node_t* cur = target_list->first;
    while (cur)
    {
        free_block_t* curb = list_node_parent(cur,free_block_t,node);
        if(curb->remain_size >= size)
        {
            ret = curb->start;
            curb->start += size;
            curb->remain_size -= size;
            *blk = curb;
            return ret;
        }
        cur = cur->next;
    }
    if(!cur)
    {
        //遍历链表，看看有没有block能够分配
        for(int j = i+1;j<HEAP_FREE_LIST_NR;++j)
        {
            list_t* col_list = &flmlk->free_list[j];
            list_node_t* curnode = col_list->first;
            while(curnode)
            {
                free_block_t* cur_b = list_node_parent(curnode,free_block_t,node);
                //找到一个可用分配的block
                if(cur_b->state == F_BLOCK_TYPE_EFFCTIVE && cur_b->remain_size == cur_b->capcity)
                {
                    //向下分裂
                    int x = mlk_split(cur_b,size,f);
                    if(x !=0)
                    {
                        //分裂失败
                        return NULL;
                    }else{
                        //分裂成功，就再次尝试分配内存
                        x = mlk_alloc(size,blk,f);
                        return x; //成功返回首地址，不成功返回NULL
                    }
                }
                curnode = curnode->next;
            }
        }
    }
    return NULL;
}



/*单位:字节*/
void* sys_malloc(uint32_t size)
{
    ph_addr_t ret = 0;
    uint32_t alloc_size = size + sizeof(mlk_info_head_t);
    free_block_t* blk = NULL;
    flmallocor_t* f = &(get_cur_task()->flmlk);
    ret = mlk_alloc(alloc_size,&blk,f);
    if(ret==NULL)
    {
        //内存不够分了，再开辟新的
        sbrk(size,f);
        int x = sys_malloc(size);
        return x;
    }
    mlk_info_head_t* head = (mlk_info_head_t*)ret;
    head->block = blk;
    head->size = size;

    return (void*)(ret + sizeof(mlk_info_head_t));
}

void sys_free(void* ptr)
{
    flmallocor_t* f = &(get_cur_task()->flmlk);
    ph_addr_t ptr_addr = (ph_addr_t)ptr;
    ptr_addr -= sizeof(mlk_info_head_t);
    mlk_info_head_t* head = (mlk_info_head_t*)ptr_addr;
    free_block_t* free_b = head->block;
    free_b->remain_size += head->size + sizeof(mlk_info_head_t);
    mlk_collect(free_b,f);
    
}

