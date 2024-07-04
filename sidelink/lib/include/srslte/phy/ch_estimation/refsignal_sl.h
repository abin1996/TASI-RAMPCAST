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

/**********************************************************************************************
 *  File:         refsignal_sl.h
 *
 *  Description:  Object to manage sidelink reference signals for channel estimation.
 *
 *  Reference:    3GPP TS 36.211 version 14.2.0 Release 14 Sec. 9.8
 *********************************************************************************************/

#ifndef SRSLTE_REFSIGNAL_SL_H
#define SRSLTE_REFSIGNAL_SL_H

#include "srslte/config.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/ch_estimation/refsignal_ul.h"

typedef srslte_refsignal_ul_t srslte_refsignal_sl_t;

SRSLTE_API int srslte_refsignal_sl_dmrs_psbch_gen(srslte_refsignal_sl_t *q, 
                                                  uint32_t nof_prb, 
                                                  uint32_t sf_idx, 
                                                  uint32_t cyclic_shift_for_dmrs,
                                                  cf_t *r_pusch);

SRSLTE_API uint32_t srslte_refsignal_sl_dmrs_psbch_N_rs(srslte_sl_mode_t mode, uint32_t slot_idx);

SRSLTE_API int srslte_refsignal_sl_dmrs_psbch_get(srslte_refsignal_sl_t *q,
                                                    srslte_sl_mode_t sl_mode,
                                                    cf_t *input,
                                                    cf_t *r_psbch);

SRSLTE_API int srslte_refsignal_sl_dmrs_psbch_put(srslte_refsignal_sl_t *q,
                                                    srslte_sl_mode_t sl_mode,
                                                    cf_t *r_psbch,
                                                    cf_t *output);

SRSLTE_API uint32_t srslte_refsignal_sl_dmrs_psbch_symbol(uint32_t m,
                                                            srslte_sl_mode_t mode,
                                                            uint32_t slot,
                                                            srslte_cp_t cp);


SRSLTE_API int srslte_refsignal_sl_dmrs_psxch_get(srslte_refsignal_sl_t *q,
                                                    srslte_sl_mode_t sl_mode,
                                                    uint32_t prb_offset,
                                                    uint32_t N_prbs,
                                                    cf_t *input,
                                                    cf_t *r_psxch);

SRSLTE_API int srslte_refsignal_sl_dmrs_psxch_put(srslte_refsignal_sl_t *q,
                                                    srslte_sl_mode_t sl_mode,
                                                    uint32_t prb_offset,
                                                    uint32_t N_prbs,
                                                    cf_t *r_psxch,
                                                    cf_t *output);


SRSLTE_API int srslte_refsignal_sl_dmrs_pscch_gen(srslte_refsignal_sl_t *q, 
                                                  uint32_t nof_prb, 
                                                  uint32_t sf_idx, 
                                                  uint32_t cyclic_shift_for_dmrs,
                                                  cf_t *r_pusch);

SRSLTE_API uint32_t srslte_refsignal_sl_dmrs_pscch_N_rs(srslte_sl_mode_t mode,
                                                        uint32_t slot_idx);

SRSLTE_API uint32_t srslte_refsignal_sl_dmrs_pscch_symbol(uint32_t m,
                                                            srslte_sl_mode_t mode,
                                                            uint32_t slot,
                                                            srslte_cp_t cp);




SRSLTE_API int srslte_refsignal_sl_dmrs_pssch_gen(srslte_refsignal_sl_t *q,
                                                    uint32_t nof_prb,
                                                    uint32_t n_PSSCH_ssf,
                                                    uint32_t n_X_ID,
                                                    cf_t *r_pssch);

SRSLTE_API int srslte_refsignal_sl_dmrs_pssch_gen_feron(srslte_refsignal_sl_t *q,
                                                        uint32_t nof_prb,
                                                        uint32_t n_PSSCH_ssf,
                                                        uint32_t n_X_ID,
                                                        cf_t *r_pssch);

SRSLTE_API int srslte_refsignal_sl_dmrs_pssch_gen_matlab(srslte_refsignal_sl_t *q,
                                                            uint32_t nof_prb,
                                                            uint32_t n_PSSCH_ssf,
                                                            uint32_t n_X_ID,
                                                            cf_t *r_pssch);


#endif // SRSLTE_REFSIGNAL_SL_H
