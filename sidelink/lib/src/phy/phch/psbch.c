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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <srslte/srslte.h>
#include <srslte/phy/common/phy_common.h>

#include "prb_dl.h"
#include "srslte/phy/phch/psbch.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/utils/bit.h"
#include "srslte/phy/utils/vector.h"
#include "srslte/phy/utils/debug.h"

// for the coding we take 7 sc-ofdm symobls into consideration, during
// mapping we only take the first six and discard the last one
#define PSBCH_RE_CP_NORM    (7 * 6 * SRSLTE_NRE)
// @todo: not sure about the length
#define PSBCH_RE_CP_EXT     0

#if 0


const uint8_t srslte_crc_mask[4][16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 } };

bool srslte_pbch_exists(int nframe, int nslot) {
  return (!(nframe % 5) && nslot == 1);
}
#endif
cf_t *offset_original_4;

int srslte_psbch_cp(cf_t *input, cf_t *output, srslte_cell_t cell, bool put) {
  int i;
  cf_t *ptr;
  
  offset_original_4 = input; 
  
  if (put) {
    ptr = input;
    output += cell.nof_prb * SRSLTE_NRE / 2 - 36;
  } else {
    ptr = output;
    input += cell.nof_prb * SRSLTE_NRE / 2 - 36;
  }

  for(i=0; i<2*SRSLTE_CP_NSYMB(cell.cp); i++) {
    // symbols containing psbch
    if(i==0 || i==3 || i==5 || i==7 || i==8 || i==10) {
      // printf("sym: %d from %x to %x\n", i, input, output);
      prb_cp(&input, &output, 6);
      if (put) {
        output += cell.nof_prb * SRSLTE_NRE - 2*36;
      } else {
        input += cell.nof_prb * SRSLTE_NRE - 2*36;
      }

    }else {
      if (put) {
        output += cell.nof_prb * SRSLTE_NRE;
      } else {
        input += cell.nof_prb * SRSLTE_NRE;
      }
    }

  }

  if (put) {
    return input - ptr;
  } else {
    return output - ptr;
  }
}

/**
 * Puts PSBCH in slot number 1
 *
 * Returns the number of symbols written to slot1_data
 *
 * 36.211 10.3 section 6.6.4
 *
 * @param[in] pbch PSBCH complex symbols to place in slot1_data
 * @param[out] psbch Complex symbol buffer for PSBCH subframe
 * @param[in] cell Cell configuration
 */
int srslte_psbch_put(cf_t *pbch, cf_t *psbch, srslte_cell_t cell) {
  return srslte_psbch_cp(pbch, psbch, cell, true);
}

/**
 * Extracts PSBCH from slot number 1
 *
 * Returns the number of symbols written to pbch
 *
 * 36.211 10.3 section 6.6.4
 *
 * @param[in] slot1_data Complex symbols for slot1
 * @param[out] pbch Extracted complex PBCH symbols
 * @param[in] cell Cell configuration
 */
int srslte_psbch_get(cf_t *slot1_data, cf_t *pbch, srslte_cell_t cell) {
  return srslte_psbch_cp(slot1_data, pbch, cell, false);
}


static void interleaver_table_gen(srslte_psbch_t *q) {
  uint32_t Qm = srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK);
  uint32_t H_prime_total = 1008 / Qm;// 7 psbch symbols * 72 RE per symbol
  uint32_t N_pusch_symbs = 2*(SRSLTE_CP_NORM_NSYMB - 2) - 3;

  uint32_t rows = H_prime_total/N_pusch_symbs;
  uint32_t cols = N_pusch_symbs;
  uint32_t idx = 0;
  for(uint32_t j=0; j<rows; j++) {        
    for(uint32_t i=0; i<cols; i++) {
      for(uint32_t k=0; k<Qm; k++) {
          // this is the indexing from SRSlte and which think is correct
          //q->interleaver_lut[j*Qm + i*rows*Qm + k] = idx;

          // this is the indexing from feron matlab implementation, which may be wrong
          q->interleaver_lut[idx] = j*Qm + i*rows*Qm + k;
          idx++;                  
      }
    }
  }
}


