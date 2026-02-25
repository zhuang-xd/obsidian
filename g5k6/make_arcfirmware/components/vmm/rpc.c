#include <rtthread.h>
#include <rpc_slave.h>
#include <rpc_hdr.h>
#include <control_rpc_proto.h>
#include "vmm.h"
#include "rpc_vmm.h"
#include "../ringbuffer/ringbuffer.h"

extern struct vmm_manager g_vmm_manager;


int stub_vmm_malloc_with_name(struct rpc_hdr *phdr, unsigned int pkt_len, void *priv)
{
    RPC_IN_VMM_ALLOC_T *pin = (RPC_IN_VMM_ALLOC_T *)phdr;
    RPC_OUT_VMM_ALLOC_T out;

    void *ptr = vmm_malloc_with_name(&g_vmm_manager.buffers[0], pin->size, pin->align, pin->name);

    out.addr = (unsigned int)ptr;
    out.hdr.ret = 0;

    rpc_send_back((struct rpc_hdr *)&out, sizeof(out), priv);

    return 0;
}

int stub_vmm_free(struct rpc_hdr *phdr, unsigned int pkt_len, void *priv)
{
    RPC_IN_VMM_FREE_T *pin = (RPC_IN_VMM_FREE_T *)phdr;

    pin->hdr.ret = vmm_free(&g_vmm_manager.buffers[0], (rt_uint32_t)pin->addr);

    rpc_send_back((struct rpc_hdr *)phdr, sizeof(*phdr), priv);

    return 0;
}

int stub_vmm_get_block_info(struct rpc_hdr *phdr, unsigned int pkt_len, void *priv)
{
    RPC_IOC_VMM_BLOCK_T *pin = (RPC_IOC_VMM_BLOCK_T *)phdr;

    pin->hdr.ret = vmm_get_block_info(&g_vmm_manager.buffers[0], &pin->addr, &pin->size);

    if (pin->hdr.ret >= 0)
    {
        pin->offset = pin->hdr.ret;
        pin->hdr.ret = 0;
    }

    rpc_send_back((struct rpc_hdr *)pin, sizeof(*pin), priv);

    return 0;
}

int stub_buffer_reset_vmm(struct rpc_hdr *phdr, unsigned int pkt_len, void *priv)
{
    phdr->ret = vmm_reset(&g_vmm_manager.buffers[0]);

    rpc_send_back((struct rpc_hdr *)phdr, sizeof(*phdr), priv);

    return 0;
}

int stub_get_vmm_info(struct rpc_hdr *phdr, unsigned int pkt_len, void *priv)
{
    RPC_IO_VMM_INFO_T out;

    out.addr = (unsigned int)vmm_get_info(&g_vmm_manager.buffers[0], &out.size);

    rpc_send_back((struct rpc_hdr *)&out, sizeof(out), priv);

    return 0;
}

int stub_vmm_init(struct rpc_hdr *phdr, unsigned int pkt_len, void *priv)
{

    RPC_IO_VMM_INFO_T *pin = (RPC_IO_VMM_INFO_T *)phdr;

    pin->hdr.ret = vmm_init(&g_vmm_manager.buffers[0], "anonymous", pin->addr, pin->size, 0x1000);

    rpc_send_back((struct rpc_hdr *)phdr, sizeof(*phdr), priv);

    return 0;
}

static rpc_function_cb vmm_rpc_func_list[] =
{
    stub_vmm_malloc_with_name,
    stub_vmm_free,
    stub_buffer_reset_vmm,
    stub_get_vmm_info,
    stub_vmm_get_block_info,
    stub_vmm_init,
};

static int vmm_proc_show(void *handle)
{
    rpc_vmm_dump_all_info(handle, &g_vmm_manager.buffers[0]);
    return 0;
}

int rpc_vmm_init_arc(void)
{
    int ret;
    ret = rpc_register_table(RPC_VERSION_CONTROL, RPC_TABLE_ID_VMM, vmm_rpc_func_list, \
                              sizeof(vmm_rpc_func_list) / sizeof(vmm_rpc_func_list[0]));
    if (ret) {
        rt_kprintf("vmm rpc_register_table failed\n");
        return ret;
    }
    ret = proc_register("vmm", vmm_proc_show, RT_NULL);
    if (ret) {
        rt_kprintf("proc_register vmm failed!\n");
        return ret;
    }
    return 0;
}
