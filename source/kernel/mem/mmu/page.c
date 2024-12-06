#include "page.h"
#include "mmu.h"
#include "debug.h"
/*管理所有物理页*/
page_t all_pages[PAGE_ALLOW_MAX];
vm_node_t vm_nodes[PAGE_ALLOW_MAX];

page_t *page_alloc(ph_addr_t ph,page_type_t type)
{
    int idx = ph / MEM_PAGE_SIZE;
    all_pages[idx].type = type;
    list_init(&all_pages[idx].rela_vm_list);
    return &all_pages[idx];
}


void page_free(page_t* page)
{
    memset(page,0,sizeof(page_t));
}


vm_node_t *vm_alloc(ph_addr_t vm)
{
    vm_node_t * ret = NULL;
    for(int i=0;i<PAGE_ALLOW_MAX;++i)
    {
        if(!vm_nodes[i].pressent)
        {
            vm_nodes[i].pressent = 1;
            vm_nodes[i].vm = vm;
            ret = &vm_nodes[i];
            break;
        }
    }
    return ret;
}

void vm_free(vm_node_t* virtual)
{
    memset(virtual,0,sizeof(vm_node_t));
}

page_t* get_page_struct(ph_addr_t ph)
{
    int idx = ph / MEM_PAGE_SIZE;
    return &all_pages[idx];
}

int page_add_vmnode(page_t* page,ph_addr_t vm)
{
    vm_node_t* vmnode = vm_alloc(vm);
    list_insert_last(&page->rela_vm_list,&vmnode->node);
    page->ref++;
    return 0;
}
int page_remove_vm_from_list(ph_addr_t vm,page_t* page)
{
    list_t* list = &page->rela_vm_list;
    list_node_t* rnode = NULL;
    list_node_t* cur = list_first(list);
    while(cur)
    {
        vm_node_t* vnode = list_node_parent(cur,vm_node_t,node);
        if(vnode->vm == vm)
        {
            rnode = &vnode->node;
            break;
        }
        list_node_next(cur);
    }
    if(rnode == NULL)
    {
        dbg_warning("the vm not recorded in the page struct\r\n");
        return -1;
    }
    list_remove(list,rnode);
    return 0;
}

int page_record_map_ref(ph_addr_t vm,page_t* page)
{
    int ret;
    ret = page_add_vmnode(page,vm);
    if(ret<0)
    {
        return -1;
    }
    return 0;
}

int page_dispel_map_ref(ph_addr_t vm,pde_t* page_dir)
{
    int ret;
    ph_addr_t ph = mmu_get_phaddr(page_dir,vm);
    if(ph == NULL)
    {
        dbg_error("vm or page_dir wrong\r\n");
        return -1;
    }
    page_t* page = get_page_struct(ph);
    ret = page_remove_vm_from_list(vm,page);
    if(ret<0)
    {
        return -1;
    }
    page->ref--;
    return 0;
    
}

int page_get_ph_ref(ph_addr_t vm,pde_t* page_dir)
{
    ph_addr_t ph = mmu_get_phaddr(page_dir,vm);
    if(ph == NULL)
    {
        dbg_error("vm or page_dir wrong\r\n");
        return -1;
    }
    page_t* page = get_page_struct(ph);
    if(!page)
    {
        return -1;
    }
    return page->ref;
}