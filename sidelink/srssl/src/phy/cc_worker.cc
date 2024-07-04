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

/*
 * Copyright 2013-2019 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
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

#include "srslte/srslte.h"
#include "srssl/hdr/phy/cc_worker.h"
#include "srslte/interfaces/ue_interfaces.h"
#include <stdio.h>
#include <time.h> 

// added by Abhijeet  09/20/2023
void writeToCSV(const char *filename, int ue_id, double snr, double ema_snr, double rsrp, double ema_rsrp, 
                float noise_power, time_t rx_full_secs, float rx_frac_secs, float rssi_dBm, float rx_gain) {
    static int isFirstTime = 1;
    FILE *csvFile;
    
    if (isFirstTime) {
        csvFile = fopen(filename, "w");
        isFirstTime = 0;
        
        // Write the header
        fprintf(csvFile, "Timestamp,UE_ID,SNR,EMA_SNR,RSRP,EMA_RSRP,Noise_Power,RX_Full_Secs,RX_Frac_Secs,RSSI,RX_Gain\n");
    } else {
        csvFile = fopen(filename, "a");
    }

    if (csvFile == NULL) {
        printf("Error opening the CSV file.\n");
        return;
    }

  // Calculate current timestamp in epoch format
    time_t current_time = time(NULL);

    fprintf(csvFile, "%ld,%d,%f,%f,%f,%f,%f,%f,%f\n",
            current_time, ue_id, snr, ema_snr, rsrp, ema_rsrp, noise_power, rssi_dBm, rx_gain);
    fclose(csvFile);
}


#define Error(fmt, ...)                                                                                                \
  if (SRSLTE_DEBUG_ENABLED)                                                                                            \
  log_h->error(fmt, ##__VA_ARGS__)
#define Warning(fmt, ...)                                                                                              \
  if (SRSLTE_DEBUG_ENABLED)                                                                                            \
  log_h->warning(fmt, ##__VA_ARGS__)
#define Info(fmt, ...)                                                                                                 \
  if (SRSLTE_DEBUG_ENABLED)                                                                                            \
  log_h->info(fmt, ##__VA_ARGS__)
#define Debug(fmt, ...)                                                                                                \
  if (SRSLTE_DEBUG_ENABLED)                                                                                            \
  log_h->debug(fmt, ##__VA_ARGS__)

#define CURRENT_TTI (sf_cfg_dl.tti)
#define CURRENT_SFIDX (sf_cfg_dl.tti % 10)
#define CURRENT_TTI_TX (sf_cfg_ul.tti)

using namespace asn1::rrc;

namespace srsue {

uint32_t srsue::cc_worker::sps_rsrp_read_cnt = 0;
uint32_t srsue::cc_worker::sps_rssi_read_cnt = 0;

/************
 *
 * Common Functions
 *
 */

cc_worker::cc_worker(uint32_t cc_idx, uint32_t max_prb, srsue::phy_common* phy, srslte::log* log_h)
{
  ZERO_OBJECT(signal_buffer_rx);
  ZERO_OBJECT(signal_buffer_tx);
  ZERO_OBJECT(pending_dl_grant);
  ZERO_OBJECT(cell);
  ZERO_OBJECT(sf_cfg_dl);
  ZERO_OBJECT(sf_cfg_ul);
  ZERO_OBJECT(ue_dl);
  ZERO_OBJECT(ue_dl_cfg);
  ZERO_OBJECT(ue_dl_cfg.cfg.pdsch);
  ZERO_OBJECT(pmch_cfg);
  ZERO_OBJECT(chest_mbsfn_cfg);
  ZERO_OBJECT(chest_default_cfg);
  ZERO_OBJECT(ue_ul);
  ZERO_OBJECT(ue_ul_cfg);
  ZERO_OBJECT(dl_metrics);
  ZERO_OBJECT(ul_metrics);

  cell_initiated = false;

  this->cc_idx = cc_idx;
  this->phy    = phy;
  this->log_h  = log_h;

  signal_buffer_max_samples = 3 * SRSLTE_SF_LEN_PRB(max_prb);

  for (uint32_t i = 0; i < phy->args->nof_rx_ant; i++) {
    signal_buffer_rx[i] = srslte_vec_cf_malloc(signal_buffer_max_samples);
    if (!signal_buffer_rx[i]) {
      Error("Allocating memory\n");
      return;
    }
    signal_buffer_tx[i] = srslte_vec_cf_malloc(signal_buffer_max_samples);
    if (!signal_buffer_tx[i]) {
      Error("Allocating memory\n");
      return;
    }
  }

  if (srslte_ue_dl_init(&ue_dl, signal_buffer_rx, max_prb, phy->args->nof_rx_ant)) {
    Error("Initiating UE DL\n");
    return;
  }

  if (srslte_ue_ul_init(&ue_ul, signal_buffer_tx[0], max_prb)) {
    Error("Initiating UE UL\n");
    return;
  }

  if (srslte_ue_sl_mib_init(&ue_sl, signal_buffer_rx, max_prb)) {
    Error("Error initaiting UE MIB decoder\n");
    return;
  }

  if(srslte_ue_sl_tx_init(&ue_sl_tx, signal_buffer_tx[0], max_prb)) {
    Error("Error initiating ue_sl_tx object\n");
    return;
  }

  // enable cfo, in current srsLTE version this is done in phy->set_ue_ul_cfg(&ue_ul_cfg)
  srslte_ue_sl_tx_set_cfo_enable(&ue_sl_tx, true);

  phy->set_ue_dl_cfg(&ue_dl_cfg);
  phy->set_ue_ul_cfg(&ue_ul_cfg);
  phy->set_pdsch_cfg(&ue_dl_cfg.cfg.pdsch);
  phy->set_pdsch_cfg(&pmch_cfg.pdsch_cfg); // set same config in PMCH decoder

  // Define MBSFN subframes channel estimation and save default one
  chest_mbsfn_cfg.filter_type          = SRSLTE_CHEST_FILTER_TRIANGLE;
  chest_mbsfn_cfg.filter_coef[0]       = 0.1;
  chest_mbsfn_cfg.interpolate_subframe = true;
  chest_mbsfn_cfg.noise_alg            = SRSLTE_NOISE_ALG_PSS;

  chest_default_cfg = ue_dl_cfg.chest_cfg;

  if (phy->args->pdsch_8bit_decoder) {
    ue_dl.pdsch.llr_is_8bit        = true;
    ue_dl.pdsch.dl_sch.llr_is_8bit = true;
  }
  pregen_enabled = false;
}

cc_worker::~cc_worker()
{
  for (uint32_t i = 0; i < phy->args->nof_rx_ant; i++) {
    if (signal_buffer_tx[i]) {
      free(signal_buffer_tx[i]);
    }
    if (signal_buffer_rx[i]) {
      free(signal_buffer_rx[i]);
    }
  }
  srslte_ue_dl_free(&ue_dl);
  srslte_ue_ul_free(&ue_ul);

  srslte_ue_sl_mib_free(&ue_sl);
  srslte_ue_sl_tx_free(&ue_sl_tx);
}

void cc_worker::reset()
{
  bzero(&dl_metrics, sizeof(dl_metrics_t));
  bzero(&ul_metrics, sizeof(ul_metrics_t));

  phy_interface_rrc_lte::phy_cfg_t empty_cfg = {};
  // defaults
  empty_cfg.common.pucch_cnfg.delta_pucch_shift.value = pucch_cfg_common_s::delta_pucch_shift_opts::ds1;
  empty_cfg.common.ul_pwr_ctrl.alpha.value            = alpha_r12_opts::al0;
  empty_cfg.common.ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format1.value =
      delta_flist_pucch_s::delta_f_pucch_format1_opts::delta_f0;
  empty_cfg.common.ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format1b.value =
      delta_flist_pucch_s::delta_f_pucch_format1b_opts::delta_f1;
  empty_cfg.common.ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format2.value =
      delta_flist_pucch_s::delta_f_pucch_format2_opts::delta_f0;
  empty_cfg.common.ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format2a.value =
      delta_flist_pucch_s::delta_f_pucch_format2a_opts::delta_f0;
  empty_cfg.common.ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format2b.value =
      delta_flist_pucch_s::delta_f_pucch_format2b_opts::delta_f0;
  set_pcell_config(&empty_cfg);
}

bool cc_worker::set_cell(srslte_cell_t cell)
{
  if (this->cell.id != cell.id || !cell_initiated) {
    this->cell = cell;

    if (srslte_ue_dl_set_cell(&ue_dl, cell)) {
      Error("Initiating UE DL\n");
      return false;
    }

    if (srslte_ue_dl_set_mbsfn_area_id(&ue_dl, 1)) {
      Error("Setting mbsfn id\n");
    }

    if (srslte_ue_ul_set_cell(&ue_ul, cell)) {
      Error("Initiating UE UL\n");
      return false;
    }

    if (srslte_ue_sl_mib_set_cell(&ue_sl, cell)) {
      Error("Error initaiting UE MIB decoder\n");
      return false;
    }

    if(srslte_ue_sl_tx_set_cell(&ue_sl_tx, cell)) {
      Error("Error setting cell for ue_sl_tx object\n");
      return false;
    }

    // enable cfo, in current srsLTE version this is done in phy->set_ue_ul_cfg(&ue_ul_cfg)
    srslte_ue_sl_tx_set_cfo_enable(&ue_sl_tx, true);

    if (cell.frame_type == SRSLTE_TDD && !ue_dl_cfg.chest_cfg.interpolate_subframe) {
      chest_default_cfg.interpolate_subframe = true;
      log_h->console("Enabling subframe interpolation for TDD cells (recommended setting)\n");
    }

    cell_initiated = true;
  }
  return true;
}

uint32_t cc_worker::get_buffer_len()
{
  return signal_buffer_max_samples;
}

cf_t* cc_worker::get_rx_buffer(uint32_t antenna_idx)
{
  return signal_buffer_rx[antenna_idx];
}

cf_t* cc_worker::get_tx_buffer(uint32_t antenna_idx)
{
  return signal_buffer_tx[antenna_idx];
}

float cc_worker::get_last_agc()
{
  float ret = agc_max_value;
  agc_max_value = 0.0;
  return ret;
}

void cc_worker::set_tti(uint32_t tti)
{
  sf_cfg_dl.tti       = tti;
  sf_cfg_ul.tti       = TTI_TX(tti);
  sf_cfg_ul.shortened = false;
}

void cc_worker::set_cfo(float cfo)
{
  ue_ul_cfg.cfo_value = cfo;
}

float cc_worker::get_ref_cfo()
{
  return ue_dl.chest_res.cfo;
}

void cc_worker::set_crnti(uint16_t rnti)
{
  srslte_ue_dl_set_rnti(&ue_dl, rnti);
  srslte_ue_ul_set_rnti(&ue_ul, rnti);
}

void cc_worker::set_tdd_config(srslte_tdd_config_t config)
{
  sf_cfg_dl.tdd_config = config;
  sf_cfg_ul.tdd_config = config;
}

void cc_worker::enable_pregen_signals(bool enabled)
{
  this->pregen_enabled = enabled;
}

void cc_worker::fill_dci_cfg(srslte_dci_cfg_t* cfg, bool rel10)
{
  bzero(cfg, sizeof(srslte_dci_cfg_t));
  if (rel10 && phy->cif_enabled) {
    cfg->cif_enabled = phy->cif_enabled;
  }
  cfg->multiple_csi_request_enabled = phy->multiple_csi_request_enabled;
  cfg->srs_request_enabled          = phy->srs_request_enabled;
}

void cc_worker::set_dl_pending_grant(uint32_t cc_idx, srslte_dci_dl_t* dl_dci)
{
  if (!pending_dl_grant[cc_idx].enable) {
    pending_dl_grant[cc_idx].dl_dci = *dl_dci;
    pending_dl_grant[cc_idx].enable = true;
  } else {
    Warning("set_dl_pending_grant: cc=%d already exists\n", cc_idx);
  }
}

bool cc_worker::get_dl_pending_grant(uint32_t cc_idx, srslte_dci_dl_t* dl_dci)
{
  if (pending_dl_grant[cc_idx].enable) {
    *dl_dci                         = pending_dl_grant[cc_idx].dl_dci;
    pending_dl_grant[cc_idx].enable = false;
    return true;
  } else {
    return false;
  }
}



/************
 *
 * Sidelink Functions
 *
 */
// this is the time in seconds


void cc_worker::set_receive_time(srslte_timestamp_t tx_time)
{
  rx_time = tx_time;
}

// this is the rx_gain
void cc_worker::set_receiver_gain(float rx_gain_from_sf_worker)
{
  curr_rx_gain = rx_gain_from_sf_worker;
}

