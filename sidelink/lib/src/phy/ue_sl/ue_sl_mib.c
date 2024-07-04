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

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>

#include "srslte/phy/ue_sl/ue_sl_mib.h"

#include "srslte/phy/utils/debug.h"
#include "srslte/phy/utils/vector.h"

#define MIB_BUFFER_MAX_SAMPLES (3 * SRSLTE_SF_LEN_PRB(SRSLTE_UE_SL_MIB_NOF_PRB))

int srslte_ue_sl_mib_init(srslte_ue_sl_mib_t * q,
                       cf_t *in_buffer[SRSLTE_MAX_PORTS],
                       uint32_t max_prb)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL)
  {

    ret = SRSLTE_ERROR;    
    bzero(q, sizeof(srslte_ue_sl_mib_t));
    
    if (srslte_psbch_init(&q->psbch)) {
      fprintf(stderr, "Error initiating PBCH\n");
      goto clean_exit;
    }

    if (srslte_pscch_init_ue(&q->pscch, max_prb, 1)) {
      fprintf(stderr, "Error initiating PSCCH\n");
      goto clean_exit;
    }

    if (srslte_pssch_init_ue(&q->pssch, max_prb, 1)) {
      fprintf(stderr, "Error creating PSSCH object\n");
      goto clean_exit;
    }

    for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
      q->softbuffers[i] = calloc(sizeof(srslte_softbuffer_rx_t), 1);
      if (!q->softbuffers[i]) {
        fprintf(stderr, "Error allocating RX soft buffer\n");
        exit(-1);
      }

      if (srslte_softbuffer_rx_init(q->softbuffers[i], max_prb)) {
        fprintf(stderr, "Error initiating RX soft buffer\n");
        exit(-1);
      }
    }

    q->sf_symbols = srslte_vec_malloc(SRSLTE_SF_LEN_RE(max_prb, SRSLTE_CP_NORM) * sizeof(cf_t));
    if (!q->sf_symbols) {
      perror("malloc");
      goto clean_exit;
    }
    
    q->ce = srslte_vec_malloc(SRSLTE_SF_LEN_RE(max_prb, SRSLTE_CP_NORM) * sizeof(cf_t));
    if (!q->ce) {
      perror("malloc");
      goto clean_exit;
    }
    bzero(q->ce, SRSLTE_SF_LEN_RE(max_prb, SRSLTE_CP_NORM) * sizeof(cf_t));

    q->ce_plot = srslte_vec_malloc(max_prb * SRSLTE_NRE * sizeof(cf_t));
    if (!q->ce_plot) {
      perror("malloc");
      goto clean_exit;
    }
    bzero(q->ce_plot, max_prb * sizeof(cf_t));

    q->td_plot = srslte_vec_malloc(SRSLTE_SF_LEN_MAX * sizeof(cf_t));
    if (!q->td_plot) {
      perror("malloc");
      goto clean_exit;
    }
    bzero(q->td_plot, SRSLTE_SF_LEN_MAX * sizeof(cf_t));

    if (srslte_ofdm_rx_init(&q->fft, SRSLTE_CP_NORM, in_buffer[0], q->sf_symbols, max_prb)) {
      fprintf(stderr, "Error initializing FFT\n");
      goto clean_exit;
    }
    srslte_ofdm_set_normalize(&q->fft, false);
    srslte_ofdm_set_freq_shift(&q->fft, -0.5);

    if (srslte_chest_sl_init(&q->chest, max_prb)) {
      fprintf(stderr, "Error initializing reference signal\n");
      goto clean_exit;
    }
    srslte_ue_sl_mib_reset(q);
    
    ret = SRSLTE_SUCCESS;
  }

clean_exit:
  if (ret == SRSLTE_ERROR) {
    srslte_ue_sl_mib_free(q);
  }
  return ret;
}