/** Initializes the PBCH transmitter and receiver. 
 * At the receiver, the field nof_ports in the cell structure indicates the 
 * maximum number of BS transmitter ports to look for.  
 */
int srslte_psbch_init(srslte_psbch_t *q) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL)
  {
    ret = SRSLTE_ERROR;

    bzero(q, sizeof(srslte_psbch_t));

    if (srslte_modem_table_lte(&q->mod, SRSLTE_MOD_QPSK)) {
      goto clean;
    }

    int poly[3] = { 0x6D, 0x4F, 0x57 };
    if (srslte_viterbi_init(&q->decoder, SRSLTE_VITERBI_37, poly, SRSLTE_SL_BCH_PAYLOADCRC_LEN/*40*/, true)) {
      goto clean;
    }

    if (srslte_crc_init(&q->crc, SRSLTE_LTE_CRC16, 16)) {
      goto clean;
    }

    q->encoder.K = 7;
    q->encoder.R = 3;
    q->encoder.tail_biting = true;
    memcpy(q->encoder.poly, poly, 3 * sizeof(int));

    q->nof_symbols = 7*6*SRSLTE_NRE; //PSBCH_RE_CP_NORM;
    q->nof_symbols_6 = 6*6*SRSLTE_NRE;

    q->d = srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
    if (!q->d) {
      goto clean;
    }
    int i;
    for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
      q->ce[i] = srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
      if (!q->ce[i]) {
        goto clean;
      }
      q->x[i] = srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
      if (!q->x[i]) {
        goto clean;
      }
      q->symbols[i] = srslte_vec_malloc(sizeof(cf_t) * q->nof_symbols);
      if (!q->symbols[i]) {
        goto clean;
      }
    }

    // q->llr = srslte_vec_malloc(sizeof(float) * q->nof_symbols * 4 * 2);
    // if (!q->llr) {
    //   goto clean;
    // }
    q->llr_s = srslte_vec_malloc(sizeof(short) * q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK));
    if (!q->llr_s) {
      goto clean;
    }
    q->g_s = srslte_vec_malloc(sizeof(short) * q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK));
    if (!q->g_s) {
      goto clean;
    }
    // q->temp = srslte_vec_malloc(sizeof(float) * q->nof_symbols * 4 * 2);
    // if (!q->temp) {
    //   goto clean;
    // }
    q->rm_b = srslte_vec_malloc(sizeof(float) * q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK));
    if (!q->rm_b) {
      goto clean;
    }
    // interleaver buffer, as we can not interlave in-place
    q->int_b = srslte_vec_malloc(sizeof(float) * q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK));
    if (!q->int_b) {
      goto clean;
    }

    // setup dft de-precoding
    if (srslte_dft_precoding_init(&q->dft_precoding, 6/*max_prb*/, false/*is_ue*/)) {
      fprintf(stderr, "Error initiating DFT transform precoding\n");
      goto clean; 
    }

    // setup scrambling sequence
    if (srslte_sequence_init(&q->seq, q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK))) {
      fprintf(stderr, "Error initiating scrambling sequence\n");
      goto clean;
    }

    // interleaver table
    // actually we could use the interleaver functionality from sch.c but because
    // we do not include any control information we only took the relevant part 
    // to genenerate a more lightweight version
    q->interleaver_lut = srslte_vec_malloc(sizeof(uint16_t) * q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK));
    if (!q->interleaver_lut) {
      goto clean;
    }
    interleaver_table_gen(q);




    ret = SRSLTE_SUCCESS;
  }
clean: 
  if (ret == SRSLTE_ERROR) {
    srslte_psbch_free(q);
  }
  return ret;
}