bool cc_worker::decode_pscch_dl(srsue::mac_interface_phy_lte::mac_grant_dl_t* grant)
{
  char timestr[64];
  timestr[0]='\0';

  uint32_t tti = sf_cfg_dl.tti;

  Debug("decode_pscch_dl TTI: %d t_SL: %d\n", tti, phy->ue_repo.subframe_rp[tti]);

#ifndef USE_SENSING_SPS
  // early completion in case of this subframe is not in resource pool
  if(phy->ue_repo.subframe_rp[tti] == -1) {
    return false;
  }
#endif

  uint8_t mdata[SRSLTE_SCI1_MAX_BITS + 16];
  uint16_t crc_rem = 0xdead;
  srslte_ra_sl_sci_t sci;
  cf_t * ce[4];
  srslte_ue_sl_mib_t * q = &ue_sl;


  ce[0] = q->ce;


    // try to decode PSCCH for each subchannel
  for(int rbp=0; rbp < phy->ue_repo.rp.numSubchannel_r14; rbp++) {

    uint32_t prb_offset = phy->ue_repo.rp.startRB_Subchannel_r14 + rbp*phy->ue_repo.rp.sizeSubchannel_r14;

    srslte_chest_sl_estimate_pscch(&q->chest, q->sf_symbols, q->ce, SRSLTE_SL_MODE_4, prb_offset);

    if (srslte_pscch_extract_llr(&q->pscch, q->sf_symbols, 
                        ce,
                        q->chest.noise_estimate,
                        0 %10, prb_offset)) {
      fprintf(stderr, "Error extracting LLRs\n");
      return -1;
    }

    if(SRSLTE_SUCCESS != srslte_pscch_dci_decode(&q->pscch, q->pscch.llr, mdata, q->pscch.max_bits, SRSLTE_SCI1_MAX_BITS, &crc_rem)) {
      continue;
    }

    if(SRSLTE_SUCCESS != srslte_repo_sci_decode(&phy->ue_repo, mdata, &sci)) {
      continue;
    }

    printf("DECODED PSCCH  N_X_ID: %x on tti %d t_SL_k: %d (rbp:%d) SCI-1: frl: 0x%x(n: %d L: %d) rri: %d gap: %d mcs: %d rti: %d\n",
            crc_rem, tti,
            srslte_repo_get_t_SL_k(&phy->ue_repo, tti),
            rbp,
            sci.frl,
            sci.frl_n_subCH,
            sci.frl_L_subCH,
            sci.resource_reservation,
            sci.time_gap,
            sci.mcs.idx,
            sci.rti);

    if(prb_offset != (uint32_t)phy->ue_repo.rp.startRB_Subchannel_r14 + sci.frl_n_subCH*phy->ue_repo.rp.sizeSubchannel_r14) {
      printf("Detected different Ressourcepool configurations.\n");
      continue;
    }

    q->pssch.n_X_ID = crc_rem;

    // memcpy(&grant->phy_grant.sl, &sci, sizeof(srslte_ra_sl_sci_t));
    memcpy(&pending_sl_grant[0].sl_dci, &sci, sizeof(srslte_ra_sl_sci_t));


    // /* Fill MAC grant structure */
    // grant->ndi[0] = 0;
    // grant->ndi[1] = 0;
    // grant->n_bytes[0] = sci.mcs.tbs / (uint32_t) 8;
    // grant->n_bytes[1] = 0;
    // grant->tti = phy->ue_repo.subframe_rp[tti];//tti;
    // grant->rv[0] = 0; // @todo change if this is a retransmission
    // grant->rv[1] = 0;
    // grant->rnti = 0;
    // grant->rnti_type = SRSLTE_RNTI_SL_PLACEHOLDER;
    // grant->last_tti = 0;
    // grant->tb_en[0] = true;
    // grant->tb_en[1] = false;
    // grant->tb_cw_swap = false;

    // here we select which harq process should handle this tb
    // @todo in future we may decode multiple SCIs in a single TTI,
    //       so the harq pid selection needs enhancing.

#ifdef USE_SENSING_SPS
    if (sci.rti==0) {
      // 1st tx
      grant->pid = (tti + sci.time_gap) % 8;
    } else {
      // retransmission
      grant->pid = tti % 8;
    }

    // we use this to distinguish transmissions from each other
    grant->sl_tti = tti;
    grant->sl_gap = sci.time_gap;
#else
    // todo: this may still be wrong, as i assume need at least 16 different processes
    grant->pid = (phy->ue_repo.subframe_rp[tti] + (8 - sci.rti * sci.time_gap)) % 8;

    grant->sl_tti = phy->ue_repo.subframe_rp[tti];
    grant->sl_gap = sci.time_gap;
#endif

    grant->tb[0].tbs = sci.mcs.tbs / (uint32_t) 8;
    grant->rnti = SRSLTE_RNTI_SL_PLACEHOLDER;

    // here we also need to select the correct rv value
    // Use ndi flag to tell dl_harq if this is a retransmission
    if (sci.rti==0) {
      grant->tb[0].rv = 0;
      grant->tb[0].ndi_present = true;
      grant->tb[0].ndi = true;
    } else {
       grant->tb[0].rv = 2;
      grant->tb[0].ndi_present = true;
      grant->tb[0].ndi = false;
    }

    // char hexstr[512];
    // hexstr[0]='\0';
    // if (log_h->get_level() >= srslte::LOG_LEVEL_INFO) {
    //   srslte_vec_sprint_hex(hexstr, sizeof(hexstr), dci_msg.data, dci_msg.nof_bits);
    // }
    // Info("PDCCH: DL DCI %s cce_index=%2d, L=%d, n_data_bits=%d, tpc_pucch=%d, hex=%s\n", srslte_dci_format_string(dci_msg.format),
    //      last_dl_pdcch_ncce, (1<<ue_dl.last_location.L), dci_msg.nof_bits, dci_unpacked.tpc_pucch, hexstr);
    
    return true; 
  }
  return false;
}




int cc_worker::decode_pssch(srslte_ra_sl_sci_t *grant, uint8_t *payload[SRSLTE_MAX_CODEWORDS],
                                     srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                                     uint32_t rv[SRSLTE_MAX_CODEWORDS],
                                     uint16_t rnti, uint32_t harq_pid, bool acks[SRSLTE_MAX_CODEWORDS]) {
  char timestr[64];
  char commonstr[128];
  char tbstr[2][128];
  bool valid_config = true;
  timestr[0]='\0';
  // srslte_mimo_type_t mimo_type = SRSLTE_MIMO_TYPE_SINGLE_ANTENNA;
  int ret = SRSLTE_SUCCESS;

  uint32_t tti = sf_cfg_dl.tti;

  // for (uint32_t tb = 0; tb < SRSLTE_MAX_CODEWORDS; tb++) {
  //   if (grant->tb_en[tb] && (rv[tb] < 0 || rv[tb] > 3)) {
  //     valid_config = false;
  //     Error("Wrong RV (%d) for TB index %d\n", rv[tb], tb);
  //   }
  // }


  /* Set power allocation according to 3GPP 36.213 clause 5.2 Downlink power allocation */


  Debug("DL Buffer TTI %d: Decoding PDSCH\n", tti);

  // we use this parameter to indicate successful decoding
  acks[0] = false;

  /* Setup PDSCH configuration for this CFI, SFIDX and RVIDX */
  if (valid_config) {
    /*if (!srslte_ue_dl_cfg_grant(&ue_dl, grant, cfi, tti%10, rv, mimo_type)) */
    if(1) {
      // if ((ue_dl.pdsch_cfg.grant.mcs[0].mod > 0 && ue_dl.pdsch_cfg.grant.mcs[0].tbs >= 0) ||
      //     (ue_dl.pdsch_cfg.grant.mcs[1].mod > 0 && ue_dl.pdsch_cfg.grant.mcs[1].tbs >= 0)) {
      if(grant->mcs.mod > 0 && grant->mcs.tbs >0) {
        
        float noise_estimate = srslte_chest_sl_get_noise_estimate(&ue_sl.chest);
        
        if (!phy->args->equalizer_mode.compare("zf")) {
          noise_estimate = 0; 
        }

        /* Set decoder iterations */
        if (phy->args->pdsch_max_its > 0) {
          srslte_pssch_set_max_noi(&ue_sl.pssch, phy->args->pdsch_max_its);
        }

        /** maybe export into seperate function */
        {

          // workaround for copy and paste
          srslte_ue_sl_mib_t *q = &ue_sl;

          uint32_t prb_offset = phy->ue_repo.rp.startRB_Subchannel_r14 + grant->frl_n_subCH*phy->ue_repo.rp.sizeSubchannel_r14;
          cf_t * ce[4];

          ce[0] = q->ce;

          // set parameters for pssch
          //q->pssch.n_X_ID = crc_rem;
          q->pssch.n_PSSCH_ssf = 0; //@todo, make dynamically when sfn is detected
          
          // generate pssch dmrs
          srslte_refsignal_sl_dmrs_pssch_gen(&q->chest.dmrs_signal,
                                              grant->frl_L_subCH*phy->ue_repo.rp.sizeSubchannel_r14 - 2,
                                              q->pssch.n_PSSCH_ssf,
                                              q->pssch.n_X_ID, // CRC checksum
                                              q->chest.pilot_known_signal);


          srslte_chest_sl_estimate_pssch(&q->chest, q->sf_symbols, ce[0], SRSLTE_SL_MODE_4,
                                          prb_offset + 2,
                                          grant->frl_L_subCH*phy->ue_repo.rp.sizeSubchannel_r14 - 2);

          cf_t *_sf_symbols[SRSLTE_MAX_PORTS]; 
          cf_t *_ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];

          _sf_symbols[0] = q->sf_symbols; 
          for (unsigned int i=0;i<q->pscch.cell.nof_ports;i++) {
            _ce[i][0] = ce[i]; 
          }

          //srslte_softbuffer_rx_reset_tbs(q->softbuffers[0], (uint32_t) grant->mcs.tbs);

          // // allocate buffer to store decoded data, tbs size in bytes plus crc 
          // uint8_t *data_rx[SRSLTE_MAX_CODEWORDS];
          // data_rx[0] = decoded;
          // memset(data_rx[0], 0xFF, sizeof(uint8_t) * sci.mcs.tbs/8 + 3);


          int decode_ret = srslte_pssch_decode_simple(&q->pssch,
                                                      grant,
                                                      NULL,//&pdsch_cfg,//srslte_pssch_cfg_t *cfg,
                                                      softbuffers,//srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                                                      _sf_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                                                      _ce,//_ce,//cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                                                      q->chest.noise_estimate,//float noise_estimate,
                                                      prb_offset + 2,
                                                      grant->frl_L_subCH*phy->ue_repo.rp.sizeSubchannel_r14 - 2,
                                                      payload);//uint8_t *data[SRSLTE_MAX_CODEWORDS],


          if(SRSLTE_SUCCESS == decode_ret) {
            ret = SRSLTE_SUCCESS;
            acks[0] = true;

            srslte_vec_fprint_byte(stdout, payload[0], grant->mcs.tbs/8 + 3);
          } else {
            Error("ERROR: Decoding PDSCH\n");
            printf("ERROR: Decoding PSSCH\n");
          }

        }


        // #ifdef LOG_EXECTIME
        //       struct timeval t[3];
        //       gettimeofday(&t[1], NULL);
        // #endif
        //       ret = srslte_pssch_decode(&ue_dl.pdsch, &ue_dl.pdsch_cfg, softbuffers, ue_dl.sf_symbols_m,
        //                                 ue_dl.ce_m, noise_estimate, rnti, payload, acks);
        //       if (ret) {
        //         Error("ERROR: Decoding PDSCH\n");
        //       }
        // #ifdef LOG_EXECTIME
        //       gettimeofday(&t[2], NULL);
        //       get_time_interval(t);
        //       snprintf(timestr, 64, ", dec_time=%4d us", (int) t[0].tv_usec);
        // #endif

        // char pinfo_str[16] = {0};
        // if (phy->config->dedicated.antenna_info_explicit_value.tx_mode == LIBLTE_RRC_TRANSMISSION_MODE_4) {
        //   snprintf(pinfo_str, 15, ", pinfo=%x", grant->pinfo);
        // }

        // snprintf(commonstr, 128, "PDSCH: l_crb=%2d, harq=%d, snr=%.1f dB, tx_scheme=%s%s", grant->nof_prb, harq_pid,
        //          10 * log10(srslte_chest_dl_get_snr(&ue_dl.chest)), srslte_mimotype2str(mimo_type), pinfo_str);

        // for (int i=0;i<SRSLTE_MAX_CODEWORDS;i++) {
        //   if (grant->tb_en[i]) {
        //     snprintf(tbstr[i], 128, ", CW%d: tbs=%d, mcs=%d, rv=%d, crc=%s, it=%d",
        //              i, grant->mcs[i].tbs/8, grant->mcs[i].idx, rv[i], acks[i] ? "OK" : "KO",
        //              srslte_pdsch_last_noi_cw(&ue_dl.pdsch, i));
        //   }
        // }

        // Info("%s%s%s%s\n", commonstr, grant->tb_en[0]?tbstr[0]:"", grant->tb_en[1]?tbstr[1]:"", timestr);

        // Store metrics
        dl_metrics.mcs    = grant->mcs.idx;
        float niters = srslte_pssch_last_noi(&ue_sl.pssch);
        if (niters) {
          dl_metrics.turbo_iters = niters;
        }
      } else {
        Warning("Received grant for TBS=0\n");
      }
    } else {
      Error("Error configuring DL grant\n");
      ret = SRSLTE_ERROR;
    }
  } else {
    Error("Error invalid DL config\n");
    ret = SRSLTE_ERROR;
  }
  return ret;
}