void srslte_ue_sl_mib_free(srslte_ue_sl_mib_t * q)
{
  if (q->sf_symbols) {
    free(q->sf_symbols);
  }
  if (q->ce) {
    free(q->ce);
  }
  if (q->ce_plot) {
    free(q->ce_plot);
  }
  if (q->td_plot) {
    free(q->td_plot);
  }
  srslte_sync_free(&q->sfind);
  srslte_chest_sl_free(&q->chest);
  srslte_psbch_free(&q->psbch);
  srslte_pscch_free(&q->pscch);
  srslte_pssch_free(&q->pssch);
  srslte_ofdm_rx_free(&q->fft);

  for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    srslte_softbuffer_rx_free(q->softbuffers[i]);
    if (q->softbuffers[i]) {
      free(q->softbuffers[i]);
    }
  }
    
  bzero(q, sizeof(srslte_ue_sl_mib_t));
    
}

int srslte_ue_sl_mib_set_cell(srslte_ue_sl_mib_t * q,
                           srslte_cell_t cell)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL &&
      cell.nof_ports <= SRSLTE_MAX_PORTS)
  {
    if (srslte_psbch_set_cell(&q->psbch, cell)) {
      fprintf(stderr, "Error initiating PBCH\n");
      return SRSLTE_ERROR;
    }

    if (srslte_pscch_set_cell(&q->pscch, cell)) {
      fprintf(stderr, "Error initiating PCCH\n");
      return SRSLTE_ERROR;
    }

    if (srslte_pssch_set_cell(&q->pssch, cell)) { 
      fprintf(stderr, "Error creating PSSCH object\n");
      return SRSLTE_ERROR;
    }

    // move fft window into CP
    if (srslte_ofdm_set_cp_shift(&q->fft, SRSLTE_CP_LEN(srslte_symbol_sz(cell.nof_prb), SRSLTE_CP_NORM_0_LEN) / 4)) {
      fprintf(stderr, "Error moving FFT window into CP\n");
      return SRSLTE_ERROR;
    }

    if (srslte_ofdm_rx_set_prb(&q->fft, cell.cp, cell.nof_prb)) {
      fprintf(stderr, "Error initializing FFT\n");
      return SRSLTE_ERROR;
    }

    if (cell.nof_ports == 0) {
      cell.nof_ports = SRSLTE_MAX_PORTS;
    }

    if (srslte_chest_sl_set_cell(&q->chest, cell)) {
      fprintf(stderr, "Error initializing reference signal\n");
      return SRSLTE_ERROR;
    }
    srslte_ue_sl_mib_reset(q);

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}


void srslte_ue_sl_mib_reset(srslte_ue_sl_mib_t * q)
{
  q->frame_cnt = 0;
}

int srslte_ue_sl_mib_decode(srslte_ue_sl_mib_t * q,
                  uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN], uint32_t *nof_tx_ports, int *sfn_offset)
{
  int ret = SRSLTE_SUCCESS;

  /* Run FFT for the slot symbols */
  srslte_ofdm_rx_sf(&q->fft);

  // get channel estimates for psbch
  // ret = srslte_chest_sl_estimate_psbch(&q->chest, q->sf_symbols, q->ce, SRSLTE_SL_MODE_4);
  ret = srslte_chest_sl_estimate_psbch(&q->chest, q->sf_symbols, q->ce, SRSLTE_SL_MODE_4, 0, 0); 
  if (ret < 0) {
    return SRSLTE_ERROR;
  }
  // /* Reset decoder if we missed a frame */
  // if (q->frame_cnt > 8) {
  //   INFO("Resetting PBCH decoder after %d frames\n", q->frame_cnt);
  //   srslte_ue_mib_reset(q);
  // }
  
  /* Decode PSBCH */
  ret = srslte_psbch_decode(&q->psbch, q->sf_symbols, q->ce, q->chest.noise_estimate, bch_payload);


  if (ret < 0) {
    fprintf(stderr, "Error decoding PSBCH (%d)\n", ret);
    ret = SRSLTE_UE_SL_MIB_NOTFOUND; 
  } else if (ret == 1) {
    INFO("MIB decoded: %u\n", q->frame_cnt);
    srslte_ue_sl_mib_reset(q);
    ret = SRSLTE_UE_SL_MIB_FOUND; 
  } else {
    ret = SRSLTE_UE_SL_MIB_NOTFOUND;
    INFO("MIB not decoded: %u\n", q->frame_cnt);
    q->frame_cnt++;
  }    
  
  return ret;
}