void srslte_psbch_free(srslte_psbch_t *q) {
  srslte_sequence_free(&q->seq);
  srslte_modem_table_free(&q->mod);
  srslte_viterbi_free(&q->decoder);
  srslte_dft_precoding_free(&q->dft_precoding);

  int i;
  for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
    if (q->ce[i]) {
      free(q->ce[i]);
    }
    if (q->x[i]) {
      free(q->x[i]);
    }
    if (q->symbols[i]) {
      free(q->symbols[i]);
    }
  }
  // if (q->llr) {
  //   free(q->llr);
  // }
  if (q->llr_s) {
    free(q->llr_s);
  }
  if (q->g_s) {
    free(q->g_s);
  }
  if (q->interleaver_lut) {
    free(q->interleaver_lut);
  }
  // if (q->temp) {
  //   free(q->temp);
  // }
  if (q->rm_b) {
    free(q->rm_b);
  }
  if (q->int_b) {
    free(q->int_b);
  }
  if (q->d) {
    free(q->d);
  }
  bzero(q, sizeof(srslte_psbch_t));
}

int srslte_psbch_set_cell(srslte_psbch_t *q, srslte_cell_t cell) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                       != NULL &&
      srslte_cell_isvalid(&cell))
  {
    if (cell.nof_ports == 0) {
      q->search_all_ports = true;
      cell.nof_ports = SRSLTE_MAX_PORTS;
    } else {
      q->search_all_ports = false;
    }

    if (q->cell.id != cell.id || q->cell.nof_prb == 0) {
      memcpy(&q->cell, &cell, sizeof(srslte_cell_t));

      // generate scrambling sequence for this cell id
      if (srslte_sequence_psbch(&q->seq, q->cell.id, q->nof_symbols * srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK))) {
        return SRSLTE_ERROR;
      }
    }
    //q->nof_symbols = (SRSLTE_CP_ISNORM(q->cell.cp)) ? PSBCH_RE_CP_NORM : PSBCH_RE_CP_EXT;

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}


/**
 * Unpacks MIB-SL from PSBCH message.
 * 
 * 36.331 v14.2.2 sec 6.5.1 and 6.5.2
 *
 * @param[in] msg PBCH in an unpacked bit array of size 24
 * @param[out] sfn System frame number
 * @param[out] directSubframeNumber_r12
 * @param[out] cell MIB information about PHICH and system bandwidth will be saved here
 */
void srslte_psbch_mib_unpack(uint8_t *msg, srslte_cell_t *cell, uint32_t *sfn, uint32_t *dsfn) {
  uint32_t bw_idx = srslte_bit_pack(&msg, 3);
  uint32_t sl_bandwidth_r12_nof_prb = 0;
  switch (bw_idx) {
  case 0:
    sl_bandwidth_r12_nof_prb = 6;
    break;
  case 1:
    sl_bandwidth_r12_nof_prb = 15;
    break;
  default:
    sl_bandwidth_r12_nof_prb = (bw_idx - 1) * 25;
    break;
  }

  // TDD-ConfigSL-r12
  uint8_t tdd_configsl_r12 = srslte_bit_pack(&msg, 3);

  uint16_t directFrameNumber_r12 = srslte_bit_pack(&msg, 10);
  uint8_t directSubframeNumber_r12 = srslte_bit_pack(&msg, 4);

  if(sfn) {
    *sfn = directFrameNumber_r12;
  }

  if(dsfn) {
    *dsfn = directSubframeNumber_r12;
  }

  uint8_t inCoverage_r12 = srslte_bit_pack(&msg, 1);

  INFO("MIB-SL: bw: %d tdd: %d dfn: %d dsfn: %d inc: %d\n",
          sl_bandwidth_r12_nof_prb,
          tdd_configsl_r12,
          directFrameNumber_r12,
          directSubframeNumber_r12,
          inCoverage_r12);

  cell->nof_prb = sl_bandwidth_r12_nof_prb;
  cell->nof_ports = 1;
}

