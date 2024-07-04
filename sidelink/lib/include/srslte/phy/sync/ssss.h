/**
* Copyright 2013-2019 
* Fraunhofer Institute for Telecommunications, Heinrich-Hertz-Institut (HHI)
*
* This file is part of the HHI Sidelink.
*
* HHI Sidelink is under the terms of the GNU Affero General Public License
* as published by the Free Software Foundation version 3.
*
* HHI Sidelink is distributed WITHOUT ANY WARRANTY,
* without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* A copy of the GNU Affero General Public License can be found in
* the LICENSE file in the top-level directory of this distribution
* and at http://www.gnu.org/licenses/.
*
* The HHI Sidelink is based on srsLTE.
* All necessary files and sources from srsLTE are part of HHI Sidelink.
* srsLTE is under Copyright 2013-2017 by Software Radio Systems Limited.
* srsLTE can be found under:
* https://github.com/srsLTE/srsLTE
*/

/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 *  File:         ssss.h
 *
 *  Description:  Secondary sidelink synchronization signal (SSS) generation and detection.
 *
 *
 *  Reference:    3GPP TS 36.211 version 14.2.0 Release 14 Sec. 9.7.2
 *****************************************************************************/

#ifndef SRSLTE_SSSS_H
#define SRSLTE_SSSS_H

#include <stdint.h>
#include <stdbool.h>

#include "srslte/phy/sync/sss.h"
#include "srslte/config.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/dft/dft.h"


#define SRSLTE_SSSS_N      31
#define SRSLTE_SSSS_LEN    2*SRSLTE_SSS_N

typedef srslte_sss_tables_t _srslte_ssss_tables_t;

/* Allocate 32 complex to make it multiple of 32-byte AVX instructions alignment requirement.
 * Should use srslte_vec_malloc() to make it platform agnostic.
 */
typedef srslte_sss_fc_tables_t _srslte_ssss_fc_tables_t;


/* Low-level API */
typedef srslte_sss_t srslte_ssss_t;


/* Basic functionality */
SRSLTE_API int srslte_ssss_init(srslte_ssss_t *q,
                                     uint32_t fft_size);

SRSLTE_API int srslte_ssss_resize(srslte_ssss_t *q,
                                        uint32_t fft_size); 

SRSLTE_API void srslte_ssss_free(srslte_ssss_t *q);

SRSLTE_API void srslte_ssss_generate(float *signal,
                                        uint32_t cell_id);

SRSLTE_API void srslte_ssss_put_sf(float *ssss,
                                    cf_t *sf,
                                    uint32_t nof_prb,
                                    srslte_cp_t cp);

SRSLTE_API int srslte_ssss_set_N_id_2(srslte_ssss_t *q,
                                           uint32_t N_id_2);

SRSLTE_API int _srslte_ssss_m0m1_partial(srslte_sss_t *q,
                                             const cf_t *input,
                                             uint32_t M, 
                                             cf_t ce[2*SRSLTE_SSS_N],
                                             uint32_t *m0, 
                                             float *m0_value, 
                                             uint32_t *m1, 
                                             float *m1_value);

SRSLTE_API int _srslte_ssss_m0m1_diff_coh(srslte_sss_t *q,
                                              const cf_t *input,
                                              cf_t ce[2*SRSLTE_SSS_N],
                                              uint32_t *m0, 
                                              float *m0_value, 
                                              uint32_t *m1, 
                                              float *m1_value);

SRSLTE_API int _srslte_ssss_m0m1_diff(srslte_sss_t *q,
                                          const cf_t *input,
                                          uint32_t *m0, 
                                          float *m0_value, 
                                          uint32_t *m1, 
                                          float *m1_value);


SRSLTE_API uint32_t _srslte_ssss_subframe(uint32_t m0,
                                              uint32_t m1);

SRSLTE_API int _srslte_ssss_N_id_1(srslte_sss_t *q,
                                       uint32_t m0, 
                                       uint32_t m1);

SRSLTE_API int _srslte_ssss_frame(srslte_sss_t *q,
                                      cf_t *input, 
                                      uint32_t *subframe_idx, 
                                      uint32_t *N_id_1);

SRSLTE_API void _srslte_ssss_set_threshold(srslte_sss_t *q,
                                               float threshold);

SRSLTE_API void _srslte_ssss_set_symbol_sz(srslte_sss_t *q,
                                               uint32_t symbol_sz);

SRSLTE_API void _srslte_ssss_set_subframe_sz(srslte_sss_t *q,
                                                 uint32_t subframe_sz);

#endif // SRSLTE_SSSS_H

