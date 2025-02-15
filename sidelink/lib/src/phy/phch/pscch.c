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
#include <math.h>

#include "prb_dl.h"
#include "srslte/phy/phch/dci.h"
#include "srslte/phy/phch/regs.h"
#include "srslte/phy/phch/pscch.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/utils/bit.h"
#include "srslte/phy/utils/vector.h"
#include "srslte/phy/utils/debug.h"

// #define PDCCH_NOF_FORMATS               4
// #define PDCCH_FORMAT_NOF_CCE(i)         (1<<i)
// #define PDCCH_FORMAT_NOF_REGS(i)        ((1<<i)*9)
// #define PDCCH_FORMAT_NOF_BITS(i)        ((1<<i)*72)

// #define NOF_CCE(cfi)  ((cfi>0&&cfi<4)?q->nof_cce[cfi-1]:0)
// #define NOF_REGS(cfi) ((cfi>0&&cfi<4)?q->nof_regs[cfi-1]:0)

// float srslte_pscch_coderate(uint32_t nof_bits, uint32_t l) {
//   return (float) (nof_bits+16)/(4*PDCCH_FORMAT_NOF_REGS(l));
// }


static void interleaver_table_gen(srslte_pscch_t *q) {
  uint32_t Qm = srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK);
  uint32_t H_prime_total = q->max_bits / Qm;
  uint32_t N_pucch_symbs = 2*(SRSLTE_CP_NORM_NSYMB - 2); // 36.212 5.4.3

  uint32_t rows = H_prime_total/N_pucch_symbs;
  uint32_t cols = N_pucch_symbs;
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


/** Initializes the PSCCH transmitter and receiver */
static int pscch_init(srslte_pscch_t *q, uint32_t max_prb, uint32_t nof_rx_antennas, bool is_ue)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL)
  {   
    ret = SRSLTE_ERROR;
    bzero(q, sizeof(srslte_pscch_t));
    q->nof_rx_antennas = nof_rx_antennas;
    q->is_ue           = is_ue;
    /* Allocate memory for the maximum number of PDCCH bits (CFI=3) */
    // 10 SC_FDMA symbols * 2 PRB/sym * 12 RE/PRB * 2 bit/RE
    q->max_bits = 10*2*SRSLTE_NRE*srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK);//max_prb*3*12*2;
    
    INFO("Init PSCCH: Max bits: %d\n", q->max_bits);

    if (srslte_modem_table_lte(&q->mod, SRSLTE_MOD_QPSK)) {
      goto clean;
    }
    if (srslte_crc_init(&q->crc, SRSLTE_LTE_CRC16, 16)) {
      goto clean;
    }

    int poly[3] = { 0x6D, 0x4F, 0x57 };
    if (srslte_viterbi_init(&q->decoder, SRSLTE_VITERBI_37, poly, SRSLTE_SCI1_MAX_BITS + 16, true)) {
      goto clean;
    }

    q->e = srslte_vec_malloc(sizeof(uint8_t) * q->max_bits);
    if (!q->e) {
      goto clean;
    }

    q->e_int_tmp = srslte_vec_malloc(sizeof(uint8_t) * q->max_bits);
    if (!q->e_int_tmp) {
      goto clean;
    }

    q->llr = srslte_vec_malloc(sizeof(float) * q->max_bits);
    if (!q->llr) {
      goto clean;
    }
    
    q->f_f = srslte_vec_malloc(sizeof(float) * q->max_bits);
    if (!q->f_f) {
      goto clean;
    }
    
    bzero(q->llr, sizeof(float) * q->max_bits);

    q->d = srslte_vec_malloc(sizeof(cf_t) * q->max_bits / 2);
    if (!q->d) {
      goto clean;
    }

    for (int i = 0; i < SRSLTE_MAX_PORTS; i++) {
      q->x[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_bits / 2);
      if (!q->x[i]) {
        goto clean;
      }
      q->symbols[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_bits / 2);
      if (!q->symbols[i]) {
        goto clean;
      }
      if (q->is_ue) {
        for (int j = 0; j < q->nof_rx_antennas; j++) {
          q->ce[i][j] = srslte_vec_malloc(sizeof(cf_t) * q->max_bits / 2);
          if (!q->ce[i][j]) {
            goto clean;
          }
        }
      }
    }

    if (srslte_sequence_pscch(&q->seq, q->max_bits)) {
      goto clean;
    }

    // interleaver table
    // actually we could use the interleaver functionality from sch.c but because
    // we do not include any control information we only took the relevant part 
    // to genenerate a more lightweight version
    q->interleaver_lut = srslte_vec_malloc(sizeof(uint16_t) * q->max_bits);
    if (!q->interleaver_lut) {
      goto clean;
    }
    interleaver_table_gen(q);

    // setup dft de-precoding
    if (srslte_dft_precoding_init(&q->dft_deprecoding, 2/*max_prb*/, false/*is_ue*/)) {
      fprintf(stderr, "Error initiating DFT transform precoding\n");
      goto clean; 
    }

    // setup dft precoding
    if (srslte_dft_precoding_init(&q->dft_precoding, 2/*max_prb*/, true/*is_ue*/)) {
      fprintf(stderr, "Error initiating DFT transform precoding\n");
      goto clean; 
    }

    ret = SRSLTE_SUCCESS;
  }
  clean: 
  if (ret == SRSLTE_ERROR) {
    srslte_pscch_free(q);
  }
  return ret;
}