/**
 * Packs MIB-SL to PSBCH message.
 *
 * @param[out] payload Output unpacked bit array of size SRSLTE_SL_BCH_PAYLOAD_LEN
 * @param[in] directFrameNumber_r12 System frame number
 * @param[in] directSubframeNumber_r12 subframe frame number
 * @param[in] cell Cell configuration to be encoded in MIB
 */
void srslte_psbch_mib_pack(srslte_cell_t *cell, uint32_t directFrameNumber_r12, uint32_t directSubframeNumber_r12, uint8_t *payload) {
  int bw;
  
  uint8_t *msg = payload; 

  bzero(msg, SRSLTE_SL_BCH_PAYLOAD_LEN);

  if (cell->nof_prb <= 6) {
    bw = 0;
  } else if (cell->nof_prb <= 15) {
    bw = 1;
  } else {
    bw = 1 + cell->nof_prb / 25;
  }

  // first bandwidth
  srslte_bit_unpack(bw, &msg, 3);
  // tdd_configsl_r12, hardcoded
  srslte_bit_unpack(0, &msg, 3);
  // directFrameNumber_r12
  srslte_bit_unpack(directFrameNumber_r12, &msg, 10);
  // directSubframeNumber_r12
  srslte_bit_unpack(directSubframeNumber_r12, &msg, 4);
  // inCoverage_r12, hardcoded
  srslte_bit_unpack(0, &msg, 1);
}


#if 0
void srslte_crc_set_mask(uint8_t *data, int nof_ports) {
  int i;
  for (i = 0; i < 16; i++) {
    data[SRSLTE_BCH_PAYLOAD_LEN + i] = (data[SRSLTE_BCH_PAYLOAD_LEN + i] + srslte_crc_mask[nof_ports - 1][i]) % 2;
  }
}

/* Checks CRC after applying the mask for the given number of ports.
 *
 * The bits buffer size must be at least 40 bytes.
 *
 * Returns 0 if the data is correct, -1 otherwise
 */
uint32_t srslte_pbch_crc_check(srslte_pbch_t *q, uint8_t *bits, uint32_t nof_ports) {
  uint8_t data[SRSLTE_BCH_PAYLOADCRC_LEN];
  memcpy(data, bits, SRSLTE_BCH_PAYLOADCRC_LEN * sizeof(uint8_t));
  srslte_crc_set_mask(data, nof_ports);
  int ret = srslte_crc_checksum(&q->crc, data, SRSLTE_BCH_PAYLOADCRC_LEN);
  if (ret == 0) {
    uint32_t chkzeros=0;
    for (int i=0;i<SRSLTE_BCH_PAYLOAD_LEN;i++) {
      chkzeros += data[i];
    }
    if (chkzeros) {
      return 0;
    } else {
      return SRSLTE_ERROR;
    }
  } else {
    return ret; 
  }
}

int decode_frame(srslte_pbch_t *q, uint32_t src, uint32_t dst, uint32_t n,
    uint32_t nof_bits, uint32_t nof_ports) {
  int j;

  if (dst + n <= 4 && src + n <= 4) {
    memcpy(&q->temp[dst * nof_bits], &q->llr[src * nof_bits],
           n * nof_bits * sizeof(float));

    /* descramble */
    srslte_scrambling_f_offset(&q->seq, &q->temp[dst * nof_bits], dst * nof_bits,
                               n * nof_bits);

    for (j = 0; j < dst * nof_bits; j++) {
      q->temp[j] = SRSLTE_RX_NULL;
    }
    for (j = (dst + n) * nof_bits; j < 4 * nof_bits; j++) {
      q->temp[j] = SRSLTE_RX_NULL;
    }

    /* unrate matching */
    srslte_rm_conv_rx(q->temp, 4 * nof_bits, q->rm_f, SRSLTE_BCH_ENCODED_LEN);

    /* Normalize LLR */
    srslte_vec_sc_prod_fff(q->rm_f, 1.0/((float) 2*n), q->rm_f, SRSLTE_BCH_ENCODED_LEN);

    /* decode */
    srslte_viterbi_decode_f(&q->decoder, q->rm_f, q->data, SRSLTE_BCH_PAYLOADCRC_LEN);

    if (!srslte_pbch_crc_check(q, q->data, nof_ports)) {
      return 1;
    } else {
      return SRSLTE_SUCCESS;
    }
  } else {
    fprintf(stderr, "Error in PBCH decoder: Invalid frame pointers dst=%d, src=%d, n=%d\n", src, dst, n);
    return -1;
  }

}
#endif