/**
 * Internal function for PSSCH decoding.
 * 
 * Does single as well as combined/harq decoding.
 * 
 */
static int sl_pssch_decode(srslte_ue_sl_mib_t * q,
                  srslte_repo_t *repo,
                  srslte_ra_sl_sci_t sci,
                  uint8_t *decoded,
                  int *n_decoded_bytes)
{
  int ret = SRSLTE_SUCCESS;
  cf_t * ce[4];

  ce[0] = q->ce;

  uint32_t prb_offset = repo->rp.startRB_Subchannel_r14 + sci.frl_n_subCH*repo->rp.sizeSubchannel_r14;

  {

    // generate pssch dmrs
    srslte_refsignal_sl_dmrs_pssch_gen(&q->chest.dmrs_signal,
                                        sci.frl_L_subCH*repo->rp.sizeSubchannel_r14 - 2,
                                        q->pssch.n_PSSCH_ssf,
                                        q->pssch.n_X_ID, // CRC checksum
                                        q->chest.pilot_known_signal);


    srslte_chest_sl_estimate_pssch(&q->chest, q->sf_symbols, ce[0], SRSLTE_SL_MODE_4,
                                    prb_offset + 2,
                                    sci.frl_L_subCH*repo->rp.sizeSubchannel_r14 - 2);

    printf("SL-SCH n0 %f\n", srslte_chest_sl_get_noise_estimate(&q->chest));
    printf("RSRP: %f dBm\n", 10*log10(q->chest.pilot_power) + 30 - q->fft.fft_plan.norm*10*log10(q->fft.fft_plan.size));
    printf("SNR: %f dB\t%f dB\t%f dB\n",
            10*log10((q->chest.pilot_power-srslte_chest_sl_get_noise_estimate(&q->chest))/srslte_chest_sl_get_noise_estimate(&q->chest)),
            10*log10(q->chest.ce_power_estimate / q->chest.noise_estimate),
            10*log10(q->chest.pilot_power / q->chest.noise_estimate));
    
    
    cf_t *_sf_symbols[SRSLTE_MAX_PORTS]; 
    cf_t *_ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];

    _sf_symbols[0] = q->sf_symbols; 
    for (int i=0;i<q->pscch.cell.nof_ports;i++) {
      _ce[i][0] = ce[i]; 
    }

    srslte_softbuffer_rx_reset_tbs(q->softbuffers[0], (uint32_t) sci.mcs.tbs);

    // only reset combining buffer on each initial transmission
    if(sci.rti == 0) {
      srslte_softbuffer_rx_reset_tbs(q->softbuffers[1], (uint32_t) sci.mcs.tbs);
    }

    // allocate buffer to store decoded data, tbs size in bytes plus crc 
    uint8_t *data_rx[SRSLTE_MAX_CODEWORDS];
    data_rx[0] = decoded;
    memset(data_rx[0], 0xFF, sizeof(uint8_t) * sci.mcs.tbs/8 + 3);

    struct timeval t[3]; 
    gettimeofday(&t[1], NULL);

    // first decoding attempt is made on first buffer, which is always reset
    int decode_ret = srslte_pssch_decode_simple(&q->pssch,
                                                &sci,
                                                NULL,//&pdsch_cfg,//srslte_pssch_cfg_t *cfg,
                                                &q->softbuffers[0],//q->softbuffers,//srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                                                _sf_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                                                _ce,//_ce,//cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                                                srslte_chest_sl_get_noise_estimate(&q->chest),//float noise_estimate,
                                                prb_offset + 2,
                                                sci.frl_L_subCH*repo->rp.sizeSubchannel_r14 - 2,
                                                data_rx);//uint8_t *data[SRSLTE_MAX_CODEWORDS],

    gettimeofday(&t[2], NULL);
    get_time_interval(t);

    printf("First decoding took %ld us and %d iterations\n", t[0].tv_usec, q->pssch.last_nof_iterations[0]);

    if(SRSLTE_SUCCESS == decode_ret) {
      printf("HARQ: single decoding successful\n");
    }

    // we need keep track if this tb was already decoded and therefore reported as success
    bool harq_already_decoded = q->softbuffers[1]->tb_crc;

    gettimeofday(&t[1], NULL);

    // second decoding attempt is made on second buffer
    int decode_ret_c = srslte_pssch_decode_simple(&q->pssch,
                                                &sci,
                                                NULL,//&pdsch_cfg,//srslte_pssch_cfg_t *cfg,
                                                &q->softbuffers[1],//q->softbuffers,//srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                                                _sf_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                                                _ce,//_ce,//cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                                                srslte_chest_sl_get_noise_estimate(&q->chest),//float noise_estimate,
                                                prb_offset + 2,
                                                sci.frl_L_subCH*repo->rp.sizeSubchannel_r14 - 2,
                                                data_rx);//uint8_t *data[SRSLTE_MAX_CODEWORDS],

    gettimeofday(&t[2], NULL);
    get_time_interval(t);

    printf("Combined decoding took %ld us and %d iterations\n", t[0].tv_usec, q->pssch.last_nof_iterations[0]);

    if(SRSLTE_SUCCESS == decode_ret_c) {
      printf("HARQ: combined decoding successful. Repeated: %d\n", harq_already_decoded);
    } else {
      printf("HARQ: Combined decoding failed with code: %x, ran %d iterations.\n", decode_ret_c, q->pssch.last_nof_iterations[0]);
    }

    // reset buffer, for future decodings
    if(sci.rti == 1) {
      srslte_softbuffer_rx_reset_tbs(q->softbuffers[1], (uint32_t) sci.mcs.tbs);
    }

    if(SRSLTE_SUCCESS == decode_ret || SRSLTE_SUCCESS == decode_ret_c) {
      ret = SRSLTE_SUCCESS;

      srslte_vec_fprint_byte(stdout, data_rx[0], sci.mcs.tbs/8 + 3);

      *n_decoded_bytes = sci.mcs.tbs/8;

      return ret;
    }

  }

  return ret;
}