bool cc_worker::work_sl_rx()
{
  bool dl_ack[SRSLTE_MAX_CODEWORDS];

  mac_interface_phy_lte::tb_action_dl_t dl_action;

  uint32_t tti = sf_cfg_dl.tti;
  float rssi_sps = 0.0f;
  float rssi_dBm = 0.0f;

  last_decoding_successful_high_rsrp = false;

  /* Run FFT for the slot symbols */
  srslte_ofdm_rx_sf(&ue_sl.fft);

  // reset agc value
  this->agc_max_value = 0.0;

  // update max value for agc calculations
  float* t = (float*) ue_sl.fft.in_buffer; 
  this->agc_max_value = t[srslte_vec_max_fi(t, 2*ue_sl.fft.sf_sz)];// take only positive max to avoid abs() (should be similar)

  /* Initialise the SPS algorithm for this tti */
  phy->sensing_sps->tick(tti);

  /* calculate S-RSSI for SPS */
  if (!phy->sensing_sps->getTransmit(tti))
  {
    // uint8_t L_subch       = 1; // phy->ue_repo.rp.numSubchannel_r14
    uint8_t n_subCH_start = 0;

    int n_re_pssch_rssi = 2 * SRSLTE_CP_NSYMB(cell.cp) - 2; // exclude l=0 and l=13.

    // struct timespec start, end;
    // // ios_base::sync_with_stdio(false); // unsync the I/O
    // clock_gettime(CLOCK_MONOTONIC, &start);

    srslte_pssch_get_for_sps_rssi(
        ue_sl.sf_symbols, ue_sl.pssch.SymSPSRssi[0], ue_sl.pssch.cell,
        phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start * phy->ue_repo.rp.sizeSubchannel_r14,
        // L_subch * phy->ue_repo.rp.sizeSubchannel_r14); // avearge over Lsubch including PSCCH and PSSCH.
        phy->ue_repo.rp.numSubchannel_r14 * phy->ue_repo.rp.sizeSubchannel_r14);

    // // Calculating total time taken by the program.
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // double time_taken;
    // time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    // time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;

    // std::cout << "Time taken by srslte_pssch_get_for_sps_rssi is : " << time_taken << setprecision(9) << " sec" << endl;

    // clock_gettime(CLOCK_MONOTONIC, &start);
    rssi_sps =
        // srslte_vec_avg_power_cf(ue_sl.pssch.SymSPSRssi[SRSLTE_MAX_PORTS], SRSLTE_SF_LEN_PRB(cell.nof_prb));
        srslte_vec_avg_power_cf(ue_sl.pssch.SymSPSRssi[0],
                                // L_subch * phy->ue_repo.rp.sizeSubchannel_r14 * n_re_pssch_rssi); 
                                // L_subch * phy->ue_repo.rp.sizeSubchannel_r14 * SRSLTE_NRE * n_re_pssch_rssi);
                                phy->ue_repo.rp.numSubchannel_r14 * phy->ue_repo.rp.sizeSubchannel_r14 * SRSLTE_NRE * n_re_pssch_rssi) + (1 + (rand() % 1000)) / 1000.0E8;
    //srslte_vec_avg_power_cf(ue_sl.pssch.SymSPSRssi[0], 1); // for calc time testing: pw avg for one sym. 
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    // time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    // // std::cout << "Time taken by rssi_sps avg_power_cf is : " << time_taken << setprecision(9) << " sec" << endl;

    ue_sl.pssch.sps_rssi[sps_rssi_read_cnt] = rssi_sps;
    phy->inst_sps_rssi[sps_rssi_read_cnt]   = 10 * log10(rssi_sps * 1000); // in dBm.
    rssi_dBm = 10 * log10(rssi_sps) - 10*log10(ue_sl.fft.symbol_sz * ue_sl.fft.symbol_sz / ue_sl.fft.nof_re); //29.93f; // 10*log10(768*768/600)
    
    // phy->inst_sps_rssi = 10 * log10(rssi_sps * 1000); // in dBm.


    // if( phy->inst_sps_rssi[sps_rssi_read_cnt] - phy->sl_rssi > 15) {
    //   printf("TTI: %4d detected large RSSI value of %f (avg: %f)\n", tti, phy->inst_sps_rssi[sps_rssi_read_cnt], phy->sl_rssi);
    //   last_decoding_successful_high_rsrp = true;
    // }

    // save average RSSI
    phy->sl_rssi = SRSLTE_VEC_EMA(rssi_dBm, phy->sl_rssi, 0.1);

    sps_rssi_read_cnt++;
    if (sps_rssi_read_cnt == 1000)
      sps_rssi_read_cnt = 0;

    // @todo: check if are save to user rssi_dBm here
    phy->sensing_sps->addAverageSRSSI(tti,10 * log10(rssi_sps * 1000));

    // for sensing-based SPS we need to measure the S-RSSI per subchannel
    // Note that this reuses the SymSPSRssi buffer
    uint32_t n_re_pscch_subchannel = phy->ue_repo.rp.sizeSubchannel_r14 * SRSLTE_NRE * n_re_pssch_rssi;

    for (int rbp = 0; rbp < phy->ue_repo.rp.numSubchannel_r14; ++rbp) {
      srslte_pssch_get_for_sps_rssi(ue_sl.sf_symbols,
                                    ue_sl.pssch.SymSPSRssi[0],
                                    ue_sl.pssch.cell,
                                    phy->ue_repo.rp.startRB_Subchannel_r14 + rbp * phy->ue_repo.rp.sizeSubchannel_r14,
                                    phy->ue_repo.rp.sizeSubchannel_r14);

      float rssi = srslte_vec_avg_power_cf(ue_sl.pssch.SymSPSRssi[0], n_re_pscch_subchannel) + (1 + (rand() % 1000)) / 1000.0E8;

      phy->sensing_sps->addChannelSRSSI(tti, rbp, 10 * log10(rssi * 1000));
    }
  }

  // in each sync symbol we also decode mib
  if(!phy->args->sidelink_master && (tti%5 == 0)) {
    // get channel estimates for psbch
    int ret = srslte_chest_sl_estimate_psbch(&ue_sl.chest, ue_sl.sf_symbols, ue_sl.ce, SRSLTE_SL_MODE_4, 0, 0);
    if (ret < 0) {
      printf("srslte_chest_sl_estimate_psbch failed with %d", ret);
    }

    // calculate snr psbch
    float snr = srslte_chest_sl_get_snr_db(&ue_sl.chest);
    float rsrp = 10*log10(ue_sl.chest.pilot_power) - 10*log10(ue_sl.fft.symbol_sz * ue_sl.fft.symbol_sz);

    phy->snr_psbch = SRSLTE_VEC_EMA(snr, phy->snr_psbch, 0.1);
    phy->rsrp_psbch = SRSLTE_VEC_EMA(rsrp, phy->rsrp_psbch, 0.1);

    uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN];
    /* Decode PSBCH */
    ret = srslte_psbch_decode(&ue_sl.psbch, ue_sl.sf_symbols, ue_sl.ce, ue_sl.chest.noise_estimate, bch_payload);

    if (ret < 0) {
      printf("Error decoding PSBCH (%d) in phch worker snr: %f / %f rsrp: %f / %f @tti %d\n",
              ret, snr, phy->snr_psbch, rsrp, phy->rsrp_psbch, tti);
      ret = SRSLTE_UE_SL_MIB_NOTFOUND; 
    } else if (ret == 1) {
      uint32_t dfn;
      uint32_t dsfn;
      srslte_cell_t cell;
      srslte_psbch_mib_unpack(bch_payload, &cell, &dfn, &dsfn);

      if(tti != 10*dfn + dsfn) {
        printf("-----------------Decoded differing MIB tti(%d/%d) numbers during camping---------\n", tti, 10*dfn + dsfn);
        // @todo: we need to find a way to either run run_sfn_sync or to update the tti manually
      }

      srslte_ue_sl_mib_reset(&ue_sl);
    } else {
      printf("MIB not decoded: %u in phch worker\n", ue_sl.frame_cnt);
      ue_sl.frame_cnt++;
    }
  } else {
    // report large RSSI values, except for broadcast subframes
    if( rssi_dBm - phy->sl_rssi > 15) {
      // printf("TTI: %4d detected large RSSI value of %f (avg: %f)\n", tti, rssi_dBm, phy->sl_rssi);
      last_decoding_successful_high_rsrp = true;
    }
  }

  int t_SL_k = srslte_repo_get_t_SL_k(&phy->ue_repo, tti);

  // do not decode our own sent messages
  if (phy->sensing_sps->getTransmit(tti)) {
    // exclude subframe from agc measurement
    this->agc_max_value = 0.0;

    // early return
    return true;
  }

  #ifndef USE_SENSING_SPS
  // stop processing in case this subframe is not in resource pool
  if (t_SL_k < 0) {
    return true;
  }
  #endif

  mac_interface_phy_lte::mac_grant_dl_t dl_mac_grant = {};

  memset(&pending_sl_grant[0], 0x00, sizeof(pending_sl_grant[0]));

  last_decoding_successful = false;

#if 1
  // do not decode our own sent messages
  if(!phy->sensing_sps->getTransmit(tti) && decode_pscch_dl(&dl_mac_grant)) {

    /* Send grant to MAC and get action for this TB */
    phy->stack->new_grant_dl(cc_idx, dl_mac_grant, &dl_action);

    /* Decode PSSCH if instructed to do so */
    if (dl_action.tb[0].enabled) {

      // indicate, that we can dump this subframe
      last_decoding_successful = true;
      
      decode_pssch(&pending_sl_grant[0].sl_dci, &dl_action.tb[0].payload,
                    &dl_action.tb[0].softbuffer.rx, &dl_action.tb[0].rv, dl_mac_grant.rnti,
                    dl_mac_grant.pid, dl_ack);
      
      // calculate snr for possibly decoded pssch
      float snr = srslte_chest_sl_get_snr_db(&ue_sl.chest);
      float rsrp = 10*log10(ue_sl.chest.pilot_power) - 10*log10(ue_sl.fft.symbol_sz * ue_sl.fft.symbol_sz);

      // calculate noise
      float noise_power = srslte_chest_sl_get_noise_estimate(&ue_sl.chest);

      // calculate for timestemp
      time_t rx_full_secs = rx_time.full_secs;
      float  rx_frac_secs = float(rx_time.frac_secs);

      // following values are dumped into PCAP
      dl_mac_grant.sl_lte_tti = tti;
      dl_mac_grant.sl_snr = snr;
      dl_mac_grant.sl_rsrp = rsrp;
      dl_mac_grant.sl_rssi = rssi_dBm;
      dl_mac_grant.sl_rx_full_secs = rx_full_secs;
      dl_mac_grant.sl_rx_frac_secs = rx_frac_secs;
      dl_mac_grant.sl_noise_power  = noise_power;
      dl_mac_grant.sl_rx_gain  = curr_rx_gain;

      // combine extracted FRL into one variable
      dl_mac_grant.sl_sci_frl = (pending_sl_grant[0].sl_dci.frl_L_subCH << 8) | (pending_sl_grant[0].sl_dci.frl_n_subCH & 0xFF);

      int ue_id = srslte_repo_get_t_SL_k(&phy->ue_repo, tti % 10240);

      #ifdef USE_SENSING_SPS
      // in SPS mode we can not say which UE sent us data, so we always tell it is 0
      ue_id = 0;
      #endif

      if(ue_id < 0) {
        // this should not happen
        printf("Decoded in none sidelink subframe.!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      } else {
        // calulate exponential moving average
        ue_id = ue_id % 5;
        phy->snr_pssch_per_ue[ue_id] = SRSLTE_VEC_EMA(snr, phy->snr_pssch_per_ue[ue_id], 0.1);
        phy->rsrp_pssch_per_ue[ue_id] = SRSLTE_VEC_EMA(rsrp, phy->rsrp_pssch_per_ue[ue_id], 0.1);
        //printf("moving snr[%d]: %f\n", ue_id, phy->snr_pssch_per_ue[ue_id]);

        printf("Current SNR: %f (EMA: %f) RSRP: %f (EMA: %f) for ue_id %d\n",
                snr, phy->snr_pssch_per_ue[ue_id], rsrp, phy->rsrp_pssch_per_ue[ue_id], ue_id);
        
        printf("Before writeToCSV");
        writeToCSV("output_new_metrics.csv", ue_id, snr, phy->snr_pssch_per_ue[ue_id], rsrp, phy->rsrp_pssch_per_ue[ue_id], noise_power, rx_full_secs, rx_frac_secs, rssi_dBm, curr_rx_gain);

        // if(phy->args->sidelink_id == ue_id) {
        //   printf("decoded data in my own time slot????? tti: %d\n", tti);
        // }
      }

      /* calculate PSSCH-RSRP for SPS */
      {
        uint8_t n_subCH_start = 0;

        int n_rs_pssch_rsrp = srslte_refsignal_sl_dmrs_pscch_N_rs(SRSLTE_SL_MODE_4, 0) +
                              srslte_refsignal_sl_dmrs_pscch_N_rs(SRSLTE_SL_MODE_4, 1); //hard coded as mode 4.

        srslte_pssch_get_for_sps_rsrp( // ue_sl.sf_symbols, ue_sl.pssch.SymSPSRsrp[SRSLTE_MAX_PORTS], ue_sl.pssch.cell,
            ue_sl.sf_symbols, ue_sl.pssch.SymSPSRsrp[0], ue_sl.pssch.cell,
            phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start * phy->ue_repo.rp.sizeSubchannel_r14 + 2,
            // L_subch * phy->ue_repo.rp.sizeSubchannel_r14 - 2); // avearge over Lsubch of PSSCH.
            phy->ue_repo.rp.numSubchannel_r14 * phy->ue_repo.rp.sizeSubchannel_r14 - 2);

        float rsrp_sps =
            // srslte_vec_avg_power_cf(ue_sl.pssch.SymSPSRsrp[SRSLTE_MAX_PORTS], SRSLTE_SF_LEN_PRB(cell.nof_prb));
            srslte_vec_avg_power_cf(ue_sl.pssch.SymSPSRsrp[0],
                                    // (L_subch * phy->ue_repo.rp.sizeSubchannel_r14 - 2) * n_rs_pssch_rsrp);
                                    (phy->ue_repo.rp.numSubchannel_r14 * phy->ue_repo.rp.sizeSubchannel_r14 - 2) * SRSLTE_NRE * n_rs_pssch_rsrp);

        int tst = (phy->ue_repo.rp.numSubchannel_r14 * phy->ue_repo.rp.sizeSubchannel_r14 - 2) * SRSLTE_NRE * n_rs_pssch_rsrp;  
                                
        ue_sl.pssch.sps_rsrp[sps_rsrp_read_cnt] = rsrp_sps;
        phy->inst_sps_rsrp[sps_rsrp_read_cnt] = 10 * log10(rsrp_sps * 1000); // in dBm.
        // phy->inst_sps_rsrp = 10 * log10(rsrp_sps * 1000); // in dBm.

        // Warning: we are currently only decoding a single PSSCH per subframe
        //          and are calculating the RSRP across all the channels
        phy->sensing_sps->addSCI(tti,
                            pending_sl_grant[0].sl_dci.frl_n_subCH,
                            pending_sl_grant[0].sl_dci.frl_L_subCH,
                            pending_sl_grant[0].sl_dci.resource_reservation,
                            pending_sl_grant[0].sl_dci.priority,
                            10 * log10(rsrp_sps * 1000)
        );


#if 0
printf("dumping subframe subframe..\n");
  for (int i=0;i<8400;++i) {
      cf_t c = ue_sl.sf_symbols[i];
      printf(" %f%+fj",crealf(c),cimagf(c));
}
#endif

        sps_rsrp_read_cnt++;
        if (sps_rsrp_read_cnt == 1000)
          sps_rsrp_read_cnt = 0;
      }

      // append SNR value after payload, the mac knows about it and will read it from there to attach it to each TCP terminated sl packet
      *((float *)(dl_action.tb[0].payload + dl_mac_grant.tb[0].tbs)) = phy->snr_pssch_per_ue[ue_id];//snr;//ue_sl.chest.noise_estimate;

#ifdef ENABLE_GUI
      // save ce for psxch for plotting
      bzero(ue_sl.ce_plot, SRSLTE_NRE * cell.nof_prb * sizeof(cf_t));// (pending_sl_grant[0].sl_dci.frl_L_subCH * phy->ue_repo.rp.sizeSubchannel_r14) * sizeof(cf_t));
      memcpy(&ue_sl.ce_plot[SRSLTE_NRE * (pending_sl_grant[0].sl_dci.frl_n_subCH * phy->ue_repo.rp.sizeSubchannel_r14)],
              &ue_sl.ce[SRSLTE_NRE * (2*cell.nof_prb + pending_sl_grant[0].sl_dci.frl_n_subCH * phy->ue_repo.rp.sizeSubchannel_r14)],
              SRSLTE_NRE * (pending_sl_grant[0].sl_dci.frl_L_subCH * phy->ue_repo.rp.sizeSubchannel_r14) * sizeof(cf_t));

      memcpy(ue_sl.td_plot, ue_sl.fft.in_buffer, ue_sl.fft.sf_sz * sizeof(cf_t));
#endif
      
    }

    phy->stack->tb_decoded(cc_idx, dl_mac_grant, dl_ack);//[0], 0, dl_mac_grant.rnti_type, dl_mac_grant.pid);
  }

#if 0
  /* Generate some Dummy SCIs / RSSI measurements */
  phy->sensing_sps->dummyRx(tti);
#endif


#endif

  return true;
}

bool cc_worker::dump_subframe() {

  if(phy->n_subframes_to_dump>0 && last_decoding_successful) {
    struct timeval t;
    gettimeofday(&t, NULL);

    char filename [30];

    snprintf(filename, sizeof(filename), "iq_dump_%ld.%ld.bin", t.tv_sec, t.tv_usec);

    srslte_filesink_t fsink;
    srslte_filesink_init(&fsink, filename, SRSLTE_COMPLEX_FLOAT_BIN);
    srslte_filesink_write(&fsink, (void*) get_rx_buffer(0), SRSLTE_SF_LEN_PRB(cell.nof_prb));
    srslte_filesink_free(&fsink);

    phy->n_subframes_to_dump--;
  }

  if(last_decoding_successful_high_rsrp && phy->n_subframes_to_dump_special>0) {
    struct timeval t;
    gettimeofday(&t, NULL);

    char filename [30];

    snprintf(filename, sizeof(filename), "iq_hr_dump_%ld.%ld.bin", t.tv_sec, t.tv_usec);

    srslte_filesink_t fsink;
    srslte_filesink_init(&fsink, filename, SRSLTE_COMPLEX_FLOAT_BIN);
    srslte_filesink_write(&fsink, (void*) get_rx_buffer(0), SRSLTE_SF_LEN_PRB(cell.nof_prb));
    srslte_filesink_free(&fsink);

    phy->n_subframes_to_dump_special--;
  }
  
  return true;
}


