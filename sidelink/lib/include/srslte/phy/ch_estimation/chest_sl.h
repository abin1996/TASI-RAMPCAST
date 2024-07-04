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
 *  File:         chest_sl.h
 *
 *  Description:  3GPP LTE Sidelink channel estimator and equalizer.
 *                Estimates the channel in the resource elements transmitting references and
 *                interpolates for the rest of the resource grid.
 *                The equalizer uses the channel estimates to produce an estimation of the
 *                transmitted symbol.
 *
 *  Reference:
 *********************************************************************************************/

#ifndef SRSLTE_CHEST_SL_H
#define SRSLTE_CHEST_SL_H

#include <stdio.h>

#include "srslte/config.h"

#include "srslte/phy/ch_estimation/chest_common.h"
#include "srslte/phy/resampling/interp.h"
#include "srslte/phy/ch_estimation/refsignal_ul.h"
#include "srslte/phy/ch_estimation/refsignal_sl.h"
#include "srslte/phy/common/phy_common.h"

// @todo: update with required fields
typedef struct SRSLTE_API {
  // cf_t*    ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];
  // uint32_t nof_re;
  // float    noise_estimate;
  // float    noise_estimate_dbm;
  float    snr_db;
  // float    snr_ant_port_db[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];
  // float    rsrp;
  // float    rsrp_dbm;
  // float    rsrp_neigh;
  // float    rsrp_port_dbm[SRSLTE_MAX_PORTS];
  // float    rsrp_ant_port_dbm[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];
  // float    rsrq;
  // float    rsrq_db;
  // float    rsrq_ant_port_db[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];
  // float    rssi_dbm;
  // float    cfo;
  // float    sync_error;
} srslte_chest_sl_res_t;

// @todo: check which member can be removed from sidelink implementation
typedef struct {
  srslte_cell_t cell; 
  
  srslte_refsignal_sl_t             dmrs_signal;
  srslte_refsignal_ul_dmrs_pregen_t dmrs_pregen; 
  bool dmrs_signal_configured; 
  
  cf_t *pilot_estimates;
  cf_t *pilot_estimates_tmp[4];
  cf_t *pilot_recv_signal; 
  cf_t *pilot_known_signal; 
  cf_t *tmp_noise; 
  
#ifdef FREQ_SEL_SNR  
  float snr_vector[12000];
  float pilot_power[12000];
#endif
  uint32_t smooth_filter_len; 
  float smooth_filter[SRSLTE_CHEST_MAX_SMOOTH_FIL_LEN];

  srslte_interp_linsrslte_vec_t srslte_interp_linvec; 
  
  float s_rsrp[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS]; 
  float rsrp_corr[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS]; 
  float pilot_power; 
  float noise_estimate;
  float ce_power_estimate;

  bool     rsrp_neighbour; 
} srslte_chest_sl_t;

SRSLTE_API int srslte_chest_sl_init(srslte_chest_sl_t *q, 
                                    uint32_t max_prb);

SRSLTE_API int srslte_chest_sl_estimate_psbch(srslte_chest_sl_t *q,
                                              cf_t *input,
                                              cf_t *ce,
                                              srslte_sl_mode_t sl_mode,
                                              uint32_t port_id,
                                              uint32_t rxant_id); 

SRSLTE_API int srslte_chest_sl_estimate_pscch(srslte_chest_sl_t *q,
                                              cf_t *input,
                                              cf_t *ce,
                                              srslte_sl_mode_t sl_mode,
                                              uint32_t prb_offset);

SRSLTE_API int srslte_chest_sl_estimate_pssch(srslte_chest_sl_t *q,
                                              cf_t *input,
                                              cf_t *ce,
                                              srslte_sl_mode_t sl_mode,
                                              uint32_t prb_offset,
                                              uint32_t prb_n);

SRSLTE_API void srslte_chest_sl_free(srslte_chest_sl_t *q);

SRSLTE_API int srslte_chest_sl_set_cell(srslte_chest_sl_t *q,
                                        srslte_cell_t cell);

#if 0

SRSLTE_API void srslte_chest_ul_set_cfg(srslte_chest_ul_t *q, 
                                        srslte_refsignal_dmrs_pusch_cfg_t *pusch_cfg,
                                        srslte_pucch_cfg_t *pucch_cfg, 
                                        srslte_refsignal_srs_cfg_t *srs_cfg);

SRSLTE_API void srslte_chest_ul_set_smooth_filter(srslte_chest_ul_t *q, 
                                                  float *filter, 
                                                  uint32_t filter_len);
#endif

SRSLTE_API void srslte_chest_sl_set_smooth_filter3_coeff(srslte_chest_sl_t* q, 
                                                         float w); 

#if 0
SRSLTE_API int srslte_chest_ul_estimate(srslte_chest_ul_t *q, 
                                        cf_t *input, 
                                        cf_t *ce, 
                                        uint32_t nof_prb, 
                                        uint32_t sf_idx, 
                                        uint32_t cyclic_shift_for_dmrs, 
                                        uint32_t n_prb[2]);

SRSLTE_API int srslte_chest_ul_estimate_pucch(srslte_chest_ul_t *q, 
                                              cf_t *input, 
                                              cf_t *ce, 
                                              srslte_pucch_format_t format, 
                                              uint32_t n_pucch, 
                                              uint32_t sf_idx, 
                                              uint8_t *pucch2_ack_bits); 
#endif

SRSLTE_API float srslte_chest_sl_get_noise_estimate(srslte_chest_sl_t *q); 

SRSLTE_API float srslte_chest_sl_get_snr(srslte_chest_sl_t *q);

SRSLTE_API float srslte_chest_sl_get_snr_db(srslte_chest_sl_t *q);


#endif // SRSLTE_CHEST_SL_H
