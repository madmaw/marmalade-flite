#ifndef _CMU_US_SLT_H__
#define _CMU_US_SLT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "cst_voice.h"

cst_voice *register_cmu_us_slt(const char *voxdir);
void unregister_cmu_us_slt(cst_voice *vox);

#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */

#endif//_CMU_US_SLT_H__