bool cc_worker::work_sl_tx() {
  bool signal_ready = false;

  mac_interface_phy_lte::mac_grant_ul_t ul_mac_grant;
  mac_interface_phy_lte::tb_action_ul_t ul_action;

  ZERO_OBJECT(ul_mac_grant);
  ZERO_OBJECT(ul_action);

  // this is the ul tti
  int tti = sf_cfg_ul.tti;

  // we are the master node and need to send sync sequences
  if(phy->args->sidelink_master && ((tti % phy->ue_repo.syncPeriod) == phy->ue_repo.syncOffsetIndicator_r12)) {

    // clear sf buffer
    bzero(ue_sl_tx.sf_symbols, sizeof(cf_t)*SRSLTE_SF_LEN_RE(ue_sl_tx.cell.nof_prb, ue_sl_tx.cell.cp));
    srslte_psss_put_sf(ue_sl_tx.psss_signal, ue_sl_tx.sf_symbols, cell.nof_prb, SRSLTE_CP_NORM);
    srslte_ssss_put_sf(ue_sl_tx.ssss_signal, ue_sl_tx.sf_symbols, cell.nof_prb, SRSLTE_CP_NORM);

    srslte_refsignal_sl_dmrs_psbch_put(&ue_sl_tx.signals, SRSLTE_SL_MODE_4, ue_sl_tx.psbch_dmrs, ue_sl_tx.sf_symbols);

    srslte_psbch_mib_pack(&cell,
                          (tti % 10240)/10, //sfn,
                          tti % 10,//sf_idx,
                          ue_sl_tx.bch_payload);

    cf_t *sf_tmp[2];
    sf_tmp[0] = ue_sl_tx.sf_symbols;
    sf_tmp[1] = NULL;

    srslte_psbch_encode(&ue_sl_tx.psbch, ue_sl_tx.bch_payload, sf_tmp);

    srslte_ofdm_tx_sf(&ue_sl_tx.fft);

    signal_ready = true;

    phy->sensing_sps->setTransmit(tti,signal_ready);

    // we can already return, as sync signals are exclusive in a subframe
    return signal_ready;
  }

  // 2 modes of operation currently -
  // Either using the bitmap from the repo pool and the sidelink_id,
  // Or a basic implementation of Sensing SPS.

  srslte_ra_sl_sci_t sci;
  uint8_t            L_subch       = 1;
  uint8_t            n_subCH_start = 0;
  int t_SL_k = -1;

#ifdef USE_SENSING_SPS
  // Call Sensing SPS algorithm to get us the resources to transmit on
  // @todo this MUST generate a valid number of PRBs..
  srslte::ReservationResource* resource = phy->sensing_sps->schedule(tti,1); // @todo for now we always say we have data to send

  if (resource) {
    uint32_t sl_gap = resource->sl_gap;
    uint32_t num_harq_proc = 8;

    sci.rti = (resource->is_retx) ? 1 : 0;

    ul_mac_grant.rnti = SRSLTE_RNTI_SL_RANDOM;

    // Select the harq process we will use
    ul_mac_grant.tb.ndi_present = true;
    if (sci.rti==0) {
      // new transmission
      ul_mac_grant.pid = ((tti + sl_gap) % 10240 ) % num_harq_proc;
      ul_mac_grant.tb.ndi = true;
      sci.resource_reservation = resource->rsvp;
      sci.time_gap = sl_gap;
    } else {
      // retransmission
      ul_mac_grant.pid = tti % num_harq_proc;
      ul_mac_grant.tb.ndi = false;
      sci.resource_reservation = resource->rsvp;
      sci.time_gap = 0;
    }
    
    sci.priority = 0; // @todo

    L_subch = resource->numSubchannels; // @todo must result in valid number of prbs srslte_dft_precoding_valid_prb()
    n_subCH_start = resource->subchannelStart;

    int n_prb = L_subch*phy->ue_repo.rp.sizeSubchannel_r14 - 2;
    // make mcs adjustable via REST @todo: is this fixed for a SPS interval
    sci.mcs.idx = phy->pssch_fixed_i_mcs;
    srslte_sl_fill_ra_mcs(&sci.mcs, n_prb);

    // mac creates a packet that has exactly this size
    ul_mac_grant.tb.tbs = sci.mcs.tbs/8;

    // get mac pdu
    phy->stack->new_grant_ul(0, ul_mac_grant, &ul_action);
  }
#else
  t_SL_k = srslte_repo_get_t_SL_k(&phy->ue_repo,tti % 10240);

  // determine if next transmit tti belongs to resource pool and we are statically assigned to it
  // note that ul_harq selects the harq pid in this case
  if(t_SL_k >0 && phy->args->sidelink_id == (t_SL_k%5)) {

    ul_mac_grant.rnti = SRSLTE_RNTI_SL_RANDOM;
    ul_mac_grant.sl_tti = t_SL_k;//tti;

    // set retransmission gap
    ul_mac_grant.sl_gap = 5;

    // find a suitable tbs and mcs level
    sci.priority = 0;
    sci.resource_reservation = 0;

    sci.time_gap = ul_mac_grant.sl_gap;

    sci.mcs.idx = phy->pssch_fixed_i_mcs;

    L_subch = 0;
    uint8_t last_valid_l_subch = 0;

    // get valid prb sizes by checking all subchannel sizes
    // @todo: we still need too handle the case, when we do not find a config to fulfill the tbs requirement
    while(L_subch < phy->ue_repo.rp.numSubchannel_r14) {
      L_subch++;
      int n_prb = L_subch * phy->ue_repo.rp.sizeSubchannel_r14 - 2;

      // check if number of prb fullfills 2^a2*3^a3*5^a5, see14.1.1.4C
      if(!srslte_dft_precoding_valid_prb(n_prb)) {
        continue;
      }

      srslte_sl_fill_ra_mcs(&sci.mcs, n_prb);
      last_valid_l_subch = L_subch;

      if(phy->pssch_min_tbs <= sci.mcs.tbs) {
        break;
      }
    }

    // if last config was not valid
    L_subch = last_valid_l_subch;

    // mac creates a packet that has exactly this size
    ul_mac_grant.tb.tbs = sci.mcs.tbs/8;

    // get mac pdu
    phy->stack->new_grant_ul(0, ul_mac_grant, &ul_action);

    // /* Set UL CFO before transmission */
    // srslte_ue_ul_set_cfo(&ue_ul, cfo);
  }
#endif

  {
    if(ul_action.tb.payload)
    {
      // fill in required values for sci
      sci.rti = ul_action.tb.rv == 0 ? 0 : 1;
      
      float c_bits = 9*12*srslte_mod_bits_x_symbol(sci.mcs.mod)*(L_subch*phy->ue_repo.rp.sizeSubchannel_r14 - 2);
      float i_bits = sci.mcs.tbs + 24;

      // effective channel code rate 3GPP 36.213 7.1.7.2
      printf("TBS: %d PRB: %d Channel-Bits: %.0f Coderate: %f\n",
              sci.mcs.tbs,
              (L_subch*phy->ue_repo.rp.sizeSubchannel_r14 - 2),
              c_bits,
              i_bits/c_bits);
      
      // select random between valid values
      n_subCH_start = rand() % (phy->ue_repo.rp.numSubchannel_r14 - L_subch + 1);

      // set RIV
      srslte_repo_encode_frl(&phy->ue_repo, &sci, L_subch, n_subCH_start);

      uint8_t sci_buffer[SRSLTE_SCI1_MAX_BITS + 16];

      memset(sci_buffer, 0x00, SRSLTE_SCI1_MAX_BITS);

      srslte_repo_sci_encode(&phy->ue_repo, sci_buffer, &sci);

      printf("TTI: %d t_SL_k: %d ENCODED SCI-1: prio: %d frl(%d): 0x%x(n: %d L: %d) rri: %d gap: %d mcs: %d rti: %d\n",
              tti,
              t_SL_k,
              sci.priority,
              srslte_repo_frl_len(&phy->ue_repo),
              sci.frl,
              sci.frl_n_subCH,
              sci.frl_L_subCH,
              sci.resource_reservation,
              sci.time_gap,
              sci.mcs.idx,
              sci.rti);

      cf_t *ta[2];
      ta[0] = ue_sl_tx.sf_symbols;

      bzero(ue_sl_tx.sf_symbols, sizeof(cf_t)*SRSLTE_SF_LEN_RE(ue_sl_tx.cell.nof_prb, ue_sl_tx.cell.cp));

      if(SRSLTE_SUCCESS != srslte_pscch_encode(&ue_sl_tx.pscch, sci_buffer, phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start*phy->ue_repo.rp.sizeSubchannel_r14, ta)) {
        printf("Failed to encode PSCCH for PRB offset: %d \n", phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start*phy->ue_repo.rp.sizeSubchannel_r14);
        exit(-1);
      }

      srslte_refsignal_sl_dmrs_psxch_put(&ue_sl_tx.signals, SRSLTE_SL_MODE_4,
                                          phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start*phy->ue_repo.rp.sizeSubchannel_r14,
                                          2,
                                          ue_sl_tx.pscch_dmrs, ue_sl_tx.sf_symbols);


      // retrieve crc for pssch dmrs generation
      uint8_t *x = &sci_buffer[SRSLTE_SCI1_MAX_BITS];
      uint16_t pscch_crc = (uint16_t) srslte_bit_pack(&x, 16);

      srslte_vec_fprint_hex(stdout, sci_buffer, SRSLTE_SCI1_MAX_BITS + 16);
      //printf("CRC: %x\n", pscch_crc);



      ue_sl_tx.pssch.n_PSSCH_ssf = 0;//ssf;
      ue_sl_tx.pssch.n_X_ID = pscch_crc;

      // rest softbuffer only on initial transmission, retransmissions will use the same buffer
      // keep in mind, that actually the w_buff used in rate-matching will be persistent and is
      // only populated when RV=0 which is only fulfilled on initial transmission
      if(sci.rti == 0) {
        srslte_softbuffer_tx_reset_tbs(ul_action.tb.softbuffer.tx, (uint32_t) sci.mcs.tbs);
      }

      int encode_ret = srslte_pssch_encode_simple(&ue_sl_tx.pssch,
                            &sci,
                            &ul_action.tb.softbuffer.tx,//softbuffers,//srslte_softbuffer_tx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                            ta,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                            phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start*phy->ue_repo.rp.sizeSubchannel_r14 + 2,//repo.rp.startRB_Subchannel_r14 + rbp*repo.rp.sizeSubchannel_r14 + 2,//uint32_t prb_offset,
                            L_subch * phy->ue_repo.rp.sizeSubchannel_r14 - 2,//sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2,//uint32_t n_prb,
                            &ul_action.tb.payload);//uint8_t *data[SRSLTE_MAX_CODEWORDS])


      // generate pssch dmrs
      srslte_refsignal_sl_dmrs_pssch_gen(&ue_sl_tx.signals,
                                          L_subch * phy->ue_repo.rp.sizeSubchannel_r14 - 2,
                                          ue_sl_tx.pssch.n_PSSCH_ssf,
                                          ue_sl_tx.pssch.n_X_ID, // CRC checksum
                                          ue_sl_tx.pssch_dmrs);

      srslte_refsignal_sl_dmrs_psxch_put(&ue_sl_tx.signals, SRSLTE_SL_MODE_4,
                                          phy->ue_repo.rp.startRB_Subchannel_r14 + n_subCH_start*phy->ue_repo.rp.sizeSubchannel_r14 + 2,
                                          L_subch * phy->ue_repo.rp.sizeSubchannel_r14 - 2,
                                          ue_sl_tx.pssch_dmrs, ue_sl_tx.sf_symbols);  

      // add awgn to transmit signal
      srslte_vec_sub_ccc(ue_sl_tx.sf_symbols, phy->noise_buffer, ue_sl_tx.sf_symbols, SRSLTE_SF_LEN_RE(cell.nof_prb, SRSLTE_CP_NORM));

      srslte_ofdm_tx_sf(&ue_sl_tx.fft);

      // apply same cfo which we have to the master node, by doing this, each other node
      // also needs to be cfo-sync to the master to be automatically sync with all nodes
      if (ue_sl_tx.cfo_en) {
        //srslte_cfo_correct(&ue_sl_tx.cfo, signal_buffer[0], signal_buffer[0], get_cfo() / srslte_symbol_sz(ue_sl_tx.cell.nof_prb));
        srslte_cfo_correct(&ue_sl_tx.cfo, ue_sl_tx.out_buffer, ue_sl_tx.out_buffer, ue_ul_cfg.cfo_value / srslte_symbol_sz(ue_sl_tx.cell.nof_prb));
      }

      srslte_ue_sl_tx_apply_norm(&ue_sl_tx, L_subch*phy->ue_repo.rp.sizeSubchannel_r14);

      signal_ready = true;
      // signal_ptr = signal_buffer[0];
    }
  }

  phy->sensing_sps->setTransmit(tti,signal_ready);

  return signal_ready;
}





/************
 *
 * Downlink Functions
 *
 */

bool cc_worker::work_dl_regular()
{
  bool dl_ack[SRSLTE_MAX_CODEWORDS];

  mac_interface_phy_lte::tb_action_dl_t dl_action;

  bool found_dl_grant = false;

  sf_cfg_dl.sf_type = SRSLTE_SF_NORM;

  // Set default channel estimation
  ue_dl_cfg.chest_cfg = chest_default_cfg;

  /* For TDD, when searching for SIB1, the ul/dl configuration is unknown and need to do blind search over
   * the possible mi values
   */
  uint32_t mi_set_len;
  if (cell.frame_type == SRSLTE_TDD && !sf_cfg_dl.tdd_config.configured) {
    mi_set_len = 3;
  } else {
    mi_set_len = 1;
  }

  // Blind search PHICH mi value
  for (uint32_t i = 0; i < mi_set_len && !found_dl_grant; i++) {

    if (mi_set_len == 1) {
      srslte_ue_dl_set_mi_auto(&ue_dl);
    } else {
      srslte_ue_dl_set_mi_manual(&ue_dl, i);
    }

    /* Do FFT and extract PDCCH LLR, or quit if no actions are required in this subframe */
    if (srslte_ue_dl_decode_fft_estimate(&ue_dl, &sf_cfg_dl, &ue_dl_cfg) < 0) {
      Error("Getting PDCCH FFT estimate\n");
      return false;
    }

    /* Look for DL and UL dci(s) if this is PCell, or no cross-carrier scheduling is enabled */
    if ((cc_idx == 0) || (!phy->cif_enabled)) {
      found_dl_grant = decode_pdcch_dl() > 0;
      decode_pdcch_ul();
    }
  }

  srslte_dci_dl_t dci_dl;
  bool            has_dl_grant = get_dl_pending_grant(cc_idx, &dci_dl);

  // If found a dci for this carrier, generate a grant, pass it to MAC and decode the associated PDSCH
  if (has_dl_grant) {

    // Read last TB from last retx for this pid
    for (uint32_t i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
      ue_dl_cfg.cfg.pdsch.grant.last_tbs[i] = phy->last_dl_tbs[dci_dl.pid][cc_idx][i];
    }
    // Generate PHY grant
    if (srslte_ue_dl_dci_to_pdsch_grant(&ue_dl, &sf_cfg_dl, &ue_dl_cfg, &dci_dl, &ue_dl_cfg.cfg.pdsch.grant)) {
      Error("Converting DCI message to DL dci\n");
      return -1;
    }

    // Save TB for next retx
    for (uint32_t i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
      phy->last_dl_tbs[dci_dl.pid][cc_idx][i] = ue_dl_cfg.cfg.pdsch.grant.last_tbs[i];
    }

    // Set RNTI
    ue_dl_cfg.cfg.pdsch.rnti = dci_dl.rnti;

    // Generate MAC grant
    mac_interface_phy_lte::mac_grant_dl_t mac_grant;
    dl_phy_to_mac_grant(&ue_dl_cfg.cfg.pdsch.grant, &dci_dl, &mac_grant);

    // Save ACK resource configuration
    srslte_pdsch_ack_resource_t ack_resource = {dci_dl.dai, dci_dl.location.ncce};

    // Send grant to MAC and get action for this TB, then call tb_decoded to unlock MAC
    phy->stack->new_grant_dl(cc_idx, mac_grant, &dl_action);
    decode_pdsch(ack_resource, &dl_action, dl_ack);
    phy->stack->tb_decoded(cc_idx, mac_grant, dl_ack);
  }

  /* Decode PHICH */
  decode_phich();

  return true;
}