int srslte_pscch_init_enb(srslte_pscch_t *q, uint32_t max_prb) {
  return pscch_init(q, max_prb, 0, false);
}

int srslte_pscch_init_ue(srslte_pscch_t *q, uint32_t max_prb, uint32_t nof_rx_antennas) {
  return pscch_init(q, max_prb, nof_rx_antennas, true);
}

void srslte_pscch_free(srslte_pscch_t *q) {

  if (q->e) {
    free(q->e);
  }
  if (q->e_int_tmp) {
    free(q->e_int_tmp);
  }
  if (q->llr) {
    free(q->llr);
  }
  if (q->f_f) {
    free(q->f_f);
  }
  if (q->d) {
    free(q->d);
  }
  for (int i = 0; i < SRSLTE_MAX_PORTS; i++) {
    if (q->x[i]) {
      free(q->x[i]);
    }
    if (q->symbols[i]) {
      free(q->symbols[i]);
    }
    if (q->is_ue) {
      for (int j=0;j<q->nof_rx_antennas;j++) {
        if (q->ce[i][j]) {
          free(q->ce[i][j]);
        }
      }
    }
  }
  srslte_sequence_free(&q->seq);

  srslte_modem_table_free(&q->mod);
  srslte_viterbi_free(&q->decoder);

  if (q->interleaver_lut) {
    free(q->interleaver_lut);
  }

  srslte_dft_precoding_free(&q->dft_precoding);
  srslte_dft_precoding_free(&q->dft_deprecoding);

  bzero(q, sizeof(srslte_pscch_t));

}

