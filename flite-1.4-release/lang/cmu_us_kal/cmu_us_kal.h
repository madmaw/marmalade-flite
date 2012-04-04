#ifndef _CMU_US_KAL_H__
#define _CMU_US_KAL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "cst_voice.h"

cst_voice *register_cmu_us_kal(const char *voxdir);
void unregister_cmu_us_kal(cst_voice *vox);

#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */

#endif//_CMU_US_KAL_H__