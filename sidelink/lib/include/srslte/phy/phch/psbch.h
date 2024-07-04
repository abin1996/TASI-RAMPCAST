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
 *  File:         psbch.h
 *
 *  Description:  Physical sidelink broadcast channel.
 *
 *  Reference:    3GPP TS 36.211 version 14.2.0 Release 14 Sec. ?.?
 *****************************************************************************/

#ifndef SRSLTE_PSBCH_H
#define SRSLTE_PSBCH_H

#include "srslte/config.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/mimo/precoding.h"
#include "srslte/phy/mimo/layermap.h"
#include "srslte/phy/modem/mod.h"
#include "srslte/phy/modem/demod_soft.h"
#include "srslte/phy/scrambling/scrambling.h"
#include "srslte/phy/fec/rm_conv.h"
#include "srslte/phy/fec/convcoder.h"
#include "srslte/phy/fec/viterbi.h"
#include "srslte/phy/fec/crc.h"
#include "srslte/phy/dft/dft_precoding.h"

// check if we are sharing code
//#include "srslte/phy/phch/pbch.h"

#define SRSLTE_SL_BCH_PAYLOAD_LEN     40
#define SRSLTE_SL_BCH_PAYLOADCRC_LEN  (SRSLTE_SL_BCH_PAYLOAD_LEN+16)
#define SRSLTE_SL_BCH_ENCODED_LEN     3*(SRSLTE_SL_BCH_PAYLOADCRC_LEN)

/* PSBCH object */
typedef struct SRSLTE_API {
  srslte_cell_t cell;
  
  uint32_t nof_symbols;
  uint32_t nof_symbols_6;

  srslte_dft_precoding_t dft_precoding;

  /* buffers */
  cf_t *ce[SRSLTE_MAX_PORTS];
  cf_t *symbols[SRSLTE_MAX_PORTS];
  cf_t *x[SRSLTE_MAX_PORTS];
  cf_t *d;

  //float *llr;
  short *llr_s;
  short *g_s;
  //float *temp;
  //float rm_f[SRSLTE_BCH_ENCODED_LEN];
  uint8_t *rm_b;
  uint8_t *int_b;
  uint8_t data[SRSLTE_SL_BCH_PAYLOADCRC_LEN];
  uint8_t data_enc[SRSLTE_SL_BCH_ENCODED_LEN];

  //uint32_t frame_idx;

  // for channel interleaving without control data
  uint16_t *interleaver_lut;

  /* tx & rx objects */
  srslte_modem_table_t mod;
  srslte_sequence_t seq;
  srslte_viterbi_t decoder;
  srslte_crc_t crc;
  srslte_convcoder_t encoder;
  
  // @todo: review this variable if it is applicable to sidelink
  bool search_all_ports;
  
} srslte_psbch_t;


SRSLTE_API int srslte_psbch_init(srslte_psbch_t *q);

SRSLTE_API void srslte_psbch_free(srslte_psbch_t *q);

SRSLTE_API int srslte_psbch_set_cell(srslte_psbch_t *q, srslte_cell_t cell);

SRSLTE_API int srslte_psbch_encode(srslte_psbch_t *q,
                                    uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN],
                                    cf_t *sf_symbols[SRSLTE_MAX_PORTS]);

//@otod: review if all parameters are required for sidelink
SRSLTE_API int srslte_psbch_decode(srslte_psbch_t *q,
                                    cf_t *subframe_symbols,
                                    cf_t *ce,
                                    float noise_estimate,
                                    uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN]);

SRSLTE_API void srslte_psbch_mib_unpack(uint8_t *msg,
                                        srslte_cell_t *cell,
                                        uint32_t *sfn,
                                        uint32_t *dsfn);

SRSLTE_API void srslte_psbch_mib_pack(srslte_cell_t *cell,
                                      uint32_t directFrameNumber_r12,
                                      uint32_t directSubframeNumber_r12,
                                      uint8_t *payload);

#endif // SRSLTE_PSBCH_H
