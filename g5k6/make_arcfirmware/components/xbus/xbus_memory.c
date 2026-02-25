#include <rtthread.h>
#include "xbus_osl.h"
#include "xbus_core.h"
#include <types/bufCtrl.h>
#include <asm/io.h>
#include <fh_chip.h>
#include <firmware_info.h>
#include "c2c_comm.h"

struct xbus_sys_info *xbus_sysinfo;

struct xbus_ring     *XOUT_RING; //relative to ARC
unsigned int          XOUT_RING_SZ; 

struct xbus_ring     *XIN_RING; //relative to ARC
unsigned int          XIN_RING_SZ;

struct rsh_ring      *RSH_OUT;  //relative to ARC
unsigned int          RSH_OUT_SZ;

struct rsh_ring      *RSH_IN;   //relative to ARC

unsigned int          g_trigger_intr_lost;

extern void* buffer_get_vmm_info(unsigned int *size);

int xbus_init_memory(unsigned int addr, unsigned int size)
{
    struct xbus_sys_info *info;
    struct rsh_ring      *rshout;
    struct rsh_ring      *rshin;
    struct xbus_ring     *xout;
    struct xbus_ring     *xin;

    unsigned int rshout_sz;
    unsigned int rshin_sz;
    unsigned int xout_sz;
    unsigned int xin_sz;

    unsigned int halfsz = size >> 1;

    XBusMemSet((void*)addr, 0, size);

    rshout_sz = RSH_SIZE_MASTER_OUT;
    rshin_sz  = halfsz - (sizeof(struct xbus_sys_info) + rshout_sz);
    xout_sz   = halfsz >> 1;
    xin_sz    = halfsz >> 1;    

    rshout = (struct rsh_ring *)(addr + sizeof(struct xbus_sys_info));
    rshout->magic = XBUS_RSHOUT_MAGIC;

    rshin = (struct rsh_ring *)((unsigned int)rshout + rshout_sz);
    rshin->magic = XBUS_RSHIN_MAGIC;

    xout = (struct xbus_ring *)((unsigned int)rshin + rshin_sz);
    xout->magic = XBUS_XOUT_MAGIC;

    xin = (struct xbus_ring *)((unsigned int)xout + xout_sz);
    xin->magic = XBUS_XIN_MAGIC;

    info = (struct xbus_sys_info *)addr;    
    info->rshout_sz = rshout_sz;
    info->rshin_sz  = rshin_sz;
    info->xout_sz   = xout_sz;
    info->xin_sz    = xin_sz;
    info->firmware_git_version = readl(&_arc_fw_sdk_git_version);
    info->magic     = XBUS_SYS_INFO_MAGIC;

    MMU_FLUSH_DCACHE(info, size);
    MMU_INVALIDATE_DCACHE(info, size);

    xbus_sysinfo = info;

    XOUT_RING    = xin;
    XOUT_RING_SZ = xin_sz - sizeof(struct xbus_ring);

    XIN_RING     = xout;
    XIN_RING_SZ  = xout_sz - sizeof(struct xbus_ring);

    RSH_OUT      = rshin;
    RSH_OUT_SZ   = rshin_sz - sizeof(struct rsh_ring);

    RSH_IN       = rshout;

    return 0;
}

void xbus_notify_host(void)
{
#ifdef FH_USING_VMM_ON_ARC
    unsigned int vmm_addr;
    unsigned int vmm_size;

    vmm_addr = (unsigned int)buffer_get_vmm_info(&vmm_size);
    writel(vmm_addr, &xbus_sysinfo->vmm_phy_addr);
    writel(vmm_size, &xbus_sysinfo->vmm_size);
#endif

    writel((unsigned int)xbus_sysinfo, &_arc_fw_xbus_sysinfo);
    writel(0x55555555, &_arc_fw_ready_flag);

    /*notify arm that ARC is ready*/
    fh_mcu_notify_xbus_buffer_address(xbus_sysinfo);
}

void xbus_trigger_host_intr(unsigned short type, void *args, unsigned short len)
{
    unsigned int msg;

    msg = readl(&xbus_sysinfo->cb_msg_info);
    if (msg)
    {
        g_trigger_intr_lost++;
        return;
    }

    msg = (((unsigned int)type) << 16) | len;

    if (msg && len <= XBUS_INTR_CALLBACK_MEM_SIZE)
    {
        if (len > 0)
        {
            if (args)
            {
                rt_memcpy((void*)xbus_sysinfo->intr_cb_mem, args, len);
                mmu_clean_dcache_intron((rt_uint32_t)(&xbus_sysinfo->intr_cb_mem[0]), len);
            }
            else /*error*/
            {
                g_trigger_intr_lost++;
                return;
            }
        }

        writel(msg, &xbus_sysinfo->cb_msg_info);

        //trigger interrupt to peer
        fh_mcu_trigger_interrupt_to_host();
    }
    else //error
    {
        g_trigger_intr_lost++;
    }
}