bool cc_worker::work_dl_mbsfn(srslte_mbsfn_cfg_t mbsfn_cfg)
{
  mac_interface_phy_lte::tb_action_dl_t dl_action;

  // Configure MBSFN settings
  srslte_ue_dl_set_mbsfn_area_id(&ue_dl, mbsfn_cfg.mbsfn_area_id);
  srslte_ue_dl_set_non_mbsfn_region(&ue_dl, mbsfn_cfg.non_mbsfn_region_length);

  sf_cfg_dl.sf_type = SRSLTE_SF_MBSFN;

  // Set MBSFN channel estimation
  chest_mbsfn_cfg.mbsfn_area_id = mbsfn_cfg.mbsfn_area_id;
  ue_dl_cfg.chest_cfg = chest_mbsfn_cfg;

  /* Do FFT and extract PDCCH LLR, or quit if no actions are required in this subframe */
  if (srslte_ue_dl_decode_fft_estimate(&ue_dl, &sf_cfg_dl, &ue_dl_cfg) < 0) {
    Error("Getting PDCCH FFT estimate\n");
    return false;
  }

  decode_pdcch_ul();

  if (mbsfn_cfg.enable) {
    srslte_configure_pmch(&pmch_cfg, &cell, &mbsfn_cfg);
    srslte_ra_dl_compute_nof_re(&cell, &sf_cfg_dl, &pmch_cfg.pdsch_cfg.grant);

    // Send grant to MAC and get action for this TB, then call tb_decoded to unlock MAC
    phy->stack->new_mch_dl(pmch_cfg.pdsch_cfg.grant, &dl_action);
    bool mch_decoded = true;
    if (!decode_pmch(&dl_action, &mbsfn_cfg)) {
      mch_decoded = false;
    }
    phy->stack->mch_decoded((uint32_t)pmch_cfg.pdsch_cfg.grant.tb[0].tbs / 8, mch_decoded);
  } else if (mbsfn_cfg.is_mcch) {
    // release lock in phy_common
    phy->set_mch_period_stop(0);
  }

  /* Decode PHICH */
  decode_phich();

  return true;
}

void cc_worker::dl_phy_to_mac_grant(srslte_pdsch_grant_t*                         phy_grant,
                                    srslte_dci_dl_t*                              dl_dci,
                                    srsue::mac_interface_phy_lte::mac_grant_dl_t* mac_grant)
{
  /* Fill MAC dci structure */
  mac_grant->pid  = dl_dci->pid;
  mac_grant->rnti = dl_dci->rnti;

  for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    mac_grant->tb[i].ndi         = dl_dci->tb[i].ndi;
    mac_grant->tb[i].ndi_present = (dl_dci->tb[i].mcs_idx <= 28);
    mac_grant->tb[i].tbs         = phy_grant->tb[i].enabled ? (phy_grant->tb[i].tbs / (uint32_t)8) : 0;
    mac_grant->tb[i].rv          = phy_grant->tb[i].rv;
  }

  // If SIB dci, use PID to signal TTI to obtain RV from higher layers
  if (mac_grant->rnti == SRSLTE_SIRNTI) {
    mac_grant->pid = CURRENT_TTI;
  }
}

int cc_worker::decode_pdcch_dl()
{
  int nof_grants = 0;

  srslte_dci_dl_t dci[SRSLTE_MAX_CARRIERS];
  ZERO_OBJECT(dci);

  uint16_t dl_rnti = phy->stack->get_dl_sched_rnti(CURRENT_TTI);
  if (dl_rnti) {

    /* Blind search first without cross scheduling then with it if enabled */
    for (int i = 0; i < (phy->cif_enabled ? 2 : 1) && !nof_grants; i++) {
      fill_dci_cfg(&ue_dl_cfg.dci_cfg, i > 0);
      Debug("PDCCH looking for rnti=0x%x\n", dl_rnti);
      nof_grants = srslte_ue_dl_find_dl_dci(&ue_dl, &sf_cfg_dl, &ue_dl_cfg, dl_rnti, dci);
      if (nof_grants < 0) {
        Error("Looking for DL grants\n");
        return -1;
      }
    }

    // If RAR dci, save TTI
    if (nof_grants > 0 && SRSLTE_RNTI_ISRAR(dl_rnti)) {
      phy->set_rar_grant_tti(CURRENT_TTI);
    }

    for (int k = 0; k < nof_grants; k++) {
      // Save dci to CC index
      set_dl_pending_grant(dci[k].cif_present ? dci[k].cif : cc_idx, &dci[k]);

      // Logging
      char str[512];
      srslte_dci_dl_info(&dci[k], str, 512);
      Info("PDCCH: cc=%d, %s, snr=%.1f dB\n", cc_idx, str, ue_dl.chest_res.snr_db);
    }
  }
  return nof_grants;
}

int cc_worker::decode_pdsch(srslte_pdsch_ack_resource_t            ack_resource,
                            mac_interface_phy_lte::tb_action_dl_t* action,
                            bool                                   mac_acks[SRSLTE_MAX_CODEWORDS])
{

  srslte_pdsch_res_t pdsch_dec[SRSLTE_MAX_CODEWORDS];
  ZERO_OBJECT(pdsch_dec);

  // See if at least 1 codeword needs to be decoded. If not need to be decode, resend ACK
  bool decode_enable = false;
  bool tb_enable[SRSLTE_MAX_CODEWORDS];
  for (uint32_t tb = 0; tb < SRSLTE_MAX_CODEWORDS; tb++) {
    tb_enable[tb] = ue_dl_cfg.cfg.pdsch.grant.tb[tb].enabled;
    if (action->tb[tb].enabled) {
      decode_enable = true;

      // Prepare I/O based on action
      pdsch_dec[tb].payload                  = action->tb[tb].payload;
      ue_dl_cfg.cfg.pdsch.softbuffers.rx[tb] = action->tb[tb].softbuffer.rx;

      // Use RV from higher layers
      ue_dl_cfg.cfg.pdsch.grant.tb[tb].rv = action->tb[tb].rv;

    } else {
      // If this TB is duplicate, indicate PDSCH to skip it
      ue_dl_cfg.cfg.pdsch.grant.tb[tb].enabled = false;
    }
  }

  // Run PDSCH decoder
  if (decode_enable) {
    if (srslte_ue_dl_decode_pdsch(&ue_dl, &sf_cfg_dl, &ue_dl_cfg.cfg.pdsch, pdsch_dec)) {
      Error("ERROR: Decoding PDSCH\n");
    }
  }

  // Generate ACKs for MAC and PUCCH
  uint8_t pending_acks[SRSLTE_MAX_CODEWORDS];
  for (uint32_t tb = 0; tb < SRSLTE_MAX_CODEWORDS; tb++) {
    // For MAC, set to true if it's a duplicate
    mac_acks[tb] = action->tb[tb].enabled ? pdsch_dec[tb].crc : true;

    // For PUCCH feedback, need to send even if duplicate, but only those CW that were enabled before disabling in th
    // grant
    pending_acks[tb] = tb_enable[tb] ? mac_acks[tb] : 2;
  }

  if (action->generate_ack && ue_dl_cfg.cfg.pdsch.grant.nof_tb > 0) {
    phy->set_dl_pending_ack(&sf_cfg_dl, cc_idx, pending_acks, ack_resource);
  }

  if (decode_enable) {
    // Metrics
    dl_metrics.mcs         = ue_dl_cfg.cfg.pdsch.grant.tb[0].mcs_idx;
    dl_metrics.turbo_iters = pdsch_dec->avg_iterations_block / 2;

    // Logging
    char str[512];
    srslte_pdsch_rx_info(&ue_dl_cfg.cfg.pdsch, pdsch_dec, str, 512);
    Info("PDSCH: cc=%d, %s, snr=%.1f dB\n", cc_idx, str, ue_dl.chest_res.snr_db);
  }

  return SRSLTE_SUCCESS;
}

int cc_worker::decode_pmch(mac_interface_phy_lte::tb_action_dl_t* action, srslte_mbsfn_cfg_t* mbsfn_cfg)
{
  srslte_pdsch_res_t pmch_dec;

  pmch_cfg.area_id                     = mbsfn_cfg->mbsfn_area_id;
  pmch_cfg.pdsch_cfg.softbuffers.rx[0] = action->tb[0].softbuffer.rx;
  pmch_dec.payload                     = action->tb[0].payload;

  if (action->tb[0].enabled) {

    srslte_softbuffer_rx_reset_tbs(pmch_cfg.pdsch_cfg.softbuffers.rx[0], pmch_cfg.pdsch_cfg.grant.tb[0].tbs);

    if (srslte_ue_dl_decode_pmch(&ue_dl, &sf_cfg_dl, &pmch_cfg, &pmch_dec)) {
      Error("Decoding PMCH\n");
      return -1;
    }

    // Store metrics
    dl_metrics.mcs = pmch_cfg.pdsch_cfg.grant.tb[0].mcs_idx;

    Info("PMCH: l_crb=%2d, tbs=%d, mcs=%d, crc=%s, snr=%.1f dB, n_iter=%.1f\n",
         pmch_cfg.pdsch_cfg.grant.nof_prb,
         pmch_cfg.pdsch_cfg.grant.tb[0].tbs / 8,
         pmch_cfg.pdsch_cfg.grant.tb[0].mcs_idx,
         pmch_dec.crc ? "OK" : "KO",
         ue_dl.chest_res.snr_db,
         pmch_dec.avg_iterations_block);


    if (pmch_dec.crc) {
      return 1;
    }

  } else {
    Warning("Received dci for TBS=0\n");
  }
  return 0;
}

void cc_worker::decode_phich()
{
  srslte_dci_ul_t      dci_ul      = {};
  srslte_phich_grant_t phich_grant = {};
  srslte_phich_res_t   phich_res   = {};

  // Receive PHICH, in TDD might be more than one
  for (uint32_t I_phich = 0; I_phich < 2; I_phich++) {
    phich_grant.I_phich = I_phich;
    if (phy->get_ul_pending_ack(&sf_cfg_dl, cc_idx, &phich_grant, &dci_ul)) {
      if (srslte_ue_dl_decode_phich(&ue_dl, &sf_cfg_dl, &ue_dl_cfg, &phich_grant, &phich_res)) {
        Error("Decoding PHICH\n");
      }
      phy->set_ul_received_ack(&sf_cfg_dl, cc_idx, phich_res.ack_value, I_phich, &dci_ul);
      Info("PHICH: hi=%d, corr=%.1f, I_lowest=%d, n_dmrs=%d, I_phich=%d\n",
           phich_res.ack_value,
           phich_res.distance,
           phich_grant.n_prb_lowest,
           phich_grant.n_dmrs,
           I_phich);
    }
  }
}

void cc_worker::update_measurements()
{
  float snr_ema_coeff = phy->args->snr_ema_coeff;

  // In TDD, ignore special subframes without PDSCH
  if (srslte_sfidx_tdd_type(sf_cfg_dl.tdd_config, CURRENT_SFIDX) == SRSLTE_TDD_SF_S &&
      srslte_sfidx_tdd_nof_dw(sf_cfg_dl.tdd_config) < 4) {
    return;
  }

  // Average RSRQ over DEFAULT_MEAS_PERIOD_MS then sent to RRC
  float rsrq_db = ue_dl.chest_res.rsrq_db;
  if (std::isnormal(rsrq_db)) {
    if (!(CURRENT_TTI % phy->pcell_report_period) || !phy->avg_rsrq_db) {
      phy->avg_rsrq_db = rsrq_db;
    } else {
      phy->avg_rsrq_db = SRSLTE_VEC_CMA(rsrq_db, phy->avg_rsrq_db, CURRENT_TTI % phy->pcell_report_period);
    }
  }

  // Average RSRP taken from CRS
  float rsrp_lin = ue_dl.chest_res.rsrp;
  if (std::isnormal(rsrp_lin)) {
    if (!phy->avg_rsrp[cc_idx] && !std::isnan(phy->avg_rsrp[cc_idx])) {
      phy->avg_rsrp[cc_idx] = SRSLTE_VEC_EMA(rsrp_lin, phy->avg_rsrp[cc_idx], snr_ema_coeff);
    } else {
      phy->avg_rsrp[cc_idx] = rsrp_lin;
    }
  }

  /* Correct absolute power measurements by RX gain offset */
  float rsrp_dbm = ue_dl.chest_res.rsrp_dbm - phy->rx_gain_offset;

  // Serving cell RSRP measurements are averaged over DEFAULT_MEAS_PERIOD_MS then sent to RRC
  if (std::isnormal(rsrp_dbm)) {
    if (!(CURRENT_TTI % phy->pcell_report_period) || !phy->avg_rsrp_dbm[cc_idx]) {
      phy->avg_rsrp_dbm[cc_idx] = rsrp_dbm;
    } else {
      phy->avg_rsrp_dbm[cc_idx] =
          SRSLTE_VEC_CMA(rsrp_dbm, phy->avg_rsrp_dbm[cc_idx], CURRENT_TTI % phy->pcell_report_period);
    }
  }

  // Compute PL
  float tx_crs_power    = ue_dl_cfg.cfg.pdsch.rs_power;
  phy->pathloss[cc_idx] = tx_crs_power - phy->avg_rsrp_dbm[cc_idx];

  // Average noise
  float cur_noise = ue_dl.chest_res.noise_estimate;
  if (std::isnormal(cur_noise)) {
    if (!phy->avg_noise) {
      phy->avg_noise[cc_idx] = cur_noise;
    } else {
      phy->avg_noise[cc_idx] = SRSLTE_VEC_EMA(cur_noise, phy->avg_noise[cc_idx], snr_ema_coeff);
    }
  }

  // Average snr in the log domain
  if (std::isnormal(ue_dl.chest_res.snr_db)) {
    if (!phy->avg_noise) {
      phy->avg_snr_db_cqi[cc_idx] = ue_dl.chest_res.snr_db;
    } else {
      phy->avg_snr_db_cqi[cc_idx] = SRSLTE_VEC_EMA(ue_dl.chest_res.snr_db, phy->avg_snr_db_cqi[cc_idx], snr_ema_coeff);
    }
  }

  // Store metrics
  dl_metrics.n        = phy->avg_noise[cc_idx];
  dl_metrics.rsrp     = phy->avg_rsrp_dbm[cc_idx];
  dl_metrics.rsrq     = phy->avg_rsrq_db;
  dl_metrics.rssi     = phy->avg_rssi_dbm;
  dl_metrics.pathloss = phy->pathloss[cc_idx];
  dl_metrics.sinr     = phy->avg_snr_db_cqi[cc_idx];
  dl_metrics.sync_err = ue_dl.chest_res.sync_error;

  phy->set_dl_metrics(dl_metrics, cc_idx);
  phy->set_ul_metrics(ul_metrics, cc_idx);
}