/**
 * @brief Decodes the PSBCH channel
 * 
 * @param q 
 * @param subframe_symbols 
 * @param ce 
 * @param noise_estimate 
 * @param bch_payload 
 * @return int                Returns 1 if successfully decoded MIB, 0 if not and -1 on error
 */
int srslte_psbch_decode(srslte_psbch_t *q, cf_t *subframe_symbols, cf_t *ce, float noise_estimate, 
                        uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN]) 
{
  int i;
  int nof_bits;
  cf_t *x[SRSLTE_MAX_LAYERS];
  
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  
  if (q                 != NULL &&
      ce                 != NULL &&
      subframe_symbols     != NULL)
  {
    // for (i=0;i<q->cell.nof_ports;i++) {
    //   if (ce_slot1[i] == NULL) {
    //     return SRSLTE_ERROR_INVALID_INPUTS;
    //   } 
    // }

    /* Set pointers for layermapping & precoding */
    nof_bits = 2 * q->nof_symbols;

    /* number of layers equals number of ports */
    for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
      x[i] = q->x[i];
    }
    
    //printf("Extraxt symbol from %x to %x\n", subframe_symbols, q->symbols[0]);
    /* extract symbols */
    if (q->nof_symbols_6 != srslte_psbch_get(subframe_symbols, q->symbols[0], q->cell)) {
      fprintf(stderr, "There was an error getting the PSBCH symbols\n");
      return SRSLTE_ERROR;
    }

    //printf("Extraxt channel from %x to %x\n", ce, q->ce[0]);
    /* extract channel estimates */
    if (q->nof_symbols_6 != srslte_psbch_get(ce, q->ce[0], q->cell)) {
      fprintf(stderr, "There was an error getting the PSBCH symbols\n");
      return SRSLTE_ERROR;
    }

    // Equalization
    srslte_predecoding_single(q->symbols[0], q->ce[0], q->d, NULL, q->nof_symbols, 1.0f, noise_estimate);
    
    // precoding according to 9.6.5 is identity operation

    // transform precoding 9.6.4
    srslte_dft_precoding(&q->dft_precoding, q->d, x[0],
                          6,//nof_prb
                          6);//nof_symbols

    // layer mapping is identy d = x

    // soft demodulation
    // @todo: during demod, we scale all llr with sqrt(2) which is not done in matlab implementation
    srslte_demod_soft_demodulate_s(SRSLTE_MOD_QPSK, x[0], q->llr_s, q->nof_symbols);

    // descrambe llr
    srslte_scrambling_s(&q->seq, q->llr_s);

    // we need to clear the llrs for the last symbol
    memset(&q->llr_s[q->nof_symbols_6*2], 0x00, 72*srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK)*sizeof(short));

    // deinterleaving
    srslte_vec_lut_sss(q->llr_s, q->interleaver_lut, q->g_s, nof_bits); //7 psbch symbols * 72 RE per symbol * 2 bit per iq
    
    //after rate-dematching the three streams d_i are interleaved
    // we are using the llr_s as temporal storage for our rm result
    srslte_rm_conv_rx_s(q->g_s, nof_bits, q->llr_s, SRSLTE_SL_BCH_ENCODED_LEN);

    // viterbi decoding
    srslte_viterbi_decode_s(&q->decoder, q->llr_s, q->data, SRSLTE_SL_BCH_PAYLOADCRC_LEN);

    ret = srslte_crc_checksum(&q->crc, q->data, SRSLTE_SL_BCH_PAYLOADCRC_LEN);

    //srslte_psbch_mib_unpack(q->data, &q->cell, NULL);

    if(ret == 0) {
      memcpy(bch_payload, q->data, SRSLTE_SL_BCH_PAYLOAD_LEN*sizeof(uint8_t));
      return 1;
    }
  }

  return SRSLTE_ERROR;
}