/**
 * @brief Decodes PSSCH, based on provided SCI-1 bits
 * 
 * Actually this should be in a seperate ue_* but for the sake of running
 * out of time we put it here.
 * 
 * This functionality is used by our simulation interface, so measure
 * PSSCH decoding performance without possible PSCCH errors.
 * 
 * @param q 
 * @param repo 
 * @param sci_bits
 * @param decoded 
 * @param n_decoded_bytes 
 * @return int 
 */
int srslte_ue_sl_pssch_decode(srslte_ue_sl_mib_t * q,
                  srslte_repo_t *repo,
                  uint8_t *sci_bits,
                  uint8_t *decoded,
                  int *n_decoded_bytes)
{
  srslte_ra_sl_sci_t sci;
  uint16_t crc_rem = 0xdead;

  *n_decoded_bytes = 0;

  /* Run FFT for the slot symbols */
  srslte_ofdm_rx_sf(&q->fft);

  if(SRSLTE_SUCCESS != srslte_repo_sci_decode(repo, sci_bits, &sci)) {
    printf("External SCI-BITS are invalid.\n");
    return 0;
  }

  crc_rem = ((uint16_t) srslte_crc_checksum(&q->pscch.crc, sci_bits, SRSLTE_SCI1_MAX_BITS) & 0xffff);

  printf("SCI-BITS from file: N_X_ID: %x\n", crc_rem);

  // set parameters for pssch
  q->pssch.n_X_ID = crc_rem;
  q->pssch.n_PSSCH_ssf = 0; //@todo, make dynamically when sfn is detected 

  // decode pssch with sci-1 information
  return sl_pssch_decode(q, repo, sci, decoded, n_decoded_bytes);
}



/**
 * @brief Trys to find a valid PSCCH and decodes the adjacent PSSCH
 * 
 * Actually this should be in a seperate ue_* but for the sake of running
 * out of time we put it here.
 * 
 * Takes the retransmission entry from SCI into consideration and combines
 * the decodings from previous transmission and a current as retransmission
 * indicated transportblock. We require, that both transmission occur in
 * adjacent subframes/function calls. This is the case for the sl_snr_file_dec
 * which simulates the performance of our harq combining.
 * 
 * @param q 
 * @param repo 
 * @param decoded 
 * @param n_decoded_bytes 
 * @return int 
 */
int srslte_ue_sl_pscch_decode(srslte_ue_sl_mib_t * q,
                  srslte_repo_t *repo,
                  uint8_t *decoded,
                  int *n_decoded_bytes)
{
  int ret = SRSLTE_SUCCESS;
  uint8_t mdata[SRSLTE_SCI1_MAX_BITS + 16];
  uint16_t crc_rem = 0xdead;
  srslte_ra_sl_sci_t sci;
  cf_t * ce[4];

  /* Run FFT for the slot symbols */
  srslte_ofdm_rx_sf(&q->fft);


  float rssi_time = (10 * log10(srslte_vec_avg_power_cf(q->fft.in_buffer, SRSLTE_SF_LEN_PRB(q->psbch.cell.nof_prb))) + 30);
  float rssi_freq = (10 * log10(srslte_vec_avg_power_cf(q->sf_symbols, q->psbch.cell.nof_prb*SRSLTE_NRE*14)) + 30);

  // apply normalization
  rssi_freq -= q->fft.fft_plan.norm ? 0.0 : 10*log10(q->fft.fft_plan.size);

  ce[0] = q->ce;
  *n_decoded_bytes = 0;

  // try to decode PSCCH for each subchannel
  for(int rbp=0; rbp<repo->rp.numSubchannel_r14; rbp++) {

    uint32_t prb_offset = repo->rp.startRB_Subchannel_r14 + rbp*repo->rp.sizeSubchannel_r14;

    srslte_chest_sl_estimate_pscch(&q->chest, q->sf_symbols, q->ce, SRSLTE_SL_MODE_4, prb_offset);

    if (srslte_pscch_extract_llr(&q->pscch, q->sf_symbols, 
                        ce,
                        srslte_chest_sl_get_noise_estimate(&q->chest),
                        0 %10, prb_offset)) {
      fprintf(stderr, "Error extracting LLRs\n");
      return -1;
    }

    if(SRSLTE_SUCCESS != srslte_pscch_dci_decode(&q->pscch, q->pscch.llr, mdata, q->pscch.max_bits, SRSLTE_SCI1_MAX_BITS, &crc_rem)) {
      continue;
    }

    if(SRSLTE_SUCCESS != srslte_repo_sci_decode(repo, mdata, &sci)) {
      continue;
    }

    printf("DECODED PSCCH  N_X_ID: %x on tti %d t_SL_k: %d (rbp:%d) SCI-1: frl: 0x%x(n: %d L: %d) gap: %d mcs: %d rti: %d\n",
            crc_rem, 0,
            srslte_repo_get_t_SL_k(repo, 0),
            rbp,
            sci.frl,
            sci.frl_n_subCH,
            sci.frl_L_subCH,
            sci.time_gap,
            sci.mcs.idx,
            sci.rti);

    printf("DECODED PSCCH  N_X_ID: %x  n0: %f\n", crc_rem, srslte_chest_sl_get_noise_estimate(&q->chest));

    printf("RSSI | t-domain: %f dBm | f-domain: %f dBm\n", rssi_time, rssi_freq);

    // set parameters for pssch
    q->pssch.n_X_ID = crc_rem;
    q->pssch.n_PSSCH_ssf = 0; //@todo, make dynamically when sfn is detected 

    // decode pssch with sci-1 information
    return sl_pssch_decode(q, repo, sci, decoded, n_decoded_bytes);
  }

  return ret;
}



int srslte_ue_sl_mib_sync_init_multi(srslte_ue_sl_mib_sync_t *q, 
                                  int (recv_callback)(void*, cf_t*[SRSLTE_MAX_PORTS], uint32_t, srslte_timestamp_t*),
                                  uint32_t nof_rx_antennas,
                                  void *stream_handler) 
{
  for (int i=0;i<nof_rx_antennas;i++) {
    q->sf_buffer[i] = srslte_vec_malloc(3*sizeof(cf_t)*SRSLTE_SF_LEN_PRB(SRSLTE_UE_SL_MIB_NOF_PRB));
  }
  q->nof_rx_antennas = nof_rx_antennas;
  
  if (srslte_ue_sl_mib_init(&q->ue_sl_mib, q->sf_buffer, SRSLTE_UE_SL_MIB_NOF_PRB)) {
    fprintf(stderr, "Error initiating ue_mib\n");
    return SRSLTE_ERROR;
  }
  if (srslte_ue_sl_sync_init_multi(&q->ue_sl_sync, SRSLTE_UE_SL_MIB_NOF_PRB, false, recv_callback, nof_rx_antennas, stream_handler)) {
    fprintf(stderr, "Error initiating ue_sync\n");
    srslte_ue_sl_mib_free(&q->ue_sl_mib);
    return SRSLTE_ERROR;
  }
  srslte_ue_sl_sync_decode_ssss_on_track(&q->ue_sl_sync, true);
  return SRSLTE_SUCCESS;
}