/************
 *
 * Uplink Functions
 *
 */

bool cc_worker::work_ul(srslte_uci_data_t* uci_data)
{

  bool signal_ready;

  srslte_dci_ul_t                   dci_ul       = {};
  mac_interface_phy_lte::mac_grant_ul_t ul_mac_grant = {};
  mac_interface_phy_lte::tb_action_ul_t ul_action    = {};
  uint32_t                          pid          = 0;

  bool ul_grant_available = phy->get_ul_pending_grant(&sf_cfg_ul, cc_idx, &pid, &dci_ul);
  ul_mac_grant.phich_available =
      phy->get_ul_received_ack(&sf_cfg_ul, cc_idx, &ul_mac_grant.hi_value, ul_grant_available ? NULL : &dci_ul);

  // If there is no grant, pid is from current TX TTI
  if (!ul_grant_available) {
    pid = phy->ul_pidof(CURRENT_TTI_TX, &sf_cfg_ul.tdd_config);
  }

  /* Generate CQI reports if required, note that in case both aperiodic
   * and periodic ones present, only aperiodic is sent (36.213 section 7.2) */
  if (ul_grant_available && dci_ul.cqi_request) {
    set_uci_aperiodic_cqi(uci_data);
  } else {
    /* Check PCell and enabled secondary cells */
    if (cc_idx == 0 || phy->scell_cfg[cc_idx].enabled) {
      set_uci_periodic_cqi(uci_data);
    }
  }

  /* Send UL dci or HARQ information (from PHICH) to MAC and receive actions*/
  if (ul_grant_available || ul_mac_grant.phich_available) {

    // Read last TB info from last retx for this PID
    ue_ul_cfg.ul_cfg.pusch.grant.last_tb = phy->last_ul_tb[pid][cc_idx];

    // Generate PHY grant
    if (srslte_ue_ul_dci_to_pusch_grant(&ue_ul, &sf_cfg_ul, &ue_ul_cfg, &dci_ul, &ue_ul_cfg.ul_cfg.pusch.grant)) {
      Error("Converting DCI message to UL dci\n");
    }

    // Save TBS info for next retx
    phy->last_ul_tb[pid][cc_idx] = ue_ul_cfg.ul_cfg.pusch.grant.tb;

    // Fill MAC dci
    ul_phy_to_mac_grant(&ue_ul_cfg.ul_cfg.pusch.grant, &dci_ul, pid, ul_grant_available, &ul_mac_grant);

    phy->stack->new_grant_ul(cc_idx, ul_mac_grant, &ul_action);

    // Calculate PUSCH Hopping procedure
    ue_ul_cfg.ul_cfg.hopping.current_tx_nb = ul_action.current_tx_nb;
    srslte_ue_ul_pusch_hopping(&ue_ul, &sf_cfg_ul, &ue_ul_cfg, &ue_ul_cfg.ul_cfg.pusch.grant);
  }

  // Set UL RNTI
  if (ul_grant_available || ul_mac_grant.phich_available) {
    ue_ul_cfg.ul_cfg.pusch.rnti = dci_ul.rnti;
  } else {
    ue_ul_cfg.ul_cfg.pucch.rnti = phy->stack->get_ul_sched_rnti(CURRENT_TTI_TX);
  }

  // PCell sends SR and ACK
  if (cc_idx == 0) {
    set_uci_sr(uci_data);
    // This must be called after set_uci_sr() and set_uci_*_cqi
    set_uci_ack(uci_data, ul_grant_available, dci_ul.dai, ul_action.tb.enabled);
  }

  // Generate uplink signal, include uci data on only PCell
  signal_ready = encode_uplink(&ul_action, (cc_idx == 0) ? uci_data : NULL);

  // Prepare to receive ACK through PHICH
  if (ul_action.expect_ack) {
    srslte_phich_grant_t phich_grant = {};
    phich_grant.I_phich              = 0;
    if (cell.frame_type == SRSLTE_TDD && sf_cfg_ul.tdd_config.sf_config == 0) {
      if ((sf_cfg_ul.tti % 10) == 4 || (sf_cfg_ul.tti % 10) == 9) {
        phich_grant.I_phich = 1;
      }
    }
    phich_grant.n_prb_lowest = ue_ul_cfg.ul_cfg.pusch.grant.n_prb_tilde[0];
    phich_grant.n_dmrs       = ue_ul_cfg.ul_cfg.pusch.grant.n_dmrs;

    phy->set_ul_pending_ack(&sf_cfg_ul, cc_idx, phich_grant, &dci_ul);
  }

  return signal_ready;
}

void cc_worker::ul_phy_to_mac_grant(srslte_pusch_grant_t*                         phy_grant,
                                    srslte_dci_ul_t*                              dci_ul,
                                    uint32_t                                      pid,
                                    bool                                          ul_grant_available,
                                    srsue::mac_interface_phy_lte::mac_grant_ul_t* mac_grant)
{
  if (mac_grant->phich_available && !dci_ul->rnti) {
    mac_grant->rnti = phy->stack->get_ul_sched_rnti(CURRENT_TTI);
  } else {
    mac_grant->rnti = dci_ul->rnti;
  }
  mac_grant->tb.ndi         = dci_ul->tb.ndi;
  mac_grant->tb.ndi_present = ul_grant_available;
  mac_grant->tb.tbs         = phy_grant->tb.tbs / (uint32_t)8;
  mac_grant->tb.rv          = phy_grant->tb.rv;
  mac_grant->pid            = pid;
}

int cc_worker::decode_pdcch_ul()
{
  int nof_grants = 0;

  srslte_dci_ul_t dci[SRSLTE_MAX_CARRIERS];
  ZERO_OBJECT(dci);

  uint16_t ul_rnti = phy->stack->get_ul_sched_rnti(CURRENT_TTI);

  if (ul_rnti) {
    /* Blind search first without cross scheduling then with it if enabled */
    for (int i = 0; i < (phy->cif_enabled ? 2 : 1) && !nof_grants; i++) {
      fill_dci_cfg(&ue_dl_cfg.dci_cfg, i > 0);
      nof_grants = srslte_ue_dl_find_ul_dci(&ue_dl, &sf_cfg_dl, &ue_dl_cfg, ul_rnti, dci);
      if (nof_grants < 0) {
        Error("Looking for UL grants\n");
        return -1;
      }
    }

    /* Convert every DCI message to UL dci */
    for (int k = 0; k < nof_grants; k++) {

      // If the DCI does not have Carrier Indicator Field then indicate in which carrier the dci was found
      uint32_t cc_idx_grant = dci[k].cif_present ? dci[k].cif : cc_idx;

      // Save DCI
      phy->set_ul_pending_grant(&sf_cfg_dl, cc_idx_grant, &dci[k]);

      // Logging
      char str[512];
      srslte_dci_ul_info(&dci[k], str, 512);
      Info("PDCCH: cc=%d, %s, snr=%.1f dB\n", cc_idx_grant, str, ue_dl.chest_res.snr_db);
    }
  }

  return nof_grants;
}

bool cc_worker::encode_uplink(mac_interface_phy_lte::tb_action_ul_t* action, srslte_uci_data_t* uci_data)
{
  srslte_pusch_data_t data = {};
  ue_ul_cfg.cc_idx         = cc_idx;

  // Setup input data
  if (action) {
    data.ptr                              = action->tb.payload;
    ue_ul_cfg.ul_cfg.pusch.softbuffers.tx = action->tb.softbuffer.tx;
  }

  // Set UCI data and configuration
  if (uci_data) {
    data.uci                       = uci_data->value;
    ue_ul_cfg.ul_cfg.pusch.uci_cfg = uci_data->cfg;
    ue_ul_cfg.ul_cfg.pucch.uci_cfg = uci_data->cfg;
  } else {
    ZERO_OBJECT(ue_ul_cfg.ul_cfg.pusch.uci_cfg);
    ZERO_OBJECT(ue_ul_cfg.ul_cfg.pucch.uci_cfg);
  }

  // Use RV from higher layers
  ue_ul_cfg.ul_cfg.pusch.grant.tb.rv = action->tb.rv;

  // Setup PUSCH grant
  ue_ul_cfg.grant_available = action->tb.enabled;

  // Set UL RNTI
  ue_ul_cfg.ul_cfg.pucch.rnti = phy->stack->get_ul_sched_rnti(CURRENT_TTI_TX);

  // Encode signal
  int ret = srslte_ue_ul_encode(&ue_ul, &sf_cfg_ul, &ue_ul_cfg, &data);
  if (ret < 0) {
    Error("Encoding UL cc=%d\n", cc_idx);
  }

  // Store metrics
  if (action->tb.enabled) {
    ul_metrics.mcs = ue_ul_cfg.ul_cfg.pusch.grant.tb.mcs_idx;
  }

  // Logging
  char str[512];
  if (srslte_ue_ul_info(&ue_ul_cfg, &sf_cfg_ul, &data.uci, str, 512)) {
    Info("%s\n", str);
  }

  return ret > 0;
}

void cc_worker::set_uci_sr(srslte_uci_data_t* uci_data)
{
  Debug("set_uci_sr() query: sr_enabled=%d, last_tx_tti=%d\n", phy->sr_enabled, phy->sr_last_tx_tti);
  if (srslte_ue_ul_gen_sr(&ue_ul_cfg, &sf_cfg_ul, uci_data, phy->sr_enabled)) {
    if (phy->sr_enabled) {
      phy->sr_last_tx_tti = CURRENT_TTI_TX;
      phy->sr_enabled     = false;
      Debug("set_uci_sr() sending SR: sr_enabled=%d, last_tx_tti=%d\n", phy->sr_enabled, phy->sr_last_tx_tti);
    }
  }
}

uint32_t cc_worker::get_wideband_cqi()
{
  int cqi_fixed = phy->args->cqi_fixed;
  int cqi_max   = phy->args->cqi_max;

  uint32_t wb_cqi_value = srslte_cqi_from_snr(phy->avg_snr_db_cqi[cc_idx] + ue_dl_cfg.snr_to_cqi_offset);

  if (cqi_fixed >= 0) {
    wb_cqi_value = cqi_fixed;
  } else if (cqi_max >= 0 && wb_cqi_value > (uint32_t)cqi_max) {
    wb_cqi_value = cqi_max;
  }

  return wb_cqi_value;
}

void cc_worker::set_uci_periodic_cqi(srslte_uci_data_t* uci_data)
{
  srslte_ue_dl_gen_cqi_periodic(&ue_dl, &ue_dl_cfg, get_wideband_cqi(), CURRENT_TTI_TX, uci_data);
}

void cc_worker::set_uci_aperiodic_cqi(srslte_uci_data_t* uci_data)
{
  if (ue_dl_cfg.cfg.cqi_report.aperiodic_configured) {
    srslte_ue_dl_gen_cqi_aperiodic(&ue_dl, &ue_dl_cfg, get_wideband_cqi(), uci_data);
  } else {
    Warning("Received CQI request but aperiodic mode is not configured\n");
  }
}

void cc_worker::set_uci_ack(srslte_uci_data_t* uci_data,
                            bool               is_grant_available,
                            uint32_t           V_dai_ul,
                            bool               is_pusch_available)
{

  srslte_pdsch_ack_t ack_info                = {};
  uint32_t           nof_configured_carriers = 0;

  // Only PCell generates ACK for all SCell
  for (uint32_t cc_idx = 0; cc_idx < phy->args->nof_carriers; cc_idx++) {
    if (cc_idx == 0 || phy->scell_cfg[cc_idx].configured) {
      phy->get_dl_pending_ack(&sf_cfg_ul, cc_idx, &ack_info.cc[cc_idx]);
      nof_configured_carriers++;
    }
  }

  // Set ACK length for CA (default value is set to DTX)
  if (ue_ul_cfg.ul_cfg.pucch.ack_nack_feedback_mode != SRSLTE_PUCCH_ACK_NACK_FEEDBACK_MODE_NORMAL) {
    if (ue_dl_cfg.cfg.tm > SRSLTE_TM2) {
      /* TM3, TM4 */
      uci_data->cfg.ack.nof_acks = nof_configured_carriers * SRSLTE_MAX_CODEWORDS;
    } else {
      /* TM1, TM2 */
      uci_data->cfg.ack.nof_acks = nof_configured_carriers;
    }
  }

  // Configure ACK parameters
  ack_info.is_grant_available     = is_grant_available;
  ack_info.is_pusch_available     = is_pusch_available;
  ack_info.V_dai_ul               = V_dai_ul;
  ack_info.tdd_ack_bundle         = ue_ul_cfg.ul_cfg.pucch.tdd_ack_bundle;
  ack_info.simul_cqi_ack          = ue_ul_cfg.ul_cfg.pucch.simul_cqi_ack;
  ack_info.ack_nack_feedback_mode = ue_ul_cfg.ul_cfg.pucch.ack_nack_feedback_mode;
  ack_info.nof_cc                 = nof_configured_carriers;
  ack_info.transmission_mode      = ue_dl_cfg.cfg.tm;

  // Generate ACK/NACK bits
  srslte_ue_dl_gen_ack(&ue_dl, &sf_cfg_dl, &ack_info, uci_data);
}

/************
 *
 * Configuration Functions
 *
 */

srslte_cqi_report_mode_t cc_worker::aperiodic_mode(cqi_report_mode_aperiodic_e mode)
{
  switch (mode) {
    case cqi_report_mode_aperiodic_e::rm12:
      return SRSLTE_CQI_MODE_12;
    case cqi_report_mode_aperiodic_e::rm20:
      return SRSLTE_CQI_MODE_20;
    case cqi_report_mode_aperiodic_e::rm22:
      return SRSLTE_CQI_MODE_22;
    case cqi_report_mode_aperiodic_e::rm30:
      return SRSLTE_CQI_MODE_30;
    case cqi_report_mode_aperiodic_e::rm31:
      return SRSLTE_CQI_MODE_31;
    case cqi_report_mode_aperiodic_e::rm10_v1310:
    case cqi_report_mode_aperiodic_e::rm11_v1310:
    case cqi_report_mode_aperiodic_e::rm32_v1250:
      fprintf(stderr, "Aperiodic mode %s not handled\n", mode.to_string().c_str());
    default:
      return SRSLTE_CQI_MODE_NA;
  }
}

void cc_worker::parse_antenna_info(phys_cfg_ded_s* dedicated)
{
  if (dedicated->ant_info_r10_present) {
    // Parse Release 10
    ant_info_ded_r10_s::tx_mode_r10_e_::options tx_mode =
        dedicated->ant_info_r10->explicit_value_r10().tx_mode_r10.value;
    if ((srslte_tm_t)tx_mode < SRSLTE_TMINV) {
      ue_dl_cfg.cfg.tm = (srslte_tm_t)tx_mode;
    } else {
      fprintf(stderr,
              "Transmission mode (R10) %s is not supported\n",
              dedicated->ant_info_r10->explicit_value_r10().tx_mode_r10.to_string().c_str());
    }
  } else if (dedicated->ant_info_present &&
             dedicated->ant_info.type() == phys_cfg_ded_s::ant_info_c_::types::explicit_value) {
    // Parse Release 8
    ant_info_ded_s::tx_mode_e_::options tx_mode = dedicated->ant_info.explicit_value().tx_mode.value;
    if ((srslte_tm_t)tx_mode < SRSLTE_TMINV) {
      ue_dl_cfg.cfg.tm = (srslte_tm_t)tx_mode;
    } else {
      fprintf(stderr,
              "Transmission mode (R8) %s is not supported\n",
              dedicated->ant_info.explicit_value().tx_mode.to_string().c_str());
    }
  } else {
    if (cell.nof_ports == 1) {
      // No antenna info provided
      ue_dl_cfg.cfg.tm = SRSLTE_TM1;
    } else {
      // No antenna info provided
      ue_dl_cfg.cfg.tm = SRSLTE_TM2;
    }
  }
}

