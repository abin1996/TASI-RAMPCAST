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
#include <stdlib.h>
#include <stdbool.h>
#include <srslte/phy/phch/pdsch_cfg.h>
#include <srslte/srslte.h>

#include <pthread.h>
#include <semaphore.h>

#include "prb_dl.h"
#include "srslte/phy/phch/pdsch.h"
#include "srslte/phy/phch/pssch.h"
#include "srslte/phy/utils/debug.h"
#include "srslte/phy/utils/vector.h"


#define MAX_PSSCH_RE(cp) (2 * (SRSLTE_CP_NSYMB(cp) - 2) * SRSLTE_NRE)


const static srslte_mod_t modulations[4] =
    { SRSLTE_MOD_BPSK, SRSLTE_MOD_QPSK, SRSLTE_MOD_16QAM, SRSLTE_MOD_64QAM };
    
//#define DEBUG_IDX

#ifdef DEBUG_IDX    
cf_t *offset_original_1=NULL;
extern int indices[100000];
extern int indices_ptr; 
#endif



typedef struct {
  /* Thread identifier: they must set before thread creation */
  pthread_t pthread;
  uint32_t cw_idx;
  uint32_t tb_idx;
  void *pdsch_ptr;
  bool *ack;

  /* Configuration Encoder/Decoder: they must be set before posting start semaphore */
  srslte_pssch_cfg_t *cfg;
  srslte_sch_t dl_sch;
  uint16_t rnti;

  /* Encoder/Decoder data pointers: they must be set before posting start semaphore  */
  uint8_t *data;
  void *softbuffer;

  /* Execution status */
  int ret_status;

  /* Semaphores */
  sem_t start;
  sem_t finish;

  /* Thread flags */
  bool started;
  bool quit;
} srslte_pssch_coworker_t;

#if 0
int srslte_pssch_cp(srslte_pssch_t *q, cf_t *input, cf_t *output, srslte_ra_dl_grant_t *grant, uint32_t lstart_grant, uint32_t nsubframe, bool put)
{
  uint32_t s, n, l, lp, lstart, lend, nof_refs;
  bool is_pbch, is_sss;
  cf_t *in_ptr = input, *out_ptr = output;
  uint32_t offset = 0;

#ifdef DEBUG_IDX    
  indices_ptr = 0; 
  if (put) {
    offset_original = output; 
  } else {
    offset_original = input;     
  }
#endif
  
  if (q->cell.nof_ports == 1) {
    nof_refs = 2;
  } else {
    nof_refs = 4;
  }

  for (s = 0; s < 2; s++) {
    for (l = 0; l < SRSLTE_CP_NSYMB(q->cell.cp); l++) {
      for (n = 0; n < q->cell.nof_prb; n++) {

        // If this PRB is assigned
        if (grant->prb_idx[s][n]) {
          if (s == 0) {
            lstart = lstart_grant;
          } else {
            lstart = 0;
          }
          lend = SRSLTE_CP_NSYMB(q->cell.cp);
          is_pbch = is_sss = false;

          // Skip PSS/SSS signals
          if (s == 0 && (nsubframe == 0 || nsubframe == 5)) {
            if (n >= q->cell.nof_prb / 2 - 3
                && n < q->cell.nof_prb / 2 + 3 + (q->cell.nof_prb%2)) {
              lend = SRSLTE_CP_NSYMB(q->cell.cp) - 2;
              is_sss = true;
            }
          }
          // Skip PBCH
          if (s == 1 && nsubframe == 0) {
            if (n >= q->cell.nof_prb / 2 - 3
                && n < q->cell.nof_prb / 2 + 3 + (q->cell.nof_prb%2)) {
              lstart = 4;
              is_pbch = true;
            }
          }
          lp = l + s * SRSLTE_CP_NSYMB(q->cell.cp);
          if (put) {
            out_ptr = &output[(lp * q->cell.nof_prb + n)
                * SRSLTE_NRE];
          } else {
            in_ptr = &input[(lp * q->cell.nof_prb + n)
                * SRSLTE_NRE];
          }
          // This is a symbol in a normal PRB with or without references
          if (l >= lstart && l < lend) {
            if (SRSLTE_SYMBOL_HAS_REF(l, q->cell.cp, q->cell.nof_ports)) {
              if (nof_refs == 2) {
                if (l == 0) {
                  offset = q->cell.id % 6;
                } else {
                  offset = (q->cell.id + 3) % 6;                  
                }
              } else {
                offset = q->cell.id % 3;
              }
              prb_cp_ref(&in_ptr, &out_ptr, offset, nof_refs, nof_refs, put);
            } else {
              prb_cp(&in_ptr, &out_ptr, 1);
            }
          }
          // This is a symbol in a PRB with PBCH or Synch signals (SS). 
          // If the number or total PRB is odd, half of the the PBCH or SS will fall into the symbol
          if ((q->cell.nof_prb % 2) && ((is_pbch && l < lstart) || (is_sss && l >= lend))) {
            if (n == q->cell.nof_prb / 2 - 3) {
              if (SRSLTE_SYMBOL_HAS_REF(l, q->cell.cp, q->cell.nof_ports)) {
                prb_cp_ref(&in_ptr, &out_ptr, offset, nof_refs, nof_refs/2, put);
              } else {
                prb_cp_half(&in_ptr, &out_ptr, 1);
              }
            } else if (n == q->cell.nof_prb / 2 + 3) {
              if (put) {
                out_ptr += 6;
              } else {
                in_ptr += 6;
              }
              if (SRSLTE_SYMBOL_HAS_REF(l, q->cell.cp, q->cell.nof_ports)) {
                prb_cp_ref(&in_ptr, &out_ptr, offset, nof_refs, nof_refs/2, put);
              } else {
                prb_cp_half(&in_ptr, &out_ptr, 1);
              }
            }
          }
        }
      }      
    }
  }
  
  int r; 
  if (put) {
    r = abs((int) (input - in_ptr));
  } else {
    r = abs((int) (output - out_ptr));
  }

  return r; 
}

/**
 * Puts PDSCH in slot number 1
 *
 * Returns the number of symbols written to sf_symbols
 *
 * 36.211 10.3 section 6.3.5
 */
int srslte_pssch_put(srslte_pssch_t *q, cf_t *symbols, cf_t *sf_symbols,
    srslte_ra_dl_grant_t *grant, uint32_t lstart, uint32_t subframe) 
{
  return srslte_pssch_cp(q, symbols, sf_symbols, grant, lstart, subframe, true);
}

/**
 * Extracts PSSCH from slot number 1
 *
 * Returns the number of symbols written to PSSCH
 *
 * 36.211 10.3 section 6.3.5
 */
int srslte_pssch_get(srslte_pssch_t *q, cf_t *sf_symbols, cf_t *symbols,
    srslte_ra_dl_grant_t *grant, uint32_t lstart, uint32_t subframe) 
{
  return srslte_pssch_cp(q, sf_symbols, symbols, grant, lstart, subframe, false);
}
#endif 

int srslte_pssch_get_for_sps_rsrp(cf_t *subframe, cf_t *pssch, srslte_cell_t cell, uint32_t prb_offset, uint32_t n_prb)
{
  return srslte_pssch_cp_for_sps_rsrp(subframe, pssch, cell, prb_offset, n_prb);
}

