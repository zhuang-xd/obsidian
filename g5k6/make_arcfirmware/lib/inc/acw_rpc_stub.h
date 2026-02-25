#ifndef __acw_rpc_stub_h__
#define __acw_rpc_stub_h__

struct audio_init_ctrl
{
    unsigned char* mem_start_addr;
    unsigned int   cap_mem_size;
    unsigned int   ply_mem_size;
    unsigned int   raw_mem_size;
    unsigned int   swap_mem_size;
};

extern int fh_audio_module_int(void *mem, int cap_mem_size, int play_mem_size, int swap_mem_size);

#endif //__acw_rpc_stub_h__
