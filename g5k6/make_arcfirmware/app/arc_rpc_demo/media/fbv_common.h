#include <rtthread.h>
#include <asm/cacheflush.h>
#include <fh_pwm.h>
#include "config.h"
#include "asm/io.h"
#include "types/type_def.h"
#include "types/bufCtrl.h"
#include "isp/isp_common.h"
#include "isp/isp_api.h"
#include "isp/isp_enum.h"
#include "vicap/fh_vicap_mpi.h"
#include "dsp/fh_common.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_vpu_mpipara.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_audio_mpi.h"
#include "dsp/fh_audio_mpipara.h"
#include "mpp/fh_vb_mpi.h"
#include "mpp/fh_vb_mpipara.h"
#include "fbv_isp_init.h"
#include "fbv_init_dsp.h"
#include "fbv_init_osd.h"
#include "fbv_init_nna.h"
#include "fbv_isp_sensor_func.h"
#include "fbv_enc_config.h"
#include "fbv_init_osd.h"
#include "fbv_audio.h"
#include "fh_time_stat.h"
#include "dsp/fh_venc_mpipara.h"



#define FBV_FUNC_ERROR_PRT(func_name, ret)               \
    do                                                   \
    {                                                    \
        if (ret)                                        \
        {                                                \
            rt_kprintf("%s faild ret:%x\n", func_name, ret); \
            return ret;                                  \
        }                                                \
    } while (0)
extern unsigned int  read_pts32(void);