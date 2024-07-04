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
 *  File:         ue_mib.h
 *
 *  Description:  This object decodes the MIB from the PBCH of an LTE signal.
 *
 *                The function srslte_ue_mib_decode() shall be called multiple
 *                times, each passing a number of samples multiple of 19200,
 *                sampled at 1.92 MHz (that is, 10 ms of samples).
 *
 *                The function uses the sync_t object to find the PSS sequence and
 *                decode the PBCH to obtain the MIB.
 * 
 *                The function returns 0 until the MIB is decoded.
 *
 *                See ue_cell_detect.c for an example.
 *
 *  Reference:
 *****************************************************************************/

#ifndef SRSLTE_UE_SL_MIB_H
#define SRSLTE_UE_SL_MIB_H

#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/phy/ue_sl/ue_sl_sync.h"
#include "srslte/phy/sync/cfo.h"
#include "srslte/phy/ch_estimation/chest_sl.h"
#include "srslte/phy/phch/psbch.h"
#include "srslte/phy/phch/pscch.h"
#include "srslte/phy/phch/pssch.h"
#include "srslte/phy/dft/ofdm.h"
#include "srslte/phy/phch/repo.h"


#define SRSLTE_UE_SL_MIB_NOF_PRB              6

#define SRSLTE_UE_SL_MIB_FOUND                1
#define SRSLTE_UE_SL_MIB_NOTFOUND             0

typedef struct SRSLTE_API {
  srslte_sync_t sfind;
 
  cf_t *sf_symbols;
  cf_t *ce;

  cf_t *ce_plot;
  cf_t *td_plot;

  srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS];
  
  srslte_ofdm_t fft;
  srslte_chest_sl_t     chest; 
  srslte_chest_sl_res_t chest_res;
  
  srslte_psbch_t psbch;
  srslte_pscch_t pscch;
  srslte_pssch_t pssch;
  
  uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN];
  uint32_t nof_tx_ports; 
  uint32_t sfn_offset; 
  
  uint32_t frame_cnt; 
} srslte_ue_sl_mib_t;

SRSLTE_API int srslte_ue_sl_mib_init(srslte_ue_sl_mib_t *q, 
                                  cf_t *in_buffer[SRSLTE_MAX_PORTS],
                                  uint32_t max_prb);

SRSLTE_API void srslte_ue_sl_mib_free(srslte_ue_sl_mib_t *q);

SRSLTE_API int srslte_ue_sl_mib_set_cell(srslte_ue_sl_mib_t * q,
                                      srslte_cell_t cell);

SRSLTE_API void srslte_ue_sl_mib_reset(srslte_ue_sl_mib_t * q); 

SRSLTE_API int srslte_ue_sl_mib_decode(srslte_ue_sl_mib_t * q, 
                                    uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN], 
                                    uint32_t *nof_tx_ports, 
                                    int *sfn_offset); 

/* This interface uses ue_mib and ue_sync to first get synchronized subframes 
 * and then decode MIB
 * 
 * This object calls the pbch object with nof_ports=0 for blind nof_ports determination 
*/
typedef struct {
  srslte_ue_sl_mib_t ue_sl_mib; 
  srslte_ue_sl_sync_t ue_sl_sync; 
  cf_t *sf_buffer[SRSLTE_MAX_PORTS];
  uint32_t nof_rx_antennas;
} srslte_ue_sl_mib_sync_t;

SRSLTE_API int srslte_ue_sl_mib_sync_init_multi(srslte_ue_sl_mib_sync_t *q,
                                             int (recv_callback)(void*, cf_t*[SRSLTE_MAX_PORTS], uint32_t, srslte_timestamp_t *),
                                             uint32_t nof_rx_antennas,
                                             void *stream_handler);

SRSLTE_API void srslte_ue_sl_mib_sync_free(srslte_ue_sl_mib_sync_t *q);

SRSLTE_API int srslte_ue_sl_mib_sync_set_cell(srslte_ue_sl_mib_sync_t *q,
                                              uint32_t cell_id,
                                              srslte_cp_t cp);

SRSLTE_API void srslte_ue_sl_mib_sync_reset(srslte_ue_sl_mib_sync_t * q); 

SRSLTE_API int srslte_ue_sl_mib_sync_decode(srslte_ue_sl_mib_sync_t * q, 
                                            uint32_t max_frames_timeout,
                                            uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN], 
                                            uint32_t *nof_tx_ports, 
                                            int *sfn_offset); 



SRSLTE_API int srslte_ue_sl_pscch_decode(srslte_ue_sl_mib_t * q,
                                          srslte_repo_t *repo,
                                          uint8_t *decoded,
                                          int *n_decoded_bytes);

SRSLTE_API int srslte_ue_sl_pssch_decode(srslte_ue_sl_mib_t * q,
                                          srslte_repo_t *repo,
                                          uint8_t *sci_bits,
                                          uint8_t *decoded,
                                          int *n_decoded_bytes);

#endif // SRSLTE_UE_SL_MIB_H

