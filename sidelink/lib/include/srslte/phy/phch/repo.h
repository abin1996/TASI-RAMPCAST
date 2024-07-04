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
 *  File:         repo.h
 *
 *  Description:  Subframe and Resource block pools for sidelink
 *
 *  Reference:    3GPP TS 36.211 version 14.2.0 Release 14
 *****************************************************************************/

#ifndef SRSLTE_REPO_H
#define SRSLTE_REPO_H

#include <stdbool.h>
#include "srslte/config.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/phch/ra.h"



typedef struct {
  // indicates the offset of the first subframe of a resource pool, i.e. the starting subframe of repeating bitmap
  // sl_Subframe, within a SFN cycle
  uint8_t sl_OffsetIndicator_r14;

  // subframe bitmap
  uint8_t sl_Subframe_r14[100];
  uint8_t sl_Subframe_r14_len;

  bool adjacencyPSCCH_PSSCH_r14;

  // indicated the number of PRBs of each subchannel in the corresponding resource pool
  uint8_t sizeSubchannel_r14;

  // indicates the number of subchannels in the corresponding resource pool
  uint8_t numSubchannel_r14;

  // indicates the lowest RB index of the subchannel with the lowest index
  uint8_t startRB_Subchannel_r14;

  // indicates the lowest RB index of PSCCH pool, this field is absent whe a pool is (pre)configured
  // such tha a UE always transmits SC and data in adjacent RBs in the same subframe
  uint8_t startRB_PSCCH_Pool_r14;

  // ...

} SL_CommResourcePoolV2X_r14;


typedef struct SRSLTE_API {
  srslte_cell_t cell;
  SL_CommResourcePoolV2X_r14 rp;

  // values for defining sync periodicity and offset
  uint32_t syncOffsetIndicator_r12;
  uint32_t syncPeriod;

  short *subframe_rp;
}srslte_repo_t;

SRSLTE_API int srslte_repo_init(srslte_repo_t *h,                          
                                srslte_cell_t cell,
                                SL_CommResourcePoolV2X_r14 pool);

SRSLTE_API void srslte_repo_free(srslte_repo_t *h);

SRSLTE_API void srslte_repo_build_resource_pool(srslte_repo_t *q);

SRSLTE_API int srslte_repo_sci_decode(srslte_repo_t *q,
                                      uint8_t *sci_packed,
                                      srslte_ra_sl_sci_t *sci);

SRSLTE_API void srslte_repo_sci_encode(srslte_repo_t *q,
                                        uint8_t *sci_packed,
                                        srslte_ra_sl_sci_t *sci);

SRSLTE_API void srslte_repo_encode_frl(srslte_repo_t *q,
                                        srslte_ra_sl_sci_t *sci,
                                        uint8_t L_subCH,
                                        uint8_t n_subCH);

SRSLTE_API int srslte_repo_frl_len(srslte_repo_t *q);

SRSLTE_API uint32_t srslte_repo_encode_rr(uint8_t rri);

SRSLTE_API uint8_t srslte_repo_decode_rr(uint32_t rr);

SRSLTE_API uint32_t srslte_repo_get_n_PSSCH_ssf(srslte_repo_t *q,
                                                uint32_t subframe);

SRSLTE_API int32_t srslte_repo_get_t_SL_k(srslte_repo_t *q,
                                          uint32_t subframe);

#endif // SRSLTE_REPO_H


