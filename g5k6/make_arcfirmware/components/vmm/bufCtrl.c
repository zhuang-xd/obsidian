#include <stdio.h>
#include "types/bufCtrl.h"
#include "vmm.h"
#include "vmm_errno.h"

#define fh_printf    rt_kprintf


struct vmm_manager g_vmm_manager={
    .vmm_count = 1,
};

/**************************************************
Function:		shareBufferInit
Description:

Input:

Output:
Returns:         OK or ERR Information
 **************************************************/
static inline int _bufferInit(struct vmm_control *vc, char *name, unsigned char *pAddr, unsigned int bufSize,
                              unsigned int align)
{
    return vmm_init(vc, name, (unsigned int)pAddr, bufSize, align);
}

int bufferInit(unsigned char* pAddr, unsigned int bufSize)
{
    return _bufferInit(&g_vmm_manager.buffers[0], "Media_Buf", pAddr, bufSize, 4);
}

int buffer_malloc(MEM_DESC *mem, int size, int align)
{
    unsigned char *ptr = vmm_malloc_with_name(&g_vmm_manager.buffers[0], size, align, "def_mem");

    if (!ptr)
    {
        return VMM_ERR_NOMEM;
    }
    else
    {
        mem->base = (unsigned int)ptr;
        mem->vbase = ptr;
        mem->size = size;
        //mem->align = align; //must comment me. In DSP, MEM_INFO will be used as MEM_DESC
    }
    return 0;
}

static inline int _buffer_malloc_withname(struct vmm_control *vc, MEM_DESC *mem, int size, int align,  char *name)
{
    unsigned char *ptr = vmm_malloc_with_name(vc, size, align, name);

    if (!ptr)
    {
        return VMM_ERR_NOMEM;
    }
    else
    {
        if (mem)
        {
            mem->base = (unsigned int)ptr;
            mem->vbase = ptr;
            mem->size = size;
            //mem->align = align; //must comment me. In DSP, MEM_INFO will be used as MEM_DESC
        }
    }
    return 0;
}

int buffer_malloc_withname(MEM_DESC *mem, int size, int align,  char* name)
{
    return _buffer_malloc_withname(&g_vmm_manager.buffers[0], mem, size, align, name);
}

void* buffer_get_vmm_info(unsigned int *size)
{
	return vmm_get_info(&g_vmm_manager.buffers[0], size);
}

int buffer_reset_vmm(void)
{
    return vmm_reset(&g_vmm_manager.buffers[0]);
}

int buffer_free(unsigned int paddr)
{
    if (paddr == 0)
        return -22;
    return vmm_free(&g_vmm_manager.buffers[0], paddr);
}

void media_mem_proc(void)
{
    vmm_dump_all_info(&g_vmm_manager.buffers[0]);
}

cmm_handle cmm_init(unsigned char* pAddr, unsigned int bufSize)
{
    int ret;

    if (g_vmm_manager.vmm_count == MAX_VMM_COUNT)
        return 0;
    _bufferInit(&g_vmm_manager.buffers[g_vmm_manager.vmm_count],"cmm buffer", pAddr, bufSize, 4);

    ret = g_vmm_manager.vmm_count++;
    return ret;

}

int cmm_malloc(cmm_handle handle, MEM_DESC *mem, int size, int align,  char* name)
{
    return _buffer_malloc_withname(&g_vmm_manager.buffers[handle], mem, size, align, name);
}

int cmm_free(cmm_handle handle)
{
    fh_printf("Not implimentation.\n");
    return 0;
}

void cmm_mem_proc(cmm_handle handle)
{
    vmm_dump_all_info(&g_vmm_manager.buffers[handle]);
}