cf_t*      offset_original_2;
int srslte_pssch_cp_for_sps_rsrp(cf_t* input, cf_t* output, srslte_cell_t cell, uint32_t prb_offset,
                                         uint32_t n_prb)
{
  int   i;
  cf_t* ptr;

  offset_original_2 = input;

  // adjust in/out pointer to account for prb_offset
  ptr = output;
  input += prb_offset * SRSLTE_NRE;
  
/*   for (i = 0; i < 2 * SRSLTE_CP_NSYMB(cell.cp) - 1; i++) {
    // symbols not containing demodulation reference signals of pssch, TS 36.211, Chapter 9.8.
    if (i != 2 || i != 5 || i != 9 || i != 12) {
      input += cell.nof_prb * SRSLTE_NRE;
    } else {
      // printf("sym: %d from %x to %x\n", i, input, output);
      prb_cp(&input, &output, n_prb);
      input += cell.nof_prb * SRSLTE_NRE - n_prb * SRSLTE_NRE;
    }
  } */
  // exclude last symbol as it is used as guard
  for (i = 0; i < 2 * SRSLTE_CP_NSYMB(cell.cp) - 1; i++) {
    // symbols containing demodulation reference signals of pssch, TS 36.211, Chapter 9.8.
    // if (i == 2 || i == 5 || i == 9 || i == 12) {
      if (i == 2 || i == 5 || i == 8 || i == 11) {
      // printf("sym: %d from %x to %x\n", i, input, output);
      prb_cp(&input, &output, n_prb);
      input += cell.nof_prb * SRSLTE_NRE - n_prb * SRSLTE_NRE;
    } else {
      input += cell.nof_prb * SRSLTE_NRE;
    }
  }
  return output - ptr;
}

int srslte_pssch_get_for_sps_rssi(cf_t *subframe, cf_t *pssch, srslte_cell_t cell, uint32_t prb_offset, uint32_t n_prb)
{
  return srslte_pssch_cp_for_sps_rssi(subframe, pssch, cell, prb_offset, n_prb);
}

cf_t*      offset_original_3;
int srslte_pssch_cp_for_sps_rssi(cf_t* input, cf_t* output, srslte_cell_t cell, uint32_t prb_offset,
                                         uint32_t n_prb)
{
  int   i;
  cf_t* ptr;

  offset_original_3 = input;

  // adjust in/out pointer to account for prb_offset
  ptr = output;
  input += prb_offset * SRSLTE_NRE;

  // exclude last symbol as it is used as guard
  for (i = 0; i < 2 * SRSLTE_CP_NSYMB(cell.cp); i++) {
    // symbols not containing pssch
    if (i == 0 || i == 13) {
      input += cell.nof_prb * SRSLTE_NRE;
    } else {
      // printf("sym: %d from %x to %x\n", i, input, output);
      prb_cp(&input, &output, n_prb);
      input += cell.nof_prb * SRSLTE_NRE - n_prb * SRSLTE_NRE;
    }
  }
  return output - ptr;
}

/**
 * @brief Generate interleaver sequence for sidelink PSSCH
 * 
 * @param q 
 * @param mod 
 */
static void interleaver_table_gen(srslte_pssch_t *q, srslte_mod_t mod, uint32_t max_bits) {
  uint32_t Qm = srslte_mod_bits_x_symbol(mod);
  uint32_t H_prime_total = max_bits / Qm;
  uint32_t N_pucch_symbs = 2*(SRSLTE_CP_NORM_NSYMB - 2); // 36.212 5.4.3

  uint32_t rows = H_prime_total/N_pucch_symbs;
  uint32_t cols = N_pucch_symbs;
  uint32_t idx = 0;
  for(uint32_t j=0; j<rows; j++) {        
    for(uint32_t i=0; i<cols; i++) {
      for(uint32_t k=0; k<Qm; k++) {
          // this is the indexing from SRSlte and which think is correct
          q->interleaver_lut[j*Qm + i*rows*Qm + k] = idx;

          // this is the indexing from feron matlab implementation, which may be wrong
          //q->interleaver_lut[idx] = j*Qm + i*rows*Qm + k;
          idx++;                  
      }
    }
  }
}

/** Initializes the PDSCH transmitter and receiver */
static int pssch_init(srslte_pssch_t *q, uint32_t max_prb, bool is_ue, uint32_t nof_antennas)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL)
  {
    
    bzero(q, sizeof(srslte_pssch_t));
    ret = SRSLTE_ERROR;
    
    q->max_re          = max_prb * MAX_PSSCH_RE(q->cell.cp);
    q->is_ue           = is_ue;
    q->nof_rx_antennas = nof_antennas;

    INFO("Init PSSCH: %d PRBs, max_symbols: %d\n", max_prb, q->max_re);

    for (int i = 0; i < 4; i++) {
      if (srslte_modem_table_lte(&q->mod[i], modulations[i])) {
        goto clean;
      }
      srslte_modem_table_bytes(&q->mod[i]);
    }

    if (srslte_sch_init(&q->dl_sch)) {
      ERROR("Initiating SL SCH");
      goto clean;
    }

    for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
      // Allocate int16_t for reception (LLRs)
      q->e[i] = srslte_vec_malloc(sizeof(int16_t) * q->max_re * srslte_mod_bits_x_symbol(SRSLTE_MOD_16QAM));
      if (!q->e[i]) {
        goto clean;
      }

      q->h[i] = srslte_vec_malloc(sizeof(int16_t) * q->max_re * srslte_mod_bits_x_symbol(SRSLTE_MOD_16QAM));
      if (!q->h[i]) {
        goto clean;
      }

      q->d[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_re);
      if (!q->d[i]) {
        goto clean;
      }

      q->csi[i] = srslte_vec_malloc(sizeof(float) * q->max_re);
      if (!q->csi[i]) {
        goto clean;
      }
    }

    for (int i = 0; i < SRSLTE_MAX_PORTS; i++) {
      q->x[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_re);
      if (!q->x[i]) {
        goto clean;
      }
      q->symbols[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_re);
      if (!q->symbols[i]) {
        goto clean;
      }
      q->SymSPSRsrp[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_re);
      if (!q->SymSPSRsrp[i]) {
        goto clean;
      }
      q->SymSPSRssi[i] = srslte_vec_malloc(sizeof(cf_t) * q->max_re);
      if (!q->SymSPSRssi[i]) {
        goto clean;
      }
      if (q->is_ue) {
        for (int j = 0; j < SRSLTE_MAX_PORTS; j++) {
          q->ce[i][j] = srslte_vec_malloc(sizeof(cf_t) * q->max_re);
          if (!q->ce[i][j]) {
            goto clean;
          }
        }
      }
    }

    q->users = calloc(sizeof(srslte_pssch_user_t*), q->is_ue?1:(1+SRSLTE_SIRNTI));
    if (!q->users) {
      perror("malloc");
      goto clean;
    }

    if (srslte_sequence_init(&q->tmp_seq, q->max_re * srslte_mod_bits_x_symbol(SRSLTE_MOD_16QAM))) {
      goto clean;
    }

    // setup dft de-precoding
    if (srslte_dft_precoding_init(&q->dft_deprecoding, max_prb, false)) {
      fprintf(stderr, "Error initiating DFT transform precoding\n");
      goto clean; 
    }

    // setup dft precoding
    if (srslte_dft_precoding_init(&q->dft_precoding, max_prb, true)) {
      fprintf(stderr, "Error initiating DFT transform precoding\n");
      goto clean; 
    }

    // interleaver table
    // actually we could use the interleaver functionality from sch.c but because
    // we do not include any control information we only took the relevant part 
    // to genenerate a more lightweight version
    q->interleaver_lut = srslte_vec_malloc(sizeof(uint16_t) * q->max_re * srslte_mod_bits_x_symbol(SRSLTE_MOD_16QAM));
    if (!q->interleaver_lut) {
      goto clean;
    }
    //interleaver_table_gen(q);


    ret = SRSLTE_SUCCESS;
  }

  clean: 
  if (ret == SRSLTE_ERROR) {
    srslte_pssch_free(q);
  }
  return ret;
}

