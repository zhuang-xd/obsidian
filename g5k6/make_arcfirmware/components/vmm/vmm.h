/*
 * vmm.h
 *
 *  Created on: Jul 21, 2015
 *      Author: duobao
 */

#ifndef VMM_H_
#define VMM_H_

#include <rtdevice.h>
#include <rtdef.h>

#define VMM_MAX_NAME_LEN		16
#define MAX_VMM_COUNT 2

struct vmm_control
{
    /*here must be the first...*/
    rt_list_t vmm_node_list;

    char name[VMM_MAX_NAME_LEN];

    rt_uint32_t align;
    rt_uint32_t begin_addr;
    rt_uint32_t end_addr;   /*include end_addr itself*/
    rt_uint32_t nbytes;     /*length */
};

struct vmm_manager{
    struct vmm_control buffers[MAX_VMM_COUNT];
    int vmm_count;
};


int vmm_init(
        struct vmm_control *p_vmm, 
        char *name, 
        rt_uint32_t beginAddr,
        rt_uint32_t bufSize, 
        rt_uint32_t bufAllign);

void* vmm_malloc_with_name(
        struct vmm_control *p_vmm, 
        rt_uint32_t size,
        rt_uint32_t align, 
        char *name);

void* vmm_malloc(struct vmm_control *p_vmm, rt_size_t size, rt_size_t align);

void* vmm_get_info(struct vmm_control *p_vmm,unsigned int *size);

int vmm_free(struct vmm_control *p_vmm, rt_uint32_t addr);

int vmm_reset(struct vmm_control *p_vmm);

int vmm_get_block_info(struct vmm_control *p_vmm, unsigned int *addr_in, unsigned int *size);

void vmm_dump_all_info(struct vmm_control *p_vmm);
void rpc_vmm_dump_all_info(void *handle, struct vmm_control *p_vmm);


#endif /* VMM_H_ */
