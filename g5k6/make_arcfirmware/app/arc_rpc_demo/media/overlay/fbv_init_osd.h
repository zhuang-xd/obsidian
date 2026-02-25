#ifndef __FBV_INIT_OSD_H__
#define __FBV_INIT_OSD_H__
#include "FHAdv_OSD_mpi.h"
FH_SINT32 fbv_osd_init(FH_SINT32 grp_id);
FH_SINT32 sample_set_gbox(FH_SINT32 grp_id, FH_UINT32 chan, FH_UINT32 enable, FH_UINT32 box_id, FH_UINT32 x, FH_UINT32 y, FH_UINT32 w, FH_UINT32 h, FHT_RgbColor_t color);

#endif /* __FBV_INIT_OSD_H__ */
