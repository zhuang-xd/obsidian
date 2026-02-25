#ifndef __RPC_VMM_h__
#define __RPC_VMM_h__

/*don't change it unless you know what you want to do*/
enum
{
    RPC_FUNC_VMM_MALLOC = 0,
    RPC_FUNC_VMM_FREE = 1,
    RPC_FUNC_VMM_RESET = 2,
    RPC_FUNC_VMM_GET_INFO = 3,
    RPC_FUNC_VMM_GET_BLOCK_INFO = 4,
    RPC_FUNC_VMM_INIT = 5,
};

typedef struct {
    struct rpc_hdr hdr;
    unsigned int size;
    unsigned int align;
    char name[32];
} RPC_IN_VMM_ALLOC_T;

typedef struct {
    struct rpc_hdr hdr;
    unsigned int addr;
} RPC_OUT_VMM_ALLOC_T;

typedef union {
    RPC_IN_VMM_ALLOC_T in;
    RPC_OUT_VMM_ALLOC_T out;
} RPC_IOC_VMM_ALLOC_T;

typedef struct {
    struct rpc_hdr hdr;
    unsigned int addr;
} RPC_IN_VMM_FREE_T;

typedef struct {
    struct rpc_hdr hdr;
    unsigned int addr;
    unsigned int size;
    unsigned int offset;
} RPC_IOC_VMM_BLOCK_T;

typedef struct {
    struct rpc_hdr hdr;
    unsigned int addr;
    unsigned int size;
} RPC_IO_VMM_INFO_T;


#endif