int srslte_pscch_set_cell(srslte_pscch_t *q, srslte_cell_t cell)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                         != NULL &&
      srslte_cell_isvalid(&cell))
  {
    // q->regs = regs;

    // for (int cfi=0;cfi<3;cfi++) {
    //   q->nof_regs[cfi] = (srslte_regs_pdcch_nregs(q->regs, cfi+1) / 9) * 9;
    //   q->nof_cce[cfi]  = q->nof_regs[cfi]/ 9;
    // }

    /* Allocate memory for the maximum number of PDCCH bits (CFI=3) */
    // q->max_bits = (NOF_REGS(3)/ 9) * 72;

    INFO("PSCCH: Cell config PCI=%d, %d ports.\n",
         q->cell.id, q->cell.nof_ports);

    if (q->cell.id != cell.id || q->cell.nof_prb == 0) {
      memcpy(&q->cell, &cell, sizeof(srslte_cell_t));
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

#if 0
uint32_t srslte_pscch_ue_locations(srslte_pscch_t *q, srslte_dci_location_t *c, uint32_t max_candidates,
                        uint32_t nsubframe, uint32_t cfi, uint16_t rnti) 
{
  return srslte_pscch_ue_locations_ncce(NOF_CCE(cfi), c, max_candidates, nsubframe, rnti);
}


uint32_t srslte_pscch_ue_locations_ncce(uint32_t nof_cce, srslte_dci_location_t *c, uint32_t max_candidates,
                                        uint32_t nsubframe, uint16_t rnti)
{
  return srslte_pscch_ue_locations_ncce_L(nof_cce, c, max_candidates, nsubframe, rnti, -1);
}

/** 36.213 v9.1.1 
 * Computes up to max_candidates UE-specific candidates for DCI messages and saves them 
 * in the structure pointed by c.
 * Returns the number of candidates saved in the array c.   
 */
uint32_t srslte_pscch_ue_locations_ncce_L(uint32_t nof_cce, srslte_dci_location_t *c, uint32_t max_candidates,
                        uint32_t nsubframe, uint16_t rnti, int Ls) {
  
  int l; // this must be int because of the for(;;--) loop
  uint32_t i, k, L, m; 
  uint32_t Yk, ncce;
  const int nof_candidates[4] = { 6, 6, 2, 2};

  // Compute Yk for this subframe
  Yk = rnti;
  for (m = 0; m < nsubframe+1; m++) {
    Yk = (39827 * Yk) % 65537;
  }

  k = 0;
  // All aggregation levels from 8 to 1
  for (l = 3; l >= 0; l--) {
    L = (1 << l);
    if (Ls<0 || Ls==L) {
      // For all candidates as given in table 9.1.1-1
      for (i = 0; i < nof_candidates[l]; i++) {
        if (nof_cce >= L) {
          ncce = L * ((Yk + i) % (nof_cce / L));
          // Check if candidate fits in c vector and in CCE region
          if (k < max_candidates  && ncce + L <= nof_cce)
          {
            c[k].L = l;
            c[k].ncce = ncce;

            DEBUG("UE-specific SS Candidate %d: nCCE: %d, L: %d\n",
                  k, c[k].ncce, c[k].L);

            k++;
          }
        }
      }
    }
  }

  DEBUG("Initiated %d candidate(s) in the UE-specific search space for C-RNTI: 0x%x, nsubframe=%d, nof_cce=%d\n", 
         k, rnti, nsubframe, nof_cce);
  
  return k; 
}



/**
 * 36.213 9.1.1
 * Computes up to max_candidates candidates in the common search space 
 * for DCI messages and saves them in the structure pointed by c.  
 * Returns the number of candidates saved in the array c.   
 */
uint32_t srslte_pscch_common_locations(srslte_pscch_t *q, srslte_dci_location_t *c, uint32_t max_candidates, 
                                uint32_t cfi) 
{
  return srslte_pscch_common_locations_ncce(NOF_CCE(cfi), c, max_candidates); 
}

uint32_t srslte_pscch_common_locations_ncce(uint32_t nof_cce, srslte_dci_location_t *c, uint32_t max_candidates) 
{
  uint32_t i, l, L, k;

  k = 0;
  for (l = 3; l > 1; l--) {
    L = (1 << l);
    for (i = 0; i < SRSLTE_MIN(nof_cce, 16) / (L); i++) {
      uint32_t ncce = (L) * (i % (nof_cce / (L)));
      if (k < max_candidates && ncce + L <= nof_cce) {
        c[k].L    = l;
        c[k].ncce = ncce;
        DEBUG("Common SS Candidate %d: nCCE: %d, L: %d\n",
            k, c[k].ncce, c[k].L);
        k++;
      }
    }
  }
  
  INFO("Initiated %d candidate(s) in the Common search space\n", k);
  
  return k;
}
#endif





/** 36.212 5.3.3.2 to 5.3.3.4
 *
 * Returns XOR between parity and remainder bits
 *
 * TODO: UE transmit antenna selection CRC mask
 */
int srslte_pscch_dci_decode(srslte_pscch_t *q, float *e, uint8_t *data, uint32_t E, uint32_t nof_bits, uint16_t *crc) {

  uint16_t p_bits, crc_res;
  uint8_t *x;

  if (q           != NULL) {
    if (data      != NULL         &&
        E         <= q->max_bits   && 
        nof_bits  <= SRSLTE_SCI1_MAX_BITS)
    {
      bzero(q->rm_f, sizeof(float)*3 * (SRSLTE_SCI1_MAX_BITS + 16));
      
      uint32_t coded_len = 3 * (nof_bits + 16); 


      for(int i=0; i< E; i++) {
        q->f_f[i] = e[q->interleaver_lut[i]];
      }
      
      /* unrate matching */
      srslte_rm_conv_rx(q->f_f, E, q->rm_f, coded_len);
      
      /* viterbi decoder */
      srslte_viterbi_decode_f(&q->decoder, q->rm_f, data, nof_bits + 16);

      x = &data[nof_bits];
      p_bits = (uint16_t) srslte_bit_pack(&x, 16);
      crc_res = ((uint16_t) srslte_crc_checksum(&q->crc, data, nof_bits) & 0xffff);

      if (crc) {
        *crc = p_bits;
      }

      if(crc_res != p_bits) {
        return SRSLTE_ERROR;
      }

      return SRSLTE_SUCCESS;
    } else {
      fprintf(stderr, "Invalid parameters: E: %d, max_bits: %d, nof_bits: %d\n", E, q->max_bits, nof_bits);
      return SRSLTE_ERROR_INVALID_INPUTS;      
    }
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;          
  }
}
#if 0
/** Tries to decode a DCI message from the LLRs stored in the srslte_pscch_t structure by the function 
 * srslte_pscch_extract_llr(). This function can be called multiple times. 
 * The decoded message is stored in msg and the CRC remainder in crc_rem pointer
 * 
 */
int srslte_pscch_decode_msg(srslte_pscch_t *q, 
                            srslte_dci_msg_t *msg, 
                            srslte_dci_location_t *location, 
                            srslte_dci_format_t format,
                            uint32_t cfi,
                            uint16_t *crc_rem) 
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if (q                 != NULL       && 
      msg               != NULL       && 
      srslte_dci_location_isvalid(location))
  {
    if (location->ncce * 72 + PDCCH_FORMAT_NOF_BITS(location->L) > 
      NOF_CCE(cfi)*72) {
      fprintf(stderr, "Invalid location: nCCE: %d, L: %d, NofCCE: %d\n", 
        location->ncce, location->L, NOF_CCE(cfi));
    } else {
      ret = SRSLTE_SUCCESS;
      
      uint32_t nof_bits = srslte_dci_format_sizeof(format, q->cell.nof_prb, q->cell.nof_ports);
      uint32_t e_bits = PDCCH_FORMAT_NOF_BITS(location->L);
    
      double mean = 0; 
      for (int i=0;i<e_bits;i++) {
        mean += fabsf(q->llr[location->ncce * 72 + i]);
      }
      mean /= e_bits; 
      if (mean > 0.5) {
        ret = srslte_pscch_dci_decode(q, &q->llr[location->ncce * 72], 
                        msg->data, e_bits, nof_bits, crc_rem);
        if (ret == SRSLTE_SUCCESS) {
          msg->nof_bits = nof_bits;
          // Check format differentiation 
          if (format == SRSLTE_DCI_FORMAT0 || format == SRSLTE_DCI_FORMAT1A) {
            msg->format = (msg->data[0] == 0)?SRSLTE_DCI_FORMAT0:SRSLTE_DCI_FORMAT1A;
          } else {
            msg->format   = format; 
          }
        } else {
          fprintf(stderr, "Error calling pdcch_dci_decode\n");
        }
        if (crc_rem) {
          DEBUG("Decoded DCI: nCCE=%d, L=%d, format=%s, msg_len=%d, mean=%f, crc_rem=0x%x\n", 
            location->ncce, location->L, srslte_dci_format_string(format), nof_bits, mean, *crc_rem);
        }
      } else {
        DEBUG("Skipping DCI:  nCCE=%d, L=%d, msg_len=%d, mean=%f\n",
              location->ncce, location->L, nof_bits, mean);        
      }
    }
  } else {
    fprintf(stderr, "Invalid parameters, location=%d,%d\n", location->ncce, location->L);
  }
  return ret;
}
#endif

cf_t *offset_original_5;
static int srslte_pscch_cp(cf_t *input, cf_t *output, srslte_cell_t cell, uint32_t prb_offset, uint32_t n_prb, bool put) {
  int i;
  cf_t *ptr;
  
  offset_original_5 = input; 
  
  // adjust in/out pointer to account for prb_offset
  if (put) {
    ptr = input;
    output += prb_offset * SRSLTE_NRE;
  } else {
    ptr = output;
    input += prb_offset * SRSLTE_NRE;
  }

  // exclude last symbol as it is used as guard
  for(i=0; i<2*SRSLTE_CP_NSYMB(cell.cp)-1; i++) {
    // symbols not containing pscch
    if(i==2 || i==5 || i==8 || i==11) {
      if (put) {
        output += cell.nof_prb * SRSLTE_NRE;
      } else {
        input += cell.nof_prb * SRSLTE_NRE;
      }
    } else {
      // printf("sym: %d from %x to %x\n", i, input, output);
      prb_cp(&input, &output, n_prb);
      if (put) {
        output += cell.nof_prb * SRSLTE_NRE - n_prb*SRSLTE_NRE;
      } else {
        input += cell.nof_prb * SRSLTE_NRE - n_prb*SRSLTE_NRE;
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
 * @brief Puts PSCCH/PSSCH symbols into the subframe
 * 
 * @param pscch       pscch iq samples
 * @param subframe    subframe iq samples
 * @param cell        cell pararmeter
 * @param prb_offset  PRB offset
 * @param n_prb       number of PRBs to the channel occupies
 * @return int        returns the number of symbols written to the subframe
 */
int srslte_pscch_put(cf_t *pscch, cf_t *subframe, srslte_cell_t cell, uint32_t prb_offset, uint32_t n_prb) {
  return srslte_pscch_cp(pscch, subframe, cell, prb_offset, n_prb, true);
}

/**
 * Extracts PSBCH from slot number 1
 *
 * Returns the number of symbols written to pscch
 *
 *
 * @param[in] subframe Complex symbols for subframe
 * @param[out] pscch Extracted complex PSCCH symbols
 * @param[in] cell Cell configuration
 * @param[in] prb_offset PRB offset where pscch starts
 */
int srslte_pscch_get(cf_t *subframe, cf_t *pscch, srslte_cell_t cell, uint32_t prb_offset, uint32_t n_prb) {
  return srslte_pscch_cp(subframe, pscch, cell, prb_offset, n_prb, false);
}


int srslte_pscch_extract_llr(srslte_pscch_t *q, cf_t *sf_symbols, cf_t *ce[SRSLTE_MAX_PORTS], float noise_estimate, 
                      uint32_t nsubframe, uint32_t prb_offset) 
{
  cf_t *_sf_symbols[SRSLTE_MAX_PORTS]; 
  cf_t *_ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];
  
  _sf_symbols[0] = sf_symbols; 
  for (int i=0;i<q->cell.nof_ports;i++) {
    _ce[i][0] = ce[i]; 
  }
  return srslte_pscch_extract_llr_multi(q, _sf_symbols, _ce, noise_estimate, nsubframe, prb_offset);
}

/** Extracts the LLRs from srslte_dci_location_t location of the subframe and stores them in the srslte_pscch_t structure. 
 * DCI messages can be extracted from this location calling the function srslte_pscch_decode_msg(). 
 * Every time this function is called (with a different location), the last demodulated symbols are overwritten and
 * new messages from other locations can be decoded 
 */
int srslte_pscch_extract_llr_multi(srslte_pscch_t *q, cf_t *sf_symbols[SRSLTE_MAX_PORTS], cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS], float noise_estimate, 
                                   uint32_t nsubframe, uint32_t prb_offset) 
{

  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  
  /* Set pointers for layermapping & precoding */
  uint32_t i, nof_symbols;
  cf_t *x[SRSLTE_MAX_LAYERS];

  const uint32_t n_prb = 2;

  if (q                 != NULL && 
      nsubframe         <  10)
  {
    // account for last symbol which is used as guard, and therefor has no valueable llrs for us
    uint32_t e_bits = q->max_bits - n_prb*SRSLTE_NRE*srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK);//72*NOF_CCE(cfi); 10*2*SRSLTE_NRE*srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK)
    nof_symbols = e_bits / srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK);
    ret = SRSLTE_ERROR;
    bzero(q->llr, sizeof(float) * q->max_bits);
    
    DEBUG("Extracting LLRs: E: %d, SF: %d, n_PRB: %d\n",
        e_bits, nsubframe, prb_offset);

    /* number of layers equals number of ports */
    for (i = 0; i < q->cell.nof_ports; i++) {
      x[i] = q->x[i];
    }
    memset(&x[q->cell.nof_ports], 0, sizeof(cf_t*) * (SRSLTE_MAX_LAYERS - q->cell.nof_ports));
          
    /* extract symbols */
    for (int j=0;j<q->nof_rx_antennas;j++) {
      //int n = srslte_regs_pdcch_get(q->regs, cfi, sf_symbols[j], q->symbols[j]);
      int n = srslte_pscch_get(sf_symbols[j], q->symbols[j], q->cell, prb_offset, n_prb);
      if (nof_symbols != n) {
        fprintf(stderr, "Expected %d PSCCH symbols but got %d symbols\n", nof_symbols, n);
        return ret;
      }

      /* extract channel estimates */
      for (i = 0; i < q->cell.nof_ports; i++) {
        //n = srslte_regs_pdcch_get(q->regs, cfi, ce[i][j], q->ce[i][j]);
        n = srslte_pscch_get(ce[i][j], q->ce[i][j], q->cell, prb_offset, n_prb);
        if (nof_symbols != n) {
          fprintf(stderr, "Expected %d PSCCH symbols but got %d symbols\n", nof_symbols, n);
          return ret;
        }
      }      
    }
    
    /* in control channels, only diversity is supported */
    if (q->cell.nof_ports == 1) {
      /* no need for layer demapping */
      srslte_predecoding_single_multi(q->symbols, q->ce[0], q->d, NULL, q->nof_rx_antennas, nof_symbols, 1.0f, noise_estimate);
    } else {
      srslte_predecoding_diversity_multi(q->symbols, q->ce, x, NULL, q->nof_rx_antennas, q->cell.nof_ports, nof_symbols, 1.0f);
      srslte_layerdemap_diversity(x, q->d, q->cell.nof_ports, nof_symbols / q->cell.nof_ports);
    }

    // transform de-precoding 9.4.4
    srslte_dft_precoding(&q->dft_deprecoding, q->d, q->d,
                          n_prb,//nof_prb
                          9);//nof_symbols

    /* demodulate symbols */
    srslte_demod_soft_demodulate(SRSLTE_MOD_QPSK, q->d, q->llr, nof_symbols);

    /* descramble */
    srslte_scrambling_f_offset(&q->seq, q->llr, 0, e_bits);

    ret = SRSLTE_SUCCESS;
  } 
  return ret;  
}


static void srslte_pscch_sci_encode_conv(srslte_pscch_t *q, uint8_t *data, uint32_t nof_bits, uint8_t *coded_data) {
  srslte_convcoder_t encoder;
  int poly[3] = { 0x6D, 0x4F, 0x57 };
  encoder.K = 7;
  encoder.R = 3;
  encoder.tail_biting = true;
  memcpy(encoder.poly, poly, 3 * sizeof(int));

  srslte_crc_attach(&q->crc, data, nof_bits);

  srslte_convcoder_encode(&encoder, data, coded_data, nof_bits + 16);
}


/**
 * @brief Does transportblock encoding of SL-CCH
 * 
 * @param q 
 * @param data      input data, upto 32 bytes where each byte represents one bit
 * @param e         output data, actually they are also interleaved so they should be called h
 * @param nof_bits  number of input bits
 * @param E         number of coded and ratematched bits   
 * @return int 
 */
int srslte_pscch_sci_encode(srslte_pscch_t *q, uint8_t *data, uint8_t *e, uint32_t nof_bits, uint32_t E) {
  uint8_t tmp[3 * (SRSLTE_SCI1_MAX_BITS + 16)];
  
  if (q                 != NULL        && 
      data              != NULL        && 
      e                 != NULL        && 
      nof_bits          <= SRSLTE_SCI1_MAX_BITS &&
      E                 <= q->max_bits)
  {

    srslte_pscch_sci_encode_conv(q, data, nof_bits, tmp); 

    DEBUG("CConv output: ");
    if (SRSLTE_VERBOSE_ISDEBUG()) {
      srslte_vec_fprint_b(stdout, tmp, 3 * (nof_bits + 16));
    }

    srslte_rm_conv_tx(tmp, 3 * (nof_bits + 16), q->e_int_tmp, E);

    for(int i=0; i<E; i++) {
      e[q->interleaver_lut[i]] = q->e_int_tmp[i];
    }
    
    return SRSLTE_SUCCESS;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}


/**
 * @brief Encodes SCI-1 into PSCCH
 * 
 * @note We only process 9 out the 10 SC_FDMA symbols as the last one is anyway not send.
 * 
 * @param q 
 * @param sci_msg     SCI-1 message, 1byte per bit, we expect 32
 * @param prb_offset  where PSCCH starts inside of the resource grid
 * @param sf_symbols  output iq-samples for the generated subframe
 * @return int 
 */
int srslte_pscch_encode(srslte_pscch_t *q, uint8_t *sci_msg, uint32_t prb_offset, cf_t *sf_symbols[SRSLTE_MAX_PORTS]) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  uint32_t i;
  // cf_t *x[SRSLTE_MAX_LAYERS];
  uint32_t nof_symbols;
  uint32_t e_bits;
  
  if (q                 != NULL &&
      sf_symbols        != NULL && 
      prb_offset        <=  q->cell.nof_prb - 2)
  {
    // we only process 9 out of 10 sc-fdma symbols, as the last one is used as guard
    nof_symbols = (10-1)*2*SRSLTE_NRE;
    e_bits = nof_symbols*srslte_mod_bits_x_symbol(SRSLTE_MOD_QPSK);
    ret = SRSLTE_ERROR;
    
    {      
      DEBUG("Encoding SCI: Nbits: %d, E: %d, oPRB: %d\n",
          SRSLTE_SCI1_MAX_BITS, e_bits, prb_offset);
          
      if(SRSLTE_SUCCESS != srslte_pscch_sci_encode(q, sci_msg, q->e, SRSLTE_SCI1_MAX_BITS, q->max_bits)) {
        printf("srslte_pscch_sci_encode failed\n");
        exit(-1);
      }

      srslte_scrambling_b_offset(&q->seq, q->e, 0, e_bits);
      
      DEBUG("Scrambling output: ");
      if (SRSLTE_VERBOSE_ISDEBUG()) {        
        srslte_vec_fprint_b(stdout, q->e, e_bits);
      }
      
      srslte_mod_modulate(&q->mod, q->e, q->d, e_bits);


      // /* number of layers equals number of ports */
      // for (i = 0; i < q->cell.nof_ports; i++) {
      //   x[i] = q->x[i];
      // }
      // memset(&x[q->cell.nof_ports], 0, sizeof(cf_t*) * (SRSLTE_MAX_LAYERS - q->cell.nof_ports));

      // /* layer mapping & precoding */
      // if (q->cell.nof_ports > 1) {
      //   srslte_layermap_diversity(q->d, x, q->cell.nof_ports, nof_symbols);
      //   srslte_precoding_diversity(x, q->symbols, q->cell.nof_ports, nof_symbols / q->cell.nof_ports, 1.0f);
      // } else {
      //   memcpy(q->symbols[0], q->d, nof_symbols * sizeof(cf_t));
      // }

      // transform precoding 9.4.4
      srslte_dft_precoding(&q->dft_precoding, q->d, q->d,
                            2,//nof_prb
                            9);//nof_symbols

      memcpy(q->symbols[0], q->d, nof_symbols * sizeof(cf_t));
      
      /* mapping to resource elements */
      for (i = 0; i < q->cell.nof_ports; i++) {
        srslte_pscch_put(q->symbols[i], sf_symbols[i], q->cell, prb_offset, 2);
        // srslte_regs_pdcch_put_offset(q->regs, cfi, q->symbols[i], sf_symbols[i],
        //                              location.ncce * 9, PDCCH_FORMAT_NOF_REGS(location.L));
      }
      
      ret = SRSLTE_SUCCESS;
      
    }
  }
  return ret;
}
