/*
 * vmm_errno.h
 */


#ifndef VMM_ERRNO_H_
#define VMM_ERRNO_H_

#define FHK_STD_MOD_VMM					(269)
#define FHK_STD_DRV_ID					(0x80000000) /* 系统\驱动层 */
#define FHK_STD_SUBLIB_ID				(0x90000000) /* lib内部逻辑层 */
#define FHK_STD_API_ID					(0xA0000000) /* api 层 */
#define FHK_STD_DRV_ERR(mod, lev, id)	((int)((FHK_STD_DRV_ID) | ((mod) << 16) | (lev) << 12) | (id))
#define FHK_STD_API_ERR(mod, lev, id)	((int)((FHK_STD_API_ID) | ((mod) << 16) | (lev) << 12) | (id))
#define FHK_STD_ERR2MOD(error)			((((FH_UINT32)(error)) >> 16) & 0x1ff)
#define FHK_STD_ERR2ID(error)			(((FH_UINT32)(error)) & 0xfff)

/*错误级别*/
enum FHK_STD_LEVEL_ID
{
	FHK_STD_LEV_DEBG = 0,      /* debug-level                                  */
	FHK_STD_LEV_INFO = 1,      /* informational                                */
	FHK_STD_LEV_NOTC = 2,      /* normal but significant condition            */
	FHK_STD_LEV_WARN = 3,      /* warning conditions                          */
	FHK_STD_LEV_ERRR = 4,      /* error conditions                            */
	FHK_STD_LEV_CRIT = 5,      /* critical conditions                          */
	FHK_STD_LEV_ALRT = 6,      /* action must be taken immediately            */
	FHK_STD_LEV_FATL = 7,      /* just for compatibility with previous version */
};

enum VMM_ERRNO
{
	ERR_NOMEM = 1,
	ERR_SYS_NOMEM,
	ERR_MULTI_USERS,
	ERR_ADDR_OVERFLOW,
	ERR_BAD_ADDRESS,
	ERR_MAP,
	ERR_OPEN_DEVICE,
	ERR_32BIT_ADDR,
	ERR_INVALID_PARAM,
	ERR_SYS_ERR,
};

#define VMM_ERR_NOMEM			FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_NOMEM)
#define VMM_ERR_SYS_NOMEM		FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_SYS_NOMEM)
#define VMM_ERR_MULTI_USERS		FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_MULTI_USERS)
#define VMM_ERR_ADDR_OVERFLOW	FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_ADDR_OVERFLOW)
#define VMM_ERR_BAD_ADDRESS		FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_BAD_ADDRESS)
#define VMM_ERR_MAP				FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_MAP)
#define VMM_ERR_OPEN_DEVICE		FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_OPEN_DEVICE)
#define VMM_ERR_32BIT_ADDR		FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_32BIT_ADDR)
#define VMM_ERR_INVALID_PARAM	FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_INVALID_PARAM)
#define VMM_ERR_SYS_ERR			FHK_STD_API_ERR(FHK_STD_MOD_VMM, FHK_STD_LEV_FATL, ERR_SYS_ERR)

#endif /* VMM_ERRNO_H_ */