void cc_worker::parse_pucch_config(phy_interface_rrc_lte::phy_cfg_t* phy_cfg)
{
  phy_interface_rrc_lte::phy_cfg_common_t* common    = &phy_cfg->common;
  phys_cfg_ded_s*                      dedicated = &phy_cfg->dedicated;

  /* PUCCH configuration */
  bzero(&ue_ul_cfg.ul_cfg.pucch, sizeof(srslte_pucch_cfg_t));
  ue_ul_cfg.ul_cfg.pucch.delta_pucch_shift = common->pucch_cnfg.delta_pucch_shift.to_number();
  ue_ul_cfg.ul_cfg.pucch.N_cs              = common->pucch_cnfg.n_cs_an;
  ue_ul_cfg.ul_cfg.pucch.n_rb_2            = common->pucch_cnfg.n_rb_cqi;

  /* PUCCH Scheduling configuration */
  ue_ul_cfg.ul_cfg.pucch.n_pucch_1[0] = 0; // TODO: n_pucch_1 for SPS
  ue_ul_cfg.ul_cfg.pucch.n_pucch_1[1] = 0;
  ue_ul_cfg.ul_cfg.pucch.n_pucch_1[2] = 0;
  ue_ul_cfg.ul_cfg.pucch.n_pucch_1[3] = 0;
  ue_ul_cfg.ul_cfg.pucch.N_pucch_1    = common->pucch_cnfg.n1_pucch_an;
  if (dedicated->cqi_report_cfg.cqi_report_periodic_present and
      dedicated->cqi_report_cfg.cqi_report_periodic.type().value == setup_e::setup) {
    ue_ul_cfg.ul_cfg.pucch.n_pucch_2     = dedicated->cqi_report_cfg.cqi_report_periodic.setup().cqi_pucch_res_idx;
    ue_ul_cfg.ul_cfg.pucch.simul_cqi_ack = dedicated->cqi_report_cfg.cqi_report_periodic.setup().simul_ack_nack_and_cqi;
  } else {
    // FIXME: put is_pucch_configured flag here?
    ue_ul_cfg.ul_cfg.pucch.n_pucch_2     = 0;
    ue_ul_cfg.ul_cfg.pucch.simul_cqi_ack = false;
  }

  /* SR configuration */
  if (dedicated->sched_request_cfg_present and dedicated->sched_request_cfg.type() == setup_e::setup) {
    ue_ul_cfg.ul_cfg.pucch.I_sr          = dedicated->sched_request_cfg.setup().sr_cfg_idx;
    ue_ul_cfg.ul_cfg.pucch.n_pucch_sr    = dedicated->sched_request_cfg.setup().sr_pucch_res_idx;
    ue_ul_cfg.ul_cfg.pucch.sr_configured = true;
  } else {
    ue_ul_cfg.ul_cfg.pucch.I_sr          = 0;
    ue_ul_cfg.ul_cfg.pucch.n_pucch_sr    = 0;
    ue_ul_cfg.ul_cfg.pucch.sr_configured = false;
  }

  if (dedicated->pucch_cfg_ded.tdd_ack_nack_feedback_mode_present) {
    ue_ul_cfg.ul_cfg.pucch.tdd_ack_bundle =
        dedicated->pucch_cfg_ded.tdd_ack_nack_feedback_mode == pucch_cfg_ded_s::tdd_ack_nack_feedback_mode_e_::bundling;
  } else {
    ue_ul_cfg.ul_cfg.pucch.tdd_ack_bundle = false;
  }

  if (dedicated->pucch_cfg_ded_v1020_present) {
    pucch_cfg_ded_v1020_s* pucch_cfg_ded = dedicated->pucch_cfg_ded_v1020.get();

    if (pucch_cfg_ded->pucch_format_r10_present) {

      typedef pucch_cfg_ded_v1020_s::pucch_format_r10_c_ pucch_format_r10_t;
      pucch_format_r10_t*                                pucch_format_r10 = &pucch_cfg_ded->pucch_format_r10;

      if (pucch_format_r10->type() == pucch_format_r10_t::types::format3_r10) {
        // Select feedback mode
        ue_ul_cfg.ul_cfg.pucch.ack_nack_feedback_mode = SRSLTE_PUCCH_ACK_NACK_FEEDBACK_MODE_PUCCH3;

        pucch_format3_conf_r13_s* format3_r13 = &pucch_format_r10->format3_r10();
        for (uint32_t n = 0; n < SRSLTE_MIN(format3_r13->n3_pucch_an_list_r13.size(), SRSLTE_PUCCH_SIZE_AN_CS); n++) {
          ue_ul_cfg.ul_cfg.pucch.n3_pucch_an_list[n] = format3_r13->n3_pucch_an_list_r13[n];
        }
        if (format3_r13->two_ant_port_activ_pucch_format3_r13_present) {
          if (format3_r13->two_ant_port_activ_pucch_format3_r13.type() == setup_e::setup) {
            // TODO: UL MIMO Configure PUCCH two antenna port
          } else {
            // TODO: UL MIMO Disable two antenna port
          }
        }
      } else if (pucch_format_r10->type() == pucch_cfg_ded_v1020_s::pucch_format_r10_c_::types::ch_sel_r10) {

        typedef pucch_format_r10_t::ch_sel_r10_s_ ch_sel_r10_t;
        ch_sel_r10_t*                             ch_sel_r10 = &pucch_format_r10->ch_sel_r10();

        if (ch_sel_r10->n1_pucch_an_cs_r10_present) {
          typedef ch_sel_r10_t::n1_pucch_an_cs_r10_c_ n1_pucch_an_cs_r10_t;
          n1_pucch_an_cs_r10_t*                       n1_pucch_an_cs_r10 = &ch_sel_r10->n1_pucch_an_cs_r10;

          if (n1_pucch_an_cs_r10->type() == setup_e::setup) {
            // Select feedback mode
            ue_ul_cfg.ul_cfg.pucch.ack_nack_feedback_mode = SRSLTE_PUCCH_ACK_NACK_FEEDBACK_MODE_CS;

            typedef n1_pucch_an_cs_r10_t::setup_s_::n1_pucch_an_cs_list_r10_l_ n1_pucch_an_cs_list_r10_t;
            n1_pucch_an_cs_list_r10_t                                          n1_pucch_an_cs_list =
                ch_sel_r10->n1_pucch_an_cs_r10.setup().n1_pucch_an_cs_list_r10;
            for (uint32_t i = 0; i < SRSLTE_MIN(n1_pucch_an_cs_list.size(), SRSLTE_PUCCH_NOF_AN_CS); i++) {
              n1_pucch_an_cs_r10_l n1_pucch_an_cs = n1_pucch_an_cs_list[i];
              for (uint32_t j = 0; j < SRSLTE_PUCCH_SIZE_AN_CS; j++) {
                ue_ul_cfg.ul_cfg.pucch.n1_pucch_an_cs[j][i] = n1_pucch_an_cs[j];
              }
            }
          } else {
            ue_ul_cfg.ul_cfg.pucch.ack_nack_feedback_mode = SRSLTE_PUCCH_ACK_NACK_FEEDBACK_MODE_NORMAL;
          }
        }
      } else {
        // Do nothing
      }
    }
  }
}

/* Translates RRC structs into PHY structs
 */
void cc_worker::set_pcell_config(phy_interface_rrc_lte::phy_cfg_t* phy_cfg)
{
  phy_interface_rrc_lte::phy_cfg_common_t* common    = &phy_cfg->common;
  phys_cfg_ded_s*                      dedicated = &phy_cfg->dedicated;

  // Configure PDSCH
  if (dedicated->pdsch_cfg_ded_present && common->pdsch_cnfg.p_b < 4) {
    ue_dl_cfg.cfg.pdsch.p_a         = dedicated->pdsch_cfg_ded.p_a.to_number();
    ue_dl_cfg.cfg.pdsch.p_b         = common->pdsch_cnfg.p_b;
    ue_dl_cfg.cfg.pdsch.power_scale = true;
  } else {
    ue_dl_cfg.cfg.pdsch.power_scale = false;
  }
  ue_dl_cfg.cfg.pdsch.rs_power = (float)common->pdsch_cnfg.ref_sig_pwr;
  parse_antenna_info(dedicated);

  // Configure PUSCH
  ue_ul_cfg.ul_cfg.pusch.enable_64qam =
      phy_cfg->common.pusch_cnfg.pusch_cfg_basic.enable64_qam && phy_cfg->common.rrc_enable_64qam;

  /* PUSCH DMRS signal configuration */
  bzero(&ue_ul_cfg.ul_cfg.dmrs, sizeof(srslte_refsignal_dmrs_pusch_cfg_t));
  ue_ul_cfg.ul_cfg.dmrs.group_hopping_en    = common->pusch_cnfg.ul_ref_sigs_pusch.group_hop_enabled;
  ue_ul_cfg.ul_cfg.dmrs.sequence_hopping_en = common->pusch_cnfg.ul_ref_sigs_pusch.seq_hop_enabled;
  ue_ul_cfg.ul_cfg.dmrs.cyclic_shift        = common->pusch_cnfg.ul_ref_sigs_pusch.cyclic_shift;
  ue_ul_cfg.ul_cfg.dmrs.delta_ss            = common->pusch_cnfg.ul_ref_sigs_pusch.group_assign_pusch;

  /* PUSCH Hopping configuration */
  bzero(&ue_ul_cfg.ul_cfg.hopping, sizeof(srslte_pusch_hopping_cfg_t));
  ue_ul_cfg.ul_cfg.hopping.n_sb = common->pusch_cnfg.pusch_cfg_basic.n_sb;
  ue_ul_cfg.ul_cfg.hopping.hop_mode =
      common->pusch_cnfg.pusch_cfg_basic.hop_mode.value ==
              pusch_cfg_common_s::pusch_cfg_basic_s_::hop_mode_e_::intra_and_inter_sub_frame
          ? ue_ul_cfg.ul_cfg.hopping.SRSLTE_PUSCH_HOP_MODE_INTRA_SF
          : ue_ul_cfg.ul_cfg.hopping.SRSLTE_PUSCH_HOP_MODE_INTER_SF;
  ue_ul_cfg.ul_cfg.hopping.hopping_offset = common->pusch_cnfg.pusch_cfg_basic.pusch_hop_offset;

  /* PUSCH UCI configuration */
  bzero(&ue_ul_cfg.ul_cfg.pusch.uci_offset, sizeof(srslte_uci_offset_cfg_t));
  ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_ack = dedicated->pusch_cfg_ded.beta_offset_ack_idx;
  ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_cqi = dedicated->pusch_cfg_ded.beta_offset_cqi_idx;
  ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_ri  = dedicated->pusch_cfg_ded.beta_offset_ri_idx;

  parse_pucch_config(phy_cfg);

  /* SRS Configuration */
  bzero(&ue_ul_cfg.ul_cfg.srs, sizeof(srslte_refsignal_srs_cfg_t));
  ue_ul_cfg.ul_cfg.srs.configured = dedicated->srs_ul_cfg_ded_present and
                                    dedicated->srs_ul_cfg_ded.type() == setup_e::setup and
                                    common->srs_ul_cnfg.type() == setup_e::setup;
  if (ue_ul_cfg.ul_cfg.srs.configured) {
    ue_ul_cfg.ul_cfg.srs.I_srs           = dedicated->srs_ul_cfg_ded.setup().srs_cfg_idx;
    ue_ul_cfg.ul_cfg.srs.B               = dedicated->srs_ul_cfg_ded.setup().srs_bw;
    ue_ul_cfg.ul_cfg.srs.b_hop           = dedicated->srs_ul_cfg_ded.setup().srs_hop_bw;
    ue_ul_cfg.ul_cfg.srs.n_rrc           = dedicated->srs_ul_cfg_ded.setup().freq_domain_position;
    ue_ul_cfg.ul_cfg.srs.k_tc            = dedicated->srs_ul_cfg_ded.setup().tx_comb;
    ue_ul_cfg.ul_cfg.srs.n_srs           = dedicated->srs_ul_cfg_ded.setup().cyclic_shift;
    ue_ul_cfg.ul_cfg.srs.simul_ack       = common->srs_ul_cnfg.setup().ack_nack_srs_simul_tx;
    ue_ul_cfg.ul_cfg.srs.bw_cfg          = common->srs_ul_cnfg.setup().srs_bw_cfg.to_number();
    ue_ul_cfg.ul_cfg.srs.subframe_config = common->srs_ul_cnfg.setup().srs_sf_cfg.to_number();
  }

  /* UL power control configuration */
  bzero(&ue_ul_cfg.ul_cfg.power_ctrl, sizeof(srslte_ue_ul_powerctrl_t));
  ue_ul_cfg.ul_cfg.power_ctrl.p0_nominal_pusch = common->ul_pwr_ctrl.p0_nominal_pusch;
  ue_ul_cfg.ul_cfg.power_ctrl.alpha            = common->ul_pwr_ctrl.alpha.to_number();
  ue_ul_cfg.ul_cfg.power_ctrl.p0_nominal_pucch = common->ul_pwr_ctrl.p0_nominal_pucch;
  ue_ul_cfg.ul_cfg.power_ctrl.delta_f_pucch[0] =
      common->ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format1.to_number();
  ue_ul_cfg.ul_cfg.power_ctrl.delta_f_pucch[1] =
      common->ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format1b.to_number();
  ue_ul_cfg.ul_cfg.power_ctrl.delta_f_pucch[2] =
      common->ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format2.to_number();
  ue_ul_cfg.ul_cfg.power_ctrl.delta_f_pucch[3] =
      common->ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format2a.to_number();
  ue_ul_cfg.ul_cfg.power_ctrl.delta_f_pucch[4] =
      common->ul_pwr_ctrl.delta_flist_pucch.delta_f_pucch_format2b.to_number();

  ue_ul_cfg.ul_cfg.power_ctrl.delta_preamble_msg3 = common->ul_pwr_ctrl.delta_preamb_msg3;

  ue_ul_cfg.ul_cfg.power_ctrl.p0_ue_pusch = dedicated->ul_pwr_ctrl_ded.p0_ue_pusch;
  ue_ul_cfg.ul_cfg.power_ctrl.delta_mcs_based =
      dedicated->ul_pwr_ctrl_ded.delta_mcs_enabled == ul_pwr_ctrl_ded_s::delta_mcs_enabled_e_::en0;
  ue_ul_cfg.ul_cfg.power_ctrl.acc_enabled  = dedicated->ul_pwr_ctrl_ded.accumulation_enabled;
  ue_ul_cfg.ul_cfg.power_ctrl.p0_ue_pucch  = dedicated->ul_pwr_ctrl_ded.p0_ue_pucch;
  ue_ul_cfg.ul_cfg.power_ctrl.p_srs_offset = dedicated->ul_pwr_ctrl_ded.p_srs_offset;

  /* CQI configuration */
  bzero(&ue_dl_cfg.cfg.cqi_report, sizeof(srslte_cqi_report_cfg_t));
  ue_dl_cfg.cfg.cqi_report.periodic_configured = dedicated->cqi_report_cfg.cqi_report_periodic_present and
                                                 dedicated->cqi_report_cfg.cqi_report_periodic.type() == setup_e::setup;
  if (ue_dl_cfg.cfg.cqi_report.periodic_configured) {
    ue_dl_cfg.cfg.cqi_report.pmi_idx = dedicated->cqi_report_cfg.cqi_report_periodic.setup().cqi_pmi_cfg_idx;
    ue_dl_cfg.cfg.cqi_report.format_is_subband =
        dedicated->cqi_report_cfg.cqi_report_periodic.setup().cqi_format_ind_periodic.type().value ==
        cqi_report_periodic_c::setup_s_::cqi_format_ind_periodic_c_::types::subband_cqi;
    if (ue_dl_cfg.cfg.cqi_report.format_is_subband) {
      ue_dl_cfg.cfg.cqi_report.subband_size =
          dedicated->cqi_report_cfg.cqi_report_periodic.setup().cqi_format_ind_periodic.subband_cqi().k;
    }
    if (dedicated->cqi_report_cfg.cqi_report_periodic.setup().ri_cfg_idx_present) {
      if (cell.nof_ports == 1) {
        log_h->warning("Received Rank Indication report configuration but only 1 antenna is available\n");
      }
      ue_dl_cfg.cfg.cqi_report.ri_idx         = dedicated->cqi_report_cfg.cqi_report_periodic.setup().ri_cfg_idx;
      ue_dl_cfg.cfg.cqi_report.ri_idx_present = true;
    } else {
      ue_dl_cfg.cfg.cqi_report.ri_idx_present = false;
    }
  }

  if (dedicated->cqi_report_cfg.cqi_report_mode_aperiodic_present) {
    ue_dl_cfg.cfg.cqi_report.aperiodic_configured = true;
    ue_dl_cfg.cfg.cqi_report.aperiodic_mode       = aperiodic_mode(dedicated->cqi_report_cfg.cqi_report_mode_aperiodic);
  }
  if (dedicated->cqi_report_cfg_pcell_v1250_present) {
    auto cqi_report_cfg_pcell_v1250 = dedicated->cqi_report_cfg_pcell_v1250.get();
    if (cqi_report_cfg_pcell_v1250->alt_cqi_table_r12_present) {
      ue_dl_cfg.pdsch_use_tbs_index_alt = true;
    }
  } else {
    ue_dl_cfg.pdsch_use_tbs_index_alt = false;
  }

  if (pregen_enabled) {
    Info("Pre-generating UL signals...\n");
    srslte_ue_ul_pregen_signals(&ue_ul, &ue_ul_cfg);
    Info("Done pre-generating signals worker...\n");
  }
}