int srslte_pssch_init_ue(srslte_pssch_t *q, uint32_t max_prb, uint32_t nof_antennas)
{
  return pssch_init(q, max_prb, true, nof_antennas);
}

int srslte_pssch_init_enb(srslte_pssch_t *q, uint32_t max_prb)
{
  return pssch_init(q, max_prb, false, 0);
}

static void srslte_pssch_disable_coworker(srslte_pssch_t *q) {
  srslte_pssch_coworker_t *h = (srslte_pssch_coworker_t *) q->coworker_ptr;
  if (h) {
    /* Stop threads */
    h->quit = true;
    sem_post(&h->start);

    pthread_join(h->pthread, NULL);
    pthread_detach(h->pthread);

    srslte_sch_free(&h->dl_sch);

    free(h);

    q->coworker_ptr = NULL;
  }
}

void srslte_pssch_free(srslte_pssch_t *q) {
  srslte_pssch_disable_coworker(q);

  for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {

    if (q->e[i]) {
      free(q->e[i]);
    }

    if (q->h[i]) {
      free(q->h[i]);
    }

    if (q->d[i]) {
      free(q->d[i]);
    }

    if (q->csi[i]) {
      free(q->csi[i]);
    }
  }

  /* Free sch objects */
  srslte_sch_free(&q->dl_sch);

  for (int i = 0; i < SRSLTE_MAX_PORTS; i++) {
    if (q->x[i]) {
      free(q->x[i]);
    }
    if (q->symbols[i]) {
      free(q->symbols[i]);
    }
    if (q->SymSPSRsrp[i]) {
      free(q->SymSPSRsrp[i]);
    }
    if (q->SymSPSRssi[i]) {
      free(q->SymSPSRssi[i]);
    }
    if (q->is_ue) {
      for (int j = 0; j < SRSLTE_MAX_PORTS; j++) {
        if (q->ce[i][j]) {
          free(q->ce[i][j]);
        }
      }
    }
  }
  if (q->users) {
    if (q->is_ue) {
      srslte_pssch_free_rnti(q, 0);
    } else {
      for (int u=0;u<=SRSLTE_SIRNTI;u++) {
        if (q->users[u]) {
          srslte_pssch_free_rnti(q, u);
        }
      }
    }
    free(q->users);
  }

  srslte_sequence_free(&q->tmp_seq);

  for (int i = 0; i < 4; i++) {
    srslte_modem_table_free(&q->mod[i]);
  }

  srslte_dft_precoding_free(&q->dft_precoding);
  srslte_dft_precoding_free(&q->dft_deprecoding);

  if (q->interleaver_lut) {
    free(q->interleaver_lut);
  }

  bzero(q, sizeof(srslte_pssch_t));
}

int srslte_pssch_set_cell(srslte_pssch_t *q, srslte_cell_t cell)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL                  &&
      srslte_cell_isvalid(&cell))
  {
    memcpy(&q->cell, &cell, sizeof(srslte_cell_t));
    q->max_re = q->cell.nof_prb * MAX_PSSCH_RE(q->cell.cp);

    INFO("PDSCH: Cell config PCI=%d, %d ports, %d PRBs, max_symbols: %d\n",
         q->cell.id, q->cell.nof_ports, q->cell.nof_prb, q->max_re);

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

/* Precalculate the PDSCH scramble sequences for a given RNTI. This function takes a while 
 * to execute, so shall be called once the final C-RNTI has been allocated for the session.
 */
int srslte_pssch_set_rnti(srslte_pssch_t *q, uint16_t rnti) {
  uint32_t rnti_idx = q->is_ue?0:rnti;

  if (!q->users[rnti_idx] || q->is_ue) {
    if (!q->users[rnti_idx]) {
      q->users[rnti_idx] = calloc(1, sizeof(srslte_pssch_user_t));
      if(!q->users[rnti_idx]) {
        perror("calloc");
        return -1;
      }
    }
    for (int i = 0; i < SRSLTE_NOF_SF_X_FRAME; i++) {
      for (int j = 0; j < SRSLTE_MAX_CODEWORDS; j++) {
        if (srslte_sequence_pdsch(&q->users[rnti_idx]->seq[j][i], rnti, j, 2 * i, q->cell.id,
                                  q->max_re * srslte_mod_bits_x_symbol(SRSLTE_MOD_64QAM)))
        {
          fprintf(stderr, "Error initializing PDSCH scrambling sequence\n");
          srslte_pssch_free_rnti(q, rnti);
          return SRSLTE_ERROR;
        }
      }
    }
    q->ue_rnti = rnti; 
    q->users[rnti_idx]->cell_id = q->cell.id;
    q->users[rnti_idx]->sequence_generated = true;
  } else {
    fprintf(stderr, "Error generating PDSCH sequence: rnti=0x%x already generated\n", rnti);
  }
  return SRSLTE_SUCCESS;
}

void srslte_pssch_set_power_allocation(srslte_pssch_t *q, float rho_a) {
  if (q) {
    q->rho_a = rho_a;
  }
}

int srslte_pssch_enable_csi(srslte_pssch_t *q, bool enable) {
  if (enable) {
    for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
      if (!q->csi[i]) {
        q->csi[i] = srslte_vec_malloc(sizeof(float) * q->max_re * 2);
        if (!q->csi[i]) {
          return SRSLTE_ERROR;
        }
      }
    }
  }
  q->csi_enabled = enable;

  return SRSLTE_SUCCESS;
}

void srslte_pssch_free_rnti(srslte_pssch_t* q, uint16_t rnti)
{
  uint32_t rnti_idx = q->is_ue?0:rnti;
  if (q->users[rnti_idx]) {
    for (int i = 0; i < SRSLTE_NOF_SF_X_FRAME; i++) {
      for (int j = 0; j < SRSLTE_MAX_CODEWORDS; j++) {
        srslte_sequence_free(&q->users[rnti_idx]->seq[j][i]);
      }
    }
    free(q->users[rnti_idx]);
    q->users[rnti_idx] = NULL;
    q->ue_rnti = 0;
  }
}

