#ifndef __FBV_NNA_INIT_H__
#define __FBV_NNA_INIT_H__
#include <float.h>
#include <stdint.h>
#include <stdbool.h>
#include "dsp/fh_nna_mpi.h"
#include "dsp/fh_nna_mpipara.h"
#include "fh_nnprimary_det_post_mpi.h"
#define PP_MAX_BBOX_NUM 150
#define NN_DETECT_CHAN 2
#define NN_DETECT_FRAMES    (5)

typedef struct _model_check_hdr
{
    unsigned int check_magic;
    unsigned int headLen;
    unsigned int dataLen;
    unsigned int reserved1;
    unsigned int reserved2;
    unsigned int reserved3;
    unsigned int reserved4;
    unsigned int reserved5;
    unsigned char model_data[0];
} MODEL_CHECK_HDR_t;

typedef struct _nn_addr_info
{
    unsigned int nbg_load_addr;
    unsigned int pkg_addr;
    unsigned int image_input_addr;
    unsigned int post_process_addr;
    unsigned int post_length;
    unsigned int reserved1;
    unsigned int reserved2;
    unsigned int reserved3;
} NN_ADDR_INFO_t;

void start_do_nn_detect(void);

#endif /* __FBV_NNA_INIT_H__ */