void cc_worker::set_scell_config(asn1::rrc::scell_to_add_mod_r10_s* phy_cfg)
{
  if (phy_cfg->rr_cfg_common_scell_r10_present) {
    rr_cfg_common_scell_r10_s* rr_cfg_common_scell_r10 = &phy_cfg->rr_cfg_common_scell_r10;

    if (rr_cfg_common_scell_r10->ul_cfg_r10_present) {
      typedef rr_cfg_common_scell_r10_s::ul_cfg_r10_s_ ul_cfg_r10_t;
      ul_cfg_r10_t*                                    ul_cfg_r10 = &rr_cfg_common_scell_r10->ul_cfg_r10;

      // Parse Power control
      ul_pwr_ctrl_common_scell_r10_s* ul_pwr_ctrl_common_scell_r10 = &ul_cfg_r10->ul_pwr_ctrl_common_scell_r10;
      bzero(&ue_ul_cfg.ul_cfg.power_ctrl, sizeof(srslte_ue_ul_powerctrl_t));
      ue_ul_cfg.ul_cfg.power_ctrl.p0_nominal_pusch = ul_pwr_ctrl_common_scell_r10->p0_nominal_pusch_r10;
      ue_ul_cfg.ul_cfg.power_ctrl.alpha            = ul_pwr_ctrl_common_scell_r10->alpha_r10.to_number();

      // Parse SRS
      typedef srs_ul_cfg_common_c::setup_s_ srs_ul_cfg_common_t;
      if (ul_cfg_r10->srs_ul_cfg_common_r10.type() == setup_e::setup) {
        srs_ul_cfg_common_t* srs_ul_cfg_common = &ul_cfg_r10->srs_ul_cfg_common_r10.setup();
        ue_ul_cfg.ul_cfg.srs.configured        = true;
        ue_ul_cfg.ul_cfg.srs.simul_ack         = srs_ul_cfg_common->ack_nack_srs_simul_tx;
        ue_ul_cfg.ul_cfg.srs.bw_cfg            = srs_ul_cfg_common->srs_bw_cfg.to_number();
        ue_ul_cfg.ul_cfg.srs.subframe_config   = srs_ul_cfg_common->srs_sf_cfg.to_number();
      } else {
        ue_ul_cfg.ul_cfg.srs.configured = false;
      }

      // Parse PUSCH
      pusch_cfg_common_s* pusch_cfg_common = &ul_cfg_r10->pusch_cfg_common_r10;
      bzero(&ue_ul_cfg.ul_cfg.hopping, sizeof(srslte_pusch_hopping_cfg_t));
      ue_ul_cfg.ul_cfg.hopping.n_sb = pusch_cfg_common->pusch_cfg_basic.n_sb;
      ue_ul_cfg.ul_cfg.hopping.hop_mode =
          pusch_cfg_common->pusch_cfg_basic.hop_mode.value ==
                  pusch_cfg_common_s::pusch_cfg_basic_s_::hop_mode_e_::intra_and_inter_sub_frame
              ? ue_ul_cfg.ul_cfg.hopping.SRSLTE_PUSCH_HOP_MODE_INTRA_SF
              : ue_ul_cfg.ul_cfg.hopping.SRSLTE_PUSCH_HOP_MODE_INTER_SF;
      ue_ul_cfg.ul_cfg.hopping.hopping_offset = pusch_cfg_common->pusch_cfg_basic.pusch_hop_offset;
      ue_ul_cfg.ul_cfg.pusch.enable_64qam     = pusch_cfg_common->pusch_cfg_basic.enable64_qam;
    }
  }

  if (phy_cfg->rr_cfg_ded_scell_r10_present) {
    rr_cfg_ded_scell_r10_s* rr_cfg_ded_scell_r10 = &phy_cfg->rr_cfg_ded_scell_r10;
    if (rr_cfg_ded_scell_r10->phys_cfg_ded_scell_r10_present) {
      phys_cfg_ded_scell_r10_s* phys_cfg_ded_scell_r10 = &rr_cfg_ded_scell_r10->phys_cfg_ded_scell_r10;

      // Parse nonUL Configuration
      if (phys_cfg_ded_scell_r10->non_ul_cfg_r10_present) {

        typedef phys_cfg_ded_scell_r10_s::non_ul_cfg_r10_s_ non_ul_cfg_t;
        non_ul_cfg_t*                                       non_ul_cfg = &phys_cfg_ded_scell_r10->non_ul_cfg_r10;

        // Parse Transmission mode
        if (non_ul_cfg->ant_info_r10_present) {
          ant_info_ded_r10_s::tx_mode_r10_e_::options tx_mode = non_ul_cfg->ant_info_r10.tx_mode_r10.value;
          if ((srslte_tm_t)tx_mode < SRSLTE_TMINV) {
            ue_dl_cfg.cfg.tm = (srslte_tm_t)tx_mode;
          } else {
            fprintf(stderr,
                    "Transmission mode (R10) %s is not supported\n",
                    non_ul_cfg->ant_info_r10.tx_mode_r10.to_string().c_str());
          }
        }

        // Parse Cross carrier scheduling
        if (non_ul_cfg->cross_carrier_sched_cfg_r10_present) {
          typedef cross_carrier_sched_cfg_r10_s::sched_cell_info_r10_c_ sched_info_t;

          typedef sched_info_t::types cross_carrier_type_e;
          sched_info_t*               sched_info = &non_ul_cfg->cross_carrier_sched_cfg_r10.sched_cell_info_r10;

          cross_carrier_type_e cross_carrier_type = sched_info->type();
          if (cross_carrier_type == cross_carrier_type_e::own_r10) {
            ue_dl_cfg.dci_cfg.cif_enabled = sched_info->own_r10().cif_presence_r10;
          } else {
            ue_dl_cfg.dci_cfg.cif_enabled = false; // This CC does not have Carrier Indicator Field
            // ue_dl_cfg.blablabla = sched_info->other_r10().pdsch_start_r10;
            // ue_dl_cfg.blablabla = sched_info->other_r10().sched_cell_id_r10;
          }
        }

        // Parse pdsch config dedicated
        if (non_ul_cfg->pdsch_cfg_ded_r10_present) {
          ue_dl_cfg.cfg.pdsch.p_b         = phy_cfg->rr_cfg_common_scell_r10.non_ul_cfg_r10.pdsch_cfg_common_r10.p_b;
          ue_dl_cfg.cfg.pdsch.p_a         = non_ul_cfg->pdsch_cfg_ded_r10.p_a.to_number();
          ue_dl_cfg.cfg.pdsch.power_scale = true;
        }
      }

      // Parse UL Configuration
      if (phys_cfg_ded_scell_r10->ul_cfg_r10_present) {
        typedef phys_cfg_ded_scell_r10_s::ul_cfg_r10_s_ ul_cfg_t;
        ul_cfg_t*                                       ul_cfg = &phys_cfg_ded_scell_r10->ul_cfg_r10;

        // Parse CQI param
        if (ul_cfg->cqi_report_cfg_scell_r10_present) {
          cqi_report_cfg_scell_r10_s* cqi_report_cfg = &ul_cfg->cqi_report_cfg_scell_r10;

          // Aperiodic report
          if (cqi_report_cfg->cqi_report_mode_aperiodic_r10_present) {
            ue_dl_cfg.cfg.cqi_report.aperiodic_configured = true;
            ue_dl_cfg.cfg.cqi_report.aperiodic_mode = aperiodic_mode(cqi_report_cfg->cqi_report_mode_aperiodic_r10);
          }

          // Periodic report
          if (cqi_report_cfg->cqi_report_periodic_scell_r10_present) {
            if (cqi_report_cfg->cqi_report_periodic_scell_r10.type() == setup_e::setup) {
              typedef cqi_report_periodic_r10_c::setup_s_ cqi_cfg_t;
              cqi_cfg_t cqi_cfg                            = cqi_report_cfg->cqi_report_periodic_scell_r10.setup();
              ue_dl_cfg.cfg.cqi_report.periodic_configured = true;
              ue_dl_cfg.cfg.cqi_report.pmi_idx             = cqi_cfg.cqi_pmi_cfg_idx;
              ue_dl_cfg.cfg.cqi_report.format_is_subband =
                  cqi_cfg.cqi_format_ind_periodic_r10.type().value ==
                  cqi_cfg_t::cqi_format_ind_periodic_r10_c_::types::subband_cqi_r10;
              if (ue_dl_cfg.cfg.cqi_report.format_is_subband) {
                ue_dl_cfg.cfg.cqi_report.subband_size = cqi_cfg.cqi_format_ind_periodic_r10.subband_cqi_r10().k;
              }
              if (cqi_cfg.ri_cfg_idx_present) {
                ue_dl_cfg.cfg.cqi_report.ri_idx         = cqi_cfg.ri_cfg_idx;
                ue_dl_cfg.cfg.cqi_report.ri_idx_present = true;
              } else {
                ue_dl_cfg.cfg.cqi_report.ri_idx_present = false;
              }
            } else {
              // Release, disable periodic reporting
              ue_dl_cfg.cfg.cqi_report.periodic_configured = false;
            }
          }
        }

        if (ul_cfg->srs_ul_cfg_ded_r10_present) {
          // Sounding reference signals
          if (ul_cfg->srs_ul_cfg_ded_r10.type() == setup_e::setup) {
            srs_ul_cfg_ded_c::setup_s_* srs_ul_cfg_ded_r10 = &ul_cfg->srs_ul_cfg_ded_r10.setup();
            ue_ul_cfg.ul_cfg.srs.I_srs                     = srs_ul_cfg_ded_r10->srs_cfg_idx;
            ue_ul_cfg.ul_cfg.srs.B                         = srs_ul_cfg_ded_r10->srs_bw;
            ue_ul_cfg.ul_cfg.srs.b_hop                     = srs_ul_cfg_ded_r10->srs_hop_bw;
            ue_ul_cfg.ul_cfg.srs.n_rrc                     = srs_ul_cfg_ded_r10->freq_domain_position;
            ue_ul_cfg.ul_cfg.srs.k_tc                      = srs_ul_cfg_ded_r10->tx_comb;
            ue_ul_cfg.ul_cfg.srs.n_srs                     = srs_ul_cfg_ded_r10->cyclic_shift;
          } else {
            ue_ul_cfg.ul_cfg.srs.configured = false;
          }
        }
      }

      if (phys_cfg_ded_scell_r10->cqi_report_cfg_scell_v1250_present) {
        auto cqi_report_cfg_scell = phys_cfg_ded_scell_r10->cqi_report_cfg_scell_v1250.get();

        // Enable/disable PDSCH 256QAM
        ue_dl_cfg.pdsch_use_tbs_index_alt = cqi_report_cfg_scell->alt_cqi_table_r12_present;
      } else {
        // Assume there is no PDSCH 256QAM
        ue_dl_cfg.pdsch_use_tbs_index_alt = false;
      }
    }
  }
}

int cc_worker::read_ce_abs(float* ce_abs, uint32_t tx_antenna, uint32_t rx_antenna)
{
  uint32_t i  = 0;
  int      sz = srslte_symbol_sz(cell.nof_prb);
  bzero(ce_abs, sizeof(float) * sz);
  int g = (sz - 12 * cell.nof_prb) / 2;
  for (i = 0; i < 12 * cell.nof_prb; i++) {
    ce_abs[g + i] = 20 * log10f(std::abs(std::complex<float>(ue_sl.ce_plot[i])));
    if (std::isinf(ce_abs[g + i])) {
      ce_abs[g + i] = -80;
    }
  }
  return sz;
}

cf_t* cc_worker::read_td_samples(uint32_t* n_samples)
{
  if (n_samples) {
    *n_samples = ue_sl.fft.sf_sz;
  }
  return ue_sl.td_plot;
}

int cc_worker::read_pdsch_d(cf_t* pdsch_d)
{
  memcpy(pdsch_d, ue_dl.pdsch.d[0], ue_dl_cfg.cfg.pdsch.grant.nof_re * sizeof(cf_t));
  return ue_dl_cfg.cfg.pdsch.grant.nof_re;
}

int cc_worker::read_pssch_d(cf_t* pssch_d)
{
  uint32_t nof_re = pending_sl_grant[0].sl_dci.frl_L_subCH > 0 ? 12*9*(pending_sl_grant[0].sl_dci.frl_L_subCH * phy->ue_repo.rp.sizeSubchannel_r14 - 2) : 0;
  memcpy(pssch_d, ue_sl.pssch.d[0], nof_re * sizeof(cf_t));
  
  return nof_re;
}
} // namespace srsue