#if 0
static void pdsch_decode_debug(srslte_pssch_t *q, srslte_pssch_cfg_t *cfg,
                               cf_t *sf_symbols[SRSLTE_MAX_PORTS], cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS])
{
  if (SRSLTE_VERBOSE_ISDEBUG()) {
    char filename[FILENAME_MAX];
    for (int j = 0; j < q->nof_rx_antennas; j++) {
      if (snprintf(filename, FILENAME_MAX, "subframe_p%d.dat", j) < 0) {
        ERROR("Generating file name");
        break;
      }
      DEBUG("SAVED FILE %s: received subframe symbols\n", filename);
      srslte_vec_save_file(filename, sf_symbols[j], SRSLTE_SF_LEN_RE(q->cell.nof_prb, q->cell.cp)*sizeof(cf_t));

      for (int i = 0; i < q->cell.nof_ports; i++) {
        if (snprintf(filename, FILENAME_MAX, "hest_%d%d.dat", i, j) < 0) {
          ERROR("Generating file name");
          break;
        }
        DEBUG("SAVED FILE %s: channel estimates for Tx %d and Rx %d\n", filename, j, i);
        srslte_vec_save_file(filename, ce[i][j], SRSLTE_SF_LEN_RE(q->cell.nof_prb, q->cell.cp)*sizeof(cf_t));
      }
    }
    for (int i=0;i<cfg->nof_layers;i++) {
      if (snprintf(filename, FILENAME_MAX, "pdsch_symbols_%d.dat", i) < 0) {
        ERROR("Generating file name");
        break;
      }
      DEBUG("SAVED FILE %s: symbols after equalization\n", filename);
      srslte_vec_save_file(filename, q->d[i], cfg->nbits[0].nof_re*sizeof(cf_t));

      if (snprintf(filename, FILENAME_MAX, "llr_%d.dat", i) < 0) {
        ERROR("Generating file name");
        break;
      }
      DEBUG("SAVED FILE %s: LLR estimates after demodulation and descrambling\n", filename);
      srslte_vec_save_file(filename, q->e[i], cfg->nbits[0].nof_bits*sizeof(int16_t));
    }
  }
}
#endif

#if 0
/* Configures the structure srslte_pssch_cfg_t from the DL DCI allocation dci_msg. 
 * If dci_msg is NULL, the grant is assumed to be already stored in cfg->grant
 */
int srslte_pssch_cfg(srslte_pssch_cfg_t *cfg, srslte_cell_t cell, srslte_ra_dl_grant_t *grant, uint32_t cfi,
                     uint32_t sf_idx, int rvidx) {
  int _rvids[SRSLTE_MAX_CODEWORDS] = {1};
  _rvids[0] = rvidx;

  return srslte_pssch_cfg_mimo(cfg, cell, grant, cfi, sf_idx, _rvids, SRSLTE_MIMO_TYPE_SINGLE_ANTENNA, 0);
}

/* Configures the structure srslte_pssch_cfg_t from the DL DCI allocation dci_msg.
 * If dci_msg is NULL, the grant is assumed to be already stored in cfg->grant
 */