int srslte_ue_sl_mib_sync_set_cell(srslte_ue_sl_mib_sync_t *q,
                                    uint32_t cell_id,
                                    srslte_cp_t cp)
{
  srslte_cell_t cell;
  // If the ports are set to 0, ue_mib goes through 1, 2 and 4 ports to blindly detect nof_ports
  cell.nof_ports = 0;
  cell.id = cell_id;
  cell.cp = cp;
  cell.nof_prb = SRSLTE_UE_SL_MIB_NOF_PRB;

  if (srslte_ue_sl_mib_set_cell(&q->ue_sl_mib, cell)) {
    fprintf(stderr, "Error initiating ue_mib\n");
    return SRSLTE_ERROR;
  }
  if (srslte_ue_sl_sync_set_cell(&q->ue_sl_sync, cell)) {
    fprintf(stderr, "Error initiating ue_sl_sync\n");
    srslte_ue_sl_mib_free(&q->ue_sl_mib);
    return SRSLTE_ERROR;
  }
  return SRSLTE_SUCCESS;
}

void srslte_ue_sl_mib_sync_free(srslte_ue_sl_mib_sync_t *q) {
  for (int i=0;i<q->nof_rx_antennas;i++) {
    if (q->sf_buffer[i]) {
      free(q->sf_buffer[i]);
    }
  }
  srslte_ue_sl_mib_free(&q->ue_sl_mib);
  srslte_ue_sl_sync_free(&q->ue_sl_sync);
}

void srslte_ue_sl_mib_sync_reset(srslte_ue_sl_mib_sync_t * q) {
  srslte_ue_sl_mib_reset(&q->ue_sl_mib);
  srslte_ue_sl_sync_reset(&q->ue_sl_sync);
}

int srslte_ue_sl_mib_sync_decode(srslte_ue_sl_mib_sync_t * q, 
                                  uint32_t max_frames_timeout,
                                  uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN], 
                                  uint32_t *nof_tx_ports, 
                                  int *sfn_offset) 
{
  
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  uint32_t nof_frames = 0; 
  int mib_ret = SRSLTE_UE_SL_MIB_NOTFOUND;

  if (q != NULL)
  {
    srslte_ue_sl_mib_sync_reset(q);

    ret = SRSLTE_SUCCESS;
    do {
      mib_ret = SRSLTE_UE_SL_MIB_NOTFOUND; 
      ret = srslte_ue_sl_sync_zerocopy_multi(&q->ue_sl_sync, q->sf_buffer, MIB_BUFFER_MAX_SAMPLES);
      if (ret < 0) {
        fprintf(stderr, "Error calling srslte_ue_sync_work()\n");       
        return -1;
      } else if (srslte_ue_sl_sync_get_sfidx(&q->ue_sl_sync) == 0) {
        if (ret == 1) {
          mib_ret = srslte_ue_sl_mib_decode(&q->ue_sl_mib, bch_payload, nof_tx_ports, sfn_offset);
        } else {
          DEBUG("Resetting PSBCH decoder after %d frames\n", q->ue_sl_mib.frame_cnt);
          srslte_ue_sl_mib_reset(&q->ue_sl_mib);
        }
        nof_frames++;
      }
    } while (mib_ret == SRSLTE_UE_SL_MIB_NOTFOUND && ret >= 0 && nof_frames < max_frames_timeout);
    if (mib_ret < 0) {
      ret = mib_ret; 
    }
  }
  return mib_ret;  
}
