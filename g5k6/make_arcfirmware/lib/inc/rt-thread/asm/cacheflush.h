#ifndef __cache_flush_h__
#define __cache_flush_h__
#include <rtdef.h>

unsigned int mmu_dcache_linesz(void);
void mmu_clean_dcache_intron(rt_uint32_t buffer, rt_uint32_t size);
void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);

void fh_hw_flush_dcache(rt_uint32_t buffer, rt_uint32_t size);
void fh_hw_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);

void fh_hw_flush_dcache_all(void);

#endif /*__cache_flush_h__*/