int srslte_pssch_cfg_mimo(srslte_pssch_cfg_t *cfg, srslte_cell_t cell, srslte_ra_dl_grant_t *grant, uint32_t cfi,
                           uint32_t sf_idx, int rvidx[SRSLTE_MAX_CODEWORDS], srslte_mimo_type_t mimo_type,
                           uint32_t pmi) {
  if (cfg && grant) {
    uint32_t nof_tb = SRSLTE_RA_DL_GRANT_NOF_TB(grant);
    memcpy(&cfg->grant, grant, sizeof(srslte_ra_dl_grant_t));


    for (int cw = 0; cw < SRSLTE_MAX_CODEWORDS; cw++) {
      if (grant->tb_en[cw]) {
        if (srslte_cbsegm(&cfg->cb_segm[cw], (uint32_t) cfg->grant.mcs[cw].tbs)) {
          fprintf(stderr, "Error computing Codeword (%d) segmentation for TBS=%d\n", cw, cfg->grant.mcs[cw].tbs);
          return SRSLTE_ERROR;
        }
      }
    }
    srslte_ra_dl_grant_to_nbits(&cfg->grant, cfi, cell, sf_idx, cfg->nbits);

    cfg->sf_idx = sf_idx;
    memcpy(cfg->rv, rvidx, sizeof(uint32_t) * SRSLTE_MAX_CODEWORDS);
    cfg->mimo_type = mimo_type;
    cfg->tb_cw_swap = grant->tb_cw_swap;

    /* Check and configure PDSCH transmission modes */
    switch(mimo_type) {
      case SRSLTE_MIMO_TYPE_SINGLE_ANTENNA:
        if (nof_tb != 1) {
          ERROR("Wrong number of transport blocks (%d) for single antenna.", nof_tb);
          return SRSLTE_ERROR;
        }
        cfg->nof_layers = 1;
        break;
      case SRSLTE_MIMO_TYPE_TX_DIVERSITY:
        if (nof_tb != 1) {
          ERROR("Wrong number of transport blocks (%d) for transmit diversity.", nof_tb);
          return SRSLTE_ERROR;
        }
        cfg->nof_layers = cell.nof_ports;
        break;
      case SRSLTE_MIMO_TYPE_SPATIAL_MULTIPLEX:
        if (nof_tb == 1) {
          cfg->codebook_idx = pmi;
          cfg->nof_layers = 1;
        } else if (nof_tb == 2) {
          cfg->codebook_idx = pmi + 1;
          cfg->nof_layers = 2;
        } else {
          ERROR("Wrong number of transport blocks (%d) for spatial multiplexing.", nof_tb);
          return SRSLTE_ERROR;
        }
        INFO("PDSCH configured for Spatial Multiplex; nof_codewords=%d; nof_layers=%d; codebook_idx=%d;\n",
             nof_tb, cfg->nof_layers, cfg->codebook_idx);
        break;
      case SRSLTE_MIMO_TYPE_CDD:
        if (nof_tb != 2) {
          ERROR("Wrong number of transport blocks (%d) for CDD.", nof_tb);
          return SRSLTE_ERROR;
        }
        cfg->nof_layers = 2;
        break;
    }

    return SRSLTE_SUCCESS;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}

static srslte_sequence_t *get_user_sequence(srslte_pssch_t *q, uint16_t rnti,
                                            uint32_t codeword_idx, uint32_t sf_idx, uint32_t len)
{
  uint32_t rnti_idx = q->is_ue?0:rnti;

  // The scrambling sequence is pregenerated for all RNTIs in the eNodeB but only for C-RNTI in the UE
  if (q->users[rnti_idx] && q->users[rnti_idx]->sequence_generated &&
      q->users[rnti_idx]->cell_id == q->cell.id                    &&
      q->ue_rnti == rnti                                           &&
      ((rnti >= SRSLTE_CRNTI_START && rnti < SRSLTE_CRNTI_END) || !q->is_ue))
  {
    return &q->users[rnti_idx]->seq[codeword_idx][sf_idx];
  } else {
    srslte_sequence_pdsch(&q->tmp_seq, rnti, codeword_idx, 2 * sf_idx, q->cell.id, len);
    return &q->tmp_seq;
  }
}

static int srslte_pssch_codeword_encode(srslte_pssch_t *q, srslte_pssch_cfg_t *cfg,
                                               srslte_softbuffer_tx_t *softbuffer, uint16_t rnti, uint8_t *data,
                                               uint32_t codeword_idx, uint32_t tb_idx) {
  srslte_ra_nbits_t *nbits = &cfg->nbits[tb_idx];
  srslte_ra_mcs_t *mcs = &cfg->grant.mcs[tb_idx];
  uint32_t rv = cfg->rv[tb_idx];
  bool valid_inputs = true;

  if (!softbuffer) {
    ERROR("Error encoding (TB%d -> CW%d), softbuffer=NULL", tb_idx, codeword_idx);
    valid_inputs = false;
  }

  if (nbits->nof_bits && valid_inputs) {
    INFO("Encoding PDSCH SF: %d (TB%d -> CW%d), Mod %s, NofBits: %d, NofSymbols: %d, NofBitsE: %d, rv_idx: %d\n",
         cfg->sf_idx, tb_idx, codeword_idx, srslte_mod_string(mcs->mod), mcs->tbs,
         nbits->nof_re, nbits->nof_bits, rv);

    /* Channel coding */
    if (srslte_dlsch_encode2(&q->dl_sch, cfg, softbuffer, data, q->e[codeword_idx], tb_idx)) {
      ERROR("Error encoding (TB%d -> CW%d)", tb_idx, codeword_idx);
      return SRSLTE_ERROR;
    }

    /* Select scrambling sequence */
    srslte_sequence_t *seq = get_user_sequence(q, rnti, codeword_idx, cfg->sf_idx, nbits->nof_bits);

    /* Bit scrambling */
    srslte_scrambling_bytes(seq, (uint8_t *) q->e[codeword_idx], nbits->nof_bits);

    /* Bit mapping */
    srslte_mod_modulate_bytes(&q->mod[mcs->mod],
                              (uint8_t *) q->e[codeword_idx],
                              q->d[codeword_idx], nbits->nof_bits);

  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  return SRSLTE_SUCCESS;
}
#endif

#if 0
static int srslte_pssch_codeword_decode(srslte_pssch_t *q, srslte_pssch_cfg_t *cfg, srslte_sch_t *dl_sch,
                                        srslte_softbuffer_rx_t *softbuffer, uint16_t rnti, uint8_t *data,
                                        uint32_t codeword_idx, uint32_t tb_idx, bool *ack) {
  srslte_ra_nbits_t *nbits = &cfg->nbits[tb_idx];
  srslte_ra_mcs_t *mcs = &cfg->grant.mcs[tb_idx];
  uint32_t rv = cfg->rv[tb_idx];
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (softbuffer && data && ack && nbits->nof_bits && nbits->nof_re) {
    INFO("Decoding PDSCH SF: %d (CW%d -> TB%d), Mod %s, NofBits: %d, NofSymbols: %d, NofBitsE: %d, rv_idx: %d\n",
         cfg->sf_idx, codeword_idx, tb_idx, srslte_mod_string(mcs->mod), mcs->tbs,
         nbits->nof_re, nbits->nof_bits, rv);

    /* demodulate symbols
     * The MAX-log-MAP algorithm used in turbo decoding is unsensitive to SNR estimation,
     * thus we don't need tot set it in the LLRs normalization
     */
    srslte_demod_soft_demodulate_s(mcs->mod, q->d[codeword_idx], q->e[codeword_idx], nbits->nof_re);

    /* Select scrambling sequence */
    srslte_sequence_t *seq = get_user_sequence(q, rnti, codeword_idx, cfg->sf_idx, nbits->nof_bits);

    /* Bit scrambling */
    srslte_scrambling_s_offset(seq, q->e[codeword_idx], 0, nbits->nof_bits);

    uint32_t qm = 0;
    switch(cfg->grant.mcs[tb_idx].mod) {

      case SRSLTE_MOD_BPSK:
        qm = 1;
        break;
      case SRSLTE_MOD_QPSK:
        qm = 2;
        break;
      case SRSLTE_MOD_16QAM:
        qm = 4;
        break;
      case SRSLTE_MOD_64QAM:
        qm = 6;
        break;
      default:
        ERROR("No modulation");
    }

    int16_t *e = q->e[codeword_idx];

    if (q->csi_enabled) {
      const uint32_t csi_max_idx = srslte_vec_max_fi(q->csi[codeword_idx], nbits->nof_bits / qm);
      float csi_max = 1.0f;
      if (csi_max_idx < nbits->nof_bits / qm) {
        csi_max = q->csi[codeword_idx][csi_max_idx];
      }
      for (int i = 0; i < nbits->nof_bits / qm; i++) {
        const float csi = q->csi[codeword_idx][i] / csi_max;
        for (int k = 0; k < qm; k++) {
          e[qm * i + k] = (int16_t) ((float) e[qm * i + k] * csi);
        }
      }
    }

    /* Return  */
    ret = srslte_dlsch_decode2(dl_sch, cfg, softbuffer, q->e[codeword_idx], data, tb_idx);

    q->last_nof_iterations[codeword_idx] = srslte_sch_last_noi(&q->dl_sch);

    if (ret == SRSLTE_SUCCESS) {
      *ack = true;
    } else if (ret == SRSLTE_ERROR) {
      *ack = false;
      ret = SRSLTE_SUCCESS;
    } else if (ret == SRSLTE_ERROR_INVALID_INPUTS) {
      *ack = false;
      ret = SRSLTE_ERROR;
    }
  } else {
    ERROR("Detected NULL pointer in TB%d &softbuffer=%p &data=%p &ack=%p, nbits=%d, nof_re=%d",
          codeword_idx, softbuffer, (void*)data, ack, nbits->nof_bits, nbits->nof_re);
  }

  return ret;
}

static void *srslte_pssch_decode_thread(void *arg) {
  srslte_pssch_coworker_t *q = (srslte_pssch_coworker_t *) arg;

  INFO("[PDSCH Coworker] waiting for data\n");

  sem_wait(&q->start);
  while (!q->quit) {
    q->ret_status = srslte_pssch_codeword_decode(q->pdsch_ptr,
                                                 q->cfg,
                                                 &q->dl_sch,
                                                 q->softbuffer,
                                                 q->rnti,
                                                 q->data,
                                                 q->cw_idx,
                                                 q->tb_idx,
                                                 q->ack);

    /* Post finish semaphore */
    sem_post(&q->finish);

    /* Wait for next loop */
    sem_wait(&q->start);
  }
  sem_post(&q->finish);

  pthread_exit(NULL);
  return q;
}
#endif


int srslte_pssch_decode_simple(srslte_pssch_t *q,
                        srslte_ra_sl_sci_t *sci,
                        srslte_pssch_cfg_t *cfg,
                        srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                        cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                        cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                        float noise_estimate,
                        uint32_t prb_offset,
                        uint32_t n_prb,
                        uint8_t *data[SRSLTE_MAX_CODEWORDS])
{

  /* Set pointers for layermapping & precoding */
  uint32_t i;
  //cf_t *x[SRSLTE_MAX_LAYERS];

  uint32_t nof_symbols = n_prb*SRSLTE_NRE*9;

  if (q            == NULL ||
      sf_symbols   == NULL ||
      ce           == NULL
      //data         != NULL ||
      //cfg          != NULL
      )
  {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  // uint32_t nof_tb = 1;//SRSLTE_RA_DL_GRANT_NOF_TB(&cfg->grant);

  // INFO("Decoding PDSCH SF: %d, RNTI: 0x%x, NofSymbols: %d, C_prb=%d, mimo_type=%s, nof_layers=%d, nof_tb=%d\n",
  //     cfg->sf_idx, rnti, cfg->nbits[0].nof_re, cfg->grant.nof_prb, srslte_mod_string(cfg->grant.mcs->mod), cfg->nof_layers, nof_tb);

  /* extract symbols */
  for (int j=0;j<q->nof_rx_antennas;j++) {
    //int n = srslte_regs_pdcch_get(q->regs, cfi, sf_symbols[j], q->symbols[j]);
    int n = srslte_pscch_get(sf_symbols[j], q->symbols[j], q->cell, prb_offset, n_prb);
    if (nof_symbols != n) {
      fprintf(stderr, "Expected %d PSCCH symbols but got %d symbols\n", nof_symbols, n);
      return SRSLTE_ERROR;
    }

    /* extract channel estimates */
    for (i = 0; i < q->cell.nof_ports; i++) {
      //n = srslte_regs_pdcch_get(q->regs, cfi, ce[i][j], q->ce[i][j]);
      n = srslte_pscch_get(ce[i][j], q->ce[i][j], q->cell, prb_offset, n_prb);
      if (nof_symbols != n) {
        fprintf(stderr, "Expected %d PSCCH symbols but got %d symbols\n", nof_symbols, n);
        return SRSLTE_ERROR;
      }
    }      
  }

  if (q->cell.nof_ports == 1) {
    /* no need for layer demapping */
    srslte_predecoding_single_multi(q->symbols, q->ce[0], q->d[0], NULL, q->nof_rx_antennas, nof_symbols, 1.0f, noise_estimate);
  } else {
    printf("Ceck if we can decode PSSCH from multiple ports.");
    return SRSLTE_ERROR;
    // srslte_predecoding_diversity_multi(q->symbols, q->ce, x, NULL, q->nof_rx_antennas, q->cell.nof_ports, nof_symbols, 1.0f);
    // srslte_layerdemap_diversity(x, q->d, q->cell.nof_ports, nof_symbols / q->cell.nof_ports);
  }


  // transform de-precoding 9.4.4
  srslte_dft_precoding(&q->dft_deprecoding, q->d[0], q->d[0],
                        n_prb,//nof_prb
                        9);//nof_symbols

  /* demodulate symbols
    * The MAX-log-MAP algorithm used in turbo decoding is unsensitive to SNR estimation,
    * thus we don't need tot set it in the LLRs normalization
    */
  srslte_demod_soft_demodulate_s(sci->mcs.mod, q->d[0], q->h[0], nof_symbols);

  /* Select scrambling sequence */
  // srslte_sequence_t *seq = get_user_sequence(q, rnti, codeword_idx, cfg->sf_idx, nbits->nof_bits);
  srslte_sequence_pssch(&q->tmp_seq, nof_symbols*srslte_mod_bits_x_symbol(sci->mcs.mod), q->n_X_ID, q->n_PSSCH_ssf);

  /* Bit scrambling */
  srslte_scrambling_s_offset(&q->tmp_seq, q->h[0], 0, nof_symbols*srslte_mod_bits_x_symbol(sci->mcs.mod));

  int16_t *h = q->h[0];
  int16_t *e = q->e[0];

  // explicitely clear last symbol, as it is used as guard
  bzero(&h[nof_symbols*srslte_mod_bits_x_symbol(sci->mcs.mod)], sizeof(int16_t) * 1*SRSLTE_NRE*n_prb*srslte_mod_bits_x_symbol(sci->mcs.mod));


  /**
   * phy decoding is now finished, now follows transport block processing
   */

  // for interleaving we need to take all 10 symbols into consideration
  int n_bits = 10*SRSLTE_NRE*n_prb*srslte_mod_bits_x_symbol(sci->mcs.mod);

  // Generate interleaver table
  // @todo: generate a interleaver table which is independent of modulation-order
  interleaver_table_gen(q, sci->mcs.mod, n_bits);
  
  // de-interleaving
  for(int i=0; i<n_bits; i++) {
    e[q->interleaver_lut[i]] = h[i];
  }

  // rate-dematching and turbo-decoding
  int ret = srslte_slsch_decode(&q->dl_sch, sci, softbuffers[0], q->e[0], n_bits, data[0], 0);

  q->last_nof_iterations[0] = srslte_sch_last_noi(&q->dl_sch);

  return ret;
}


#if 0
/** Decodes the PDSCH from the received symbols
 */
int srslte_pssch_decode(srslte_pssch_t *q,
                        srslte_pssch_cfg_t *cfg, srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                        cf_t *sf_symbols[SRSLTE_MAX_PORTS], cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                        float noise_estimate, uint16_t rnti, uint8_t *data[SRSLTE_MAX_CODEWORDS],
                        bool acks[SRSLTE_MAX_CODEWORDS])
{

  /* Set pointers for layermapping & precoding */
  uint32_t i;
  cf_t *x[SRSLTE_MAX_LAYERS];

  if (q            != NULL &&
      sf_symbols   != NULL &&
      data         != NULL &&
      cfg          != NULL)
  {
    uint32_t nof_tb = SRSLTE_RA_DL_GRANT_NOF_TB(&cfg->grant);

    INFO("Decoding PDSCH SF: %d, RNTI: 0x%x, NofSymbols: %d, C_prb=%d, mimo_type=%s, nof_layers=%d, nof_tb=%d\n",
        cfg->sf_idx, rnti, cfg->nbits[0].nof_re, cfg->grant.nof_prb, srslte_mod_string(cfg->grant.mcs->mod), cfg->nof_layers, nof_tb);

    // Extract Symbols and Channel Estimates
    for (int j=0;j<q->nof_rx_antennas;j++) {
      int n = srslte_pssch_get(q, sf_symbols[j], q->symbols[j], &cfg->grant, cfg->nbits[0].lstart, cfg->sf_idx);
      if (n != cfg->nbits[0].nof_re) {
        fprintf(stderr, "Error expecting %d symbols but got %d\n", cfg->nbits[0].nof_re, n);
        return SRSLTE_ERROR;
      }

      for (i = 0; i < q->cell.nof_ports; i++) {
        n = srslte_pssch_get(q, ce[i][j], q->ce[i][j], &cfg->grant, cfg->nbits[0].lstart, cfg->sf_idx);
        if (n != cfg->nbits[0].nof_re) {
          fprintf(stderr, "Error expecting %d symbols but got %d\n", cfg->nbits[0].nof_re, n);
          return SRSLTE_ERROR;
        }
      }
    }

    // Prepare layers
    int nof_symbols [SRSLTE_MAX_CODEWORDS];
    nof_symbols[0] = cfg->nbits[0].nof_re * nof_tb / cfg->nof_layers;
    nof_symbols[1] = cfg->nbits[1].nof_re * nof_tb / cfg->nof_layers;

    if (cfg->nof_layers == nof_tb) {
      /* Skip layer demap */
      for (i = 0; i < cfg->nof_layers; i++) {
        x[i] = q->d[i];
      }
    } else {
      /* number of layers equals number of ports */
      for (i = 0; i < cfg->nof_layers; i++) {
        x[i] = q->x[i];
      }
      memset(&x[cfg->nof_layers], 0, sizeof(cf_t*) * (SRSLTE_MAX_LAYERS - cfg->nof_layers));
    }

    float pdsch_scaling = 1.0f;
    if (q->rho_a != 0.0f) {
      pdsch_scaling = q->rho_a;
    }

    // Pre-decoder
    if (srslte_predecoding_type(q->symbols, q->ce, x, q->csi, q->nof_rx_antennas, q->cell.nof_ports, cfg->nof_layers,
                                      cfg->codebook_idx, cfg->nbits[0].nof_re, cfg->mimo_type, pdsch_scaling, noise_estimate)<0) {
      DEBUG("Error predecoding\n");
      return SRSLTE_ERROR;
    }

    // Layer demapping only if necessary
    if (cfg->nof_layers != nof_tb) {
        srslte_layerdemap_type(x, q->d, cfg->nof_layers, nof_tb,
                             nof_symbols[0], nof_symbols, cfg->mimo_type);
    }

    /* Codeword decoding: Implementation of 3GPP 36.212 Table 5.3.3.1.5-1 and Table 5.3.3.1.5-2 */
    uint32_t cw_idx = (nof_tb == SRSLTE_MAX_TB && cfg->tb_cw_swap) ? 1 : 0;
    for (uint32_t tb_idx = 0; tb_idx < SRSLTE_MAX_TB; tb_idx++) {
      /* Decode only if transport block is enabled and the default ACK is not true */
      if (cfg->grant.tb_en[tb_idx]) {
        if (!acks[tb_idx]) {
          int ret = SRSLTE_SUCCESS;
          if (SRSLTE_RA_DL_GRANT_NOF_TB(&cfg->grant) > 1 && tb_idx == 0 && q->coworker_ptr) {
            srslte_pssch_coworker_t *h = (srslte_pssch_coworker_t *) q->coworker_ptr;

            h->pdsch_ptr = q;
            h->cfg = cfg;
            h->softbuffer = softbuffers[tb_idx];
            h->rnti = rnti;
            h->data = data[tb_idx];
            h->cw_idx = cw_idx;
            h->tb_idx = tb_idx;
            h->ack = &acks[tb_idx];
            h->dl_sch.max_iterations = q->dl_sch.max_iterations;
            h->started = true;
            sem_post(&h->start);

          } else {
            ret = srslte_pssch_codeword_decode(q,
                                               cfg,
                                               &q->dl_sch,
                                               softbuffers[tb_idx],
                                               rnti,
                                               data[tb_idx],
                                               cw_idx,
                                               tb_idx,
                                               &acks[tb_idx]);
          }

          /* Check if there has been any execution error */
          if (ret) {
            /* Do Nothing */
          }
        }

        cw_idx = (cw_idx + 1) % SRSLTE_MAX_CODEWORDS;
      }
    }

    if (q->coworker_ptr) {
      srslte_pssch_coworker_t *h = (srslte_pssch_coworker_t *) q->coworker_ptr;
      if (h->started) {
        int err = sem_wait(&h->finish);
        if (err) {
          printf("SCH coworker: %s (nof_tb=%d)\n", strerror(errno), SRSLTE_RA_DL_GRANT_NOF_TB(&cfg->grant));
        }
        if (h->ret_status) {
          ERROR("PDSCH Coworker Decoder: Error decoding");
        }

        h->started = false;
      }
    }

    pdsch_decode_debug(q, cfg, sf_symbols, ce);

    return SRSLTE_SUCCESS;
  } else {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
}
#endif 

int srslte_pssch_pmi_select(srslte_pssch_t *q,
                                  srslte_pssch_cfg_t *cfg,
                                  cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS], float noise_estimate, uint32_t nof_ce,
                                  uint32_t pmi[SRSLTE_MAX_LAYERS], float sinr[SRSLTE_MAX_LAYERS][SRSLTE_MAX_CODEBOOKS]) {

  if (q->cell.nof_ports == 2 && q->nof_rx_antennas <= 2) {
    int nof_layers = 1;
    for (; nof_layers <= q->nof_rx_antennas; nof_layers++ ) {
      if (sinr[nof_layers - 1] && pmi) {
        if (srslte_precoding_pmi_select(ce, nof_ce, noise_estimate, nof_layers, &pmi[nof_layers - 1],
                                        sinr[nof_layers - 1]) < 0) {
          ERROR("PMI Select for %d layers", nof_layers);
          return SRSLTE_ERROR;
        }
      }
    }

    /* FIXME: Set other layers to 0 */
    for (; nof_layers <= SRSLTE_MAX_LAYERS; nof_layers++ ) {
      if (sinr[nof_layers - 1] && pmi) {
        for (int cb = 0; cb < SRSLTE_MAX_CODEBOOKS; cb++) {
          sinr[nof_layers - 1][cb] = -INFINITY;
        }
        pmi[nof_layers - 1] = 0;
      }
    }
  } else {
    DEBUG("Not implemented configuration");
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  return SRSLTE_SUCCESS;
}

int srslte_pssch_cn_compute(srslte_pssch_t *q,
                            cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS], uint32_t nof_ce, float *cn) {
  return srslte_precoding_cn(ce, q->cell.nof_ports, q->nof_rx_antennas, nof_ce, cn);
}

#if 0
int srslte_pssch_encode(srslte_pssch_t *q,
                        srslte_pssch_cfg_t *cfg, srslte_softbuffer_tx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                        uint8_t *data[SRSLTE_MAX_CODEWORDS], uint16_t rnti, cf_t *sf_symbols[SRSLTE_MAX_PORTS])
{

  int i;
  /* Set pointers for layermapping & precoding */
  cf_t *x[SRSLTE_MAX_LAYERS];
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL &&
      cfg != NULL) {
    uint32_t nof_tb = SRSLTE_RA_DL_GRANT_NOF_TB(&cfg->grant);


    for (i = 0; i < q->cell.nof_ports; i++) {
      if (sf_symbols[i] == NULL) {
        return SRSLTE_ERROR_INVALID_INPUTS;
      }
    }

    /* If both transport block size is zero return error */
    if (!nof_tb) {
      return SRSLTE_ERROR_INVALID_INPUTS;
    }

    if (cfg->nbits[0].nof_re > q->max_re || cfg->nbits[1].nof_re > q->max_re) {
      fprintf(stderr,
              "Error too many RE per subframe (%d). PDSCH configured for %d RE (%d PRB)\n",
              cfg->nbits[0].nof_re, q->max_re, q->cell.nof_prb);
      return SRSLTE_ERROR_INVALID_INPUTS;
    }

    /* Implementation of 3GPP 36.212 Table 5.3.3.1.5-1 and Table 5.3.3.1.5-2 */
    uint32_t cw_idx = (nof_tb == SRSLTE_MAX_TB && cfg->tb_cw_swap) ? 1 : 0;
    for (uint32_t tb_idx = 0; tb_idx < SRSLTE_MAX_TB; tb_idx++) {
      if (cfg->grant.tb_en[tb_idx]) {
        ret |= srslte_pssch_codeword_encode(q, cfg, softbuffers[tb_idx], rnti, data[tb_idx], cw_idx, tb_idx);
        cw_idx = (cw_idx + 1) % SRSLTE_MAX_CODEWORDS;
      }
    }

    /* Set scaling configured by Power Allocation */
    float scaling = 1.0f;
    if (q->rho_a != 0.0f) {
      scaling = q->rho_a;
    }

    // Layer mapping & precode if necessary
    if (q->cell.nof_ports > 1) {
      int nof_symbols;
      /* If number of layers is equal to transport blocks (codewords) skip layer mapping */
      if (cfg->nof_layers == nof_tb) {
        for (i = 0; i < cfg->nof_layers; i++) {
          x[i] = q->d[i];
        }
        nof_symbols = cfg->nbits[0].nof_re;
      } else {
        /* Initialise layer map pointers */
        for (i = 0; i < cfg->nof_layers; i++) {
          x[i] = q->x[i];
        }
        memset(&x[cfg->nof_layers], 0, sizeof(cf_t *) * (SRSLTE_MAX_LAYERS - cfg->nof_layers));

        nof_symbols = srslte_layermap_type(q->d, x, nof_tb, cfg->nof_layers,
                                           (int[SRSLTE_MAX_CODEWORDS]) {cfg->nbits[0].nof_re, cfg->nbits[1].nof_re},
                                           cfg->mimo_type);
      }

      /* Precode */
      srslte_precoding_type(x, q->symbols, cfg->nof_layers, q->cell.nof_ports, cfg->codebook_idx,
                            nof_symbols, scaling, cfg->mimo_type);
    } else {
      if (scaling == 1.0f) {
        memcpy(q->symbols[0], q->d[0], cfg->nbits[0].nof_re * sizeof(cf_t));
      } else {
        srslte_vec_sc_prod_cfc(q->d[0], scaling, q->symbols[0], cfg->nbits[0].nof_re);
      }
    }

    /* mapping to resource elements */
    for (i = 0; i < q->cell.nof_ports; i++) {
      srslte_pssch_put(q, q->symbols[i], sf_symbols[i], &cfg->grant, cfg->nbits[0].lstart, cfg->sf_idx);
    }

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}
#endif


/**
 * @brief Does transport and pyh processing for one transport block
 * 
 * @param q 
 * @param sci           sidelink control information
 * @param softbuffers   provided buffer to save softbuffer
 * @param sf_symbols    buffer to subframe
 * @param prb_offset    PRB offset of channel
 * @param n_prb         nubmer of PRBs
 * @param data          input data, hardbits i.e. 8 hard-bit/byte
 * @return int          SRSLTE_SUCCESS on success
 */
int srslte_pssch_encode_simple(srslte_pssch_t *q,
                        srslte_ra_sl_sci_t *sci,
                        srslte_softbuffer_tx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                        cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                        uint32_t prb_offset,
                        uint32_t n_prb,
                        uint8_t *data[SRSLTE_MAX_CODEWORDS])
{
  int i;
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q == NULL || softbuffers[0] == NULL) {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  for (i = 0; i < q->cell.nof_ports; i++) {
    if (sf_symbols[i] == NULL) {
      return SRSLTE_ERROR_INVALID_INPUTS;
    }
  }

  uint32_t nof_re = 10*(n_prb)*SRSLTE_NRE;
  uint32_t E = nof_re*srslte_mod_bits_x_symbol(sci->mcs.mod);

  INFO("Encoding PSSCH SF: %d (TB%d -> CW%d), Mod %s, NofBits: %d, NofSymbols: %d, NofBitsE: %d\n",
        q->n_PSSCH_ssf,//cfg->sf_idx,
        0,//tb_idx,
        0,//codeword_idx,
        srslte_mod_string(sci->mcs.mod), sci->mcs.tbs,
        nof_re, E);

  // channel coding
  if(SRSLTE_SUCCESS != srslte_slsch_encode(&q->dl_sch,
                                            sci,//srslte_ra_sl_sci_t *sci,
                                            softbuffers[0],//srslte_softbuffer_tx_t *softbuffer,
                                            data[0],//uint8_t *data,
                                            q->e[0],// uint8_t *e_bits,
                                            E)) {
    printf("srslte_slsch_encode failed.\n");
    return SRSLTE_ERROR;
  }

  // Generate interleaver table
  interleaver_table_gen(q, sci->mcs.mod, E);

  // y = x(lut)
  srslte_bit_interleave_w_offset(q->e[0], q->h[0], q->interleaver_lut, E, 0);

  // PHY processing: from here on we only need to process 9 sc-fdma symbols
  nof_re = 9 * n_prb * SRSLTE_NRE;
  E = nof_re * srslte_mod_bits_x_symbol(sci->mcs.mod);

  // srcambling
  srslte_sequence_pssch(&q->tmp_seq, E, q->n_X_ID, q->n_PSSCH_ssf);

  srslte_scrambling_bytes(&q->tmp_seq, q->h[0], E);

  // modulation
  srslte_mod_modulate_bytes(&q->mod[sci->mcs.mod],
                              (uint8_t *) q->h[0],
                              q->d[0], E);

  // transform precoding 9.3.4
  srslte_dft_precoding(&q->dft_precoding, q->d[0], q->d[0],
                        n_prb,//nof_prb
                        9);//nof_symbols

  // @todo: do we need to apply some kind of scaling ?
  //srslte_vec_sc_prod_cfc(q->d[0], scaling, q->symbols[0], cfg->nbits[0].nof_re);
  memcpy(q->symbols[0], q->d[0], nof_re * sizeof(cf_t));

  // signal mapping
  for (i = 0; i < q->cell.nof_ports; i++) {
    srslte_pscch_put(q->symbols[i], sf_symbols[i], q->cell, prb_offset, n_prb);
  }

  ret = SRSLTE_SUCCESS;

  return ret;
}


void srslte_pssch_set_max_noi(srslte_pssch_t *q, uint32_t max_iter) {
  srslte_sch_set_max_noi(&q->dl_sch, max_iter);
}

float srslte_pssch_last_noi(srslte_pssch_t *q) {
  float niters = 0;
  int   active_cw = 0;
  for (int i=0;i<SRSLTE_MAX_CODEWORDS;i++) {
    if (q->last_nof_iterations[i]) {
      niters += q->last_nof_iterations[i];
      active_cw++;
    }
  }
  if (active_cw) {
    return niters/active_cw;
  } else {
    return 0;
  }
}

int srslte_pssch_enable_coworker(srslte_pssch_t *q) {
  int ret = SRSLTE_SUCCESS;

  if (!q->coworker_ptr) {
    srslte_pssch_coworker_t *h = calloc(sizeof(srslte_pssch_coworker_t), 1);

    if (!h) {
      ERROR("Allocating coworker");
      ret = SRSLTE_ERROR;
      goto clean;
    }
    q->coworker_ptr = h;

    if (srslte_sch_init(&h->dl_sch)) {
      ERROR("Initiating DL SCH");
      ret = SRSLTE_ERROR;
      goto clean;
    }

    if (sem_init(&h->start, 0, 0)) {
      ERROR("Creating semaphore");
      ret = SRSLTE_ERROR;
      goto clean;
    }
    if (sem_init(&h->finish, 0, 0)) {
      ERROR("Creating semaphore");
      ret = SRSLTE_ERROR;
      goto clean;
    }
    printf("ERROR: No coworker enabled for pssch decoding");
    ret = SRSLTE_ERROR;
    goto clean;
    //pthread_create(&h->pthread, NULL, srslte_pssch_decode_thread, (void *) h);
  }

  clean:
  if (ret) {
    srslte_pssch_disable_coworker(q);
  }
  return ret;
}

uint32_t srslte_pssch_last_noi_cw(srslte_pssch_t *q, uint32_t cw_idx) {
  return q->last_nof_iterations[cw_idx];
}


  