/**
 * @brief Encodes BCH and mappes them to the subframe
 * 
 * @param q 
 * @param bch_payload   Payload with onye byte per bit
 * @param sf_symbols    IQ-samples for the subframe, currently only one port is used
 * @return int 
 */
int srslte_psbch_encode(srslte_psbch_t *q, uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN], cf_t *sf_symbols[SRSLTE_MAX_PORTS]) {
  int i;
  int nof_bits;
  cf_t *x[SRSLTE_MAX_LAYERS];
  
  if (q                 != NULL &&
      bch_payload               != NULL)
  {
    for (i=0;i<q->cell.nof_ports;i++) {
      if (sf_symbols[i] == NULL) {
        return SRSLTE_ERROR_INVALID_INPUTS;
      } 
    }
    /* Set pointers for layermapping & precoding */
    nof_bits = 2 * q->nof_symbols;

    /* number of layers equals number of ports */
    for (i = 0; i < q->cell.nof_ports; i++) {
      x[i] = q->x[i];
    }
    memset(&x[q->cell.nof_ports], 0, sizeof(cf_t*) * (SRSLTE_MAX_LAYERS - q->cell.nof_ports));

    memcpy(q->data, bch_payload, sizeof(uint8_t) * SRSLTE_SL_BCH_PAYLOAD_LEN);

    /* encode & modulate */
    srslte_crc_attach(&q->crc, q->data, SRSLTE_SL_BCH_PAYLOAD_LEN);
    //srslte_crc_set_mask(q->data, q->cell.nof_ports);
    
    srslte_convcoder_encode(&q->encoder, q->data, q->data_enc, SRSLTE_SL_BCH_PAYLOADCRC_LEN);

    srslte_rm_conv_tx(q->data_enc, SRSLTE_SL_BCH_ENCODED_LEN, q->rm_b, nof_bits);

    // interleaving
    // @todo: implement a simd version a-la srslte_vec_lut_sss
    for(i=0; i< nof_bits; i++) {
      q->int_b[i] = q->rm_b[q->interleaver_lut[i]];
    }

    srslte_scrambling_b_offset(&q->seq, &q->int_b[0], 0, nof_bits);
    srslte_mod_modulate(&q->mod, &q->int_b[0], q->x[0], nof_bits);

    srslte_dft_precoding(&q->dft_precoding, q->x[0], q->d,
                          6,//nof_prb
                          6);//nof_symbols

    /* layer mapping & precoding */
    if (q->cell.nof_ports > 1) {
      srslte_layermap_diversity(q->d, x, q->cell.nof_ports, q->nof_symbols);
      srslte_precoding_diversity(x, q->symbols, q->cell.nof_ports,
                                 q->nof_symbols / q->cell.nof_ports, 1.0f);
    } else {
      memcpy(q->symbols[0], q->d, q->nof_symbols * sizeof(cf_t));
    }

    /* mapping to resource elements */
    for (i = 0; i < q->cell.nof_ports; i++) {
      srslte_psbch_put(q->symbols[i], sf_symbols[i], q->cell);
    }
    return SRSLTE_SUCCESS;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}
