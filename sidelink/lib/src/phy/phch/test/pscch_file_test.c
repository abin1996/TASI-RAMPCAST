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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "srslte/srslte.h"

char *input_file_name = NULL;

srslte_cell_t cell = {
  6,            // cell.cell.cell.nof_prb
  1,            // cell.cell.nof_ports
  0,            // cell.id
  SRSLTE_CP_NORM,       // cyclic prefix
  SRSLTE_PHICH_R_1,          // PHICH resources      
  SRSLTE_PHICH_NORM    // PHICH length
};


SL_CommResourcePoolV2X_r14 respool = {
  0, //uint8_t sl_OffsetIndicator_r14;

  // subframe bitmap
  {0,0,1},//uint8_t sl_Subframe_r14[100];
  20, //uint8_t sl_Subframe_r14_len;

  true,//bool adjacencyPSCCH_PSSCH_r14;

  // indicated the number of PRBs of each subchannel in the corresponding resource pool
  5,//uint8_t sizeSubchannel_r14;

  // indicates the number of subchannels in the corresponding resource pool
  5,//uint8_t numSubchannel_r14;

  // indicates the lowest RB index of the subchannel with the lowest index
  0,//uint8_t startRB_Subchannel_r14;

  // indicates the lowest RB index of PSCCH pool, this field is absent whe a pool is (pre)configured
  // such tha a UE always transmits SC and data in adjacent RBs in the same subframe
  0,//uint8_t startRB_PSCCH_Pool_r14;
};

uint32_t cfi = 2;
int flen;
uint16_t rnti = SRSLTE_SIRNTI;
int max_frames = 10;

srslte_dci_format_t dci_format = SRSLTE_DCI_FORMAT1A;
srslte_filesource_t fsrc;
srslte_pscch_t pscch;
srslte_pssch_t pssch;
cf_t *input_buffer, *fft_buffer, *ce[SRSLTE_MAX_PORTS];
// srslte_regs_t regs;
srslte_ofdm_t fft;

srslte_chest_sl_t chest;

srslte_repo_t repo;


void usage(char *prog) {
  printf("Usage: %s [vcfoe] -i input_file\n", prog);
  printf("\t-c cell.id [Default %d]\n", cell.id);
  printf("\t-f cfi [Default %d]\n", cfi);
  printf("\t-o DCI Format [Default %s]\n", srslte_dci_format_string(dci_format));
  printf("\t-r rnti [Default SI-RNTI]\n");
  printf("\t-p cell.nof_ports [Default %d]\n", cell.nof_ports);
  printf("\t-n cell.nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-m max_frames [Default %d]\n", max_frames);
  printf("\t-e Set extended prefix [Default Normal]\n");
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "irvofcenmp")) != -1) {
    switch(opt) {
    case 'i':
      input_file_name = argv[optind];
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'r':
      rnti = strtoul(argv[optind], NULL, 0);
      break;
    case 'm':
      max_frames = strtoul(argv[optind], NULL, 0);
      break;
    case 'f':
      cfi = atoi(argv[optind]);
      break;
    case 'n':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'p':
      cell.nof_ports = atoi(argv[optind]);
      break;
    case 'o':
      dci_format = srslte_dci_format_from_string(argv[optind]);
      if (dci_format == SRSLTE_DCI_NOF_FORMATS) {
        fprintf(stderr, "Error unsupported format %s\n", argv[optind]);
        exit(-1);
      }
      break;
    case 'v':
      srslte_verbose++;
      break;
    case 'e':
      cell.cp = SRSLTE_CP_EXT;
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
  if (!input_file_name) {
    usage(argv[0]);
    exit(-1);
  }
}

int base_init() {
  int i;

  if (srslte_filesource_init(&fsrc, input_file_name, SRSLTE_COMPLEX_FLOAT_BIN)) {
    fprintf(stderr, "Error opening file %s\n", input_file_name);
    exit(-1);
  }

  flen = 2 * (SRSLTE_SLOT_LEN(srslte_symbol_sz_power2(cell.nof_prb)));

  input_buffer = malloc(flen * sizeof(cf_t));
  if (!input_buffer) {
    perror("malloc");
    exit(-1);
  }

  fft_buffer = malloc(SRSLTE_SF_LEN_RE(cell.nof_prb, cell.cp) * sizeof(cf_t));
  if (!fft_buffer) {
    perror("malloc");
    return -1;
  }

  for (i=0;i<SRSLTE_MAX_PORTS;i++) {
    ce[i] = malloc(SRSLTE_SF_LEN_RE(cell.nof_prb, cell.cp) * sizeof(cf_t));
    if (!ce[i]) {
      perror("malloc");
      return -1;
    }
  }

  printf("srslte_chest_sl_init(est, nof_prfb=%d)\n", cell.nof_prb);
  if (srslte_chest_sl_init(&chest, cell.nof_prb)) {
    fprintf(stderr, "Error initializing equalizer\n");
    exit(-1);
  }
  printf("srslte_chest_sl_set_cell(est, cell_id=%d rb: %d)\n", cell.id, cell.nof_prb);
  if (srslte_chest_sl_set_cell(&chest, cell)) {
        fprintf(stderr, "Error initializing equalizer\n");
        exit(-1);
  }

  // if (srslte_chest_dl_init(&chest, cell.nof_prb)) {
  //   fprintf(stderr, "Error initializing equalizer\n");
  //   return -1;
  // }
  // if (srslte_chest_dl_set_cell(&chest, cell)) {
  //   fprintf(stderr, "Error initializing equalizer\n");
  //   return -1;
  // }

  // srslte_symbol_sz(max_prb)
  if (srslte_ofdm_init_(&fft, cell.cp, input_buffer, fft_buffer, srslte_symbol_sz_power2(cell.nof_prb), cell.nof_prb, SRSLTE_DFT_FORWARD)) {
    fprintf(stderr, "Error initializing FFT\n");
    return -1;
  }

  srslte_ofdm_set_normalize(&fft, false);
  srslte_ofdm_set_freq_shift(&fft, -0.5);

  // if (srslte_regs_init(&regs, cell)) {
  //   fprintf(stderr, "Error initiating regs\n");
  //   return -1;
  // }

  if (srslte_pscch_init_ue(&pscch, cell.nof_prb, 1)) {
    fprintf(stderr, "Error creating PSCCH object\n");
    exit(-1);
  }
  if (srslte_pscch_set_cell(&pscch, cell)) {
    fprintf(stderr, "Error creating PSCCH object\n");
    exit(-1);
  }

  if (srslte_repo_init(&repo, cell, respool)) {
    fprintf(stderr, "Error creating Ressource Pool object\n");
    exit(-1);
  }

  if (srslte_pssch_init_ue(&pssch, cell.nof_prb, cell.nof_ports)) {
    fprintf(stderr, "Error creating PSSCH object\n");
    exit(-1);

  }
  if (srslte_pssch_set_cell(&pssch, cell)) { 
    fprintf(stderr, "Error creating PSSCH object\n");
    exit(-1);
  }

  DEBUG("Memory init OK\n");
  return 0;
}

void base_free() {
  int i;

  srslte_filesource_free(&fsrc);

  free(input_buffer);
  free(fft_buffer);

  srslte_filesource_free(&fsrc);
  for (i=0;i<SRSLTE_MAX_PORTS;i++) {
    free(ce[i]);
  }
  srslte_chest_sl_free(&chest);
  srslte_ofdm_rx_free(&fft);

  srslte_pscch_free(&pscch);
  // srslte_regs_free(&regs);
  srslte_pssch_free(&pssch);
}


int main(int argc, char **argv) {
  int i;
  int frame_cnt;
  int ret;
  srslte_dci_location_t locations[MAX_CANDIDATES];
  uint32_t nof_locations;
  srslte_dci_msg_t dci_msg;
  srslte_ra_sl_sci_t sci;

  if (argc < 3) {
    usage(argv[0]);
    exit(-1);
  }

  parse_args(argc,argv);

  srslte_softbuffer_rx_t *softbuffers_rx[SRSLTE_MAX_CODEWORDS];
  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    softbuffers_rx[i] = calloc(sizeof(srslte_softbuffer_rx_t), 1);
    if (!softbuffers_rx[i]) {
      fprintf(stderr, "Error allocating RX soft buffer\n");
      exit(-1);
    }

    if (srslte_softbuffer_rx_init(softbuffers_rx[i], cell.nof_prb)) {
      fprintf(stderr, "Error initiating RX soft buffer\n");
      exit(-1);
    }
  }


  if (base_init()) {
    fprintf(stderr, "Error initializing memory\n");
    exit(-1);
  }

    if(repo.rp.numSubchannel_r14 * repo.rp.sizeSubchannel_r14 > cell.nof_prb) {
    printf("Ressourcepool configuration does not match cell size.\n");
    return -1;
  }

  ret = -1;
  frame_cnt = 0;
  do {
    srslte_filesource_read(&fsrc, input_buffer, flen);

    INFO("Reading %d samples sub-frame %d\n", flen, frame_cnt);

    srslte_ofdm_rx_sf(&fft);

    struct timeval t[3];

    bool acks[SRSLTE_MAX_CODEWORDS];

    uint8_t mdata[32];
    uint16_t crc_rem = 0xdead;

    // generic parameter for the test
    pssch.n_PSSCH_ssf = srslte_repo_get_n_PSSCH_ssf(&repo, 24);

    for(int rbp=0; rbp<repo.rp.numSubchannel_r14; rbp++) {

      uint32_t prb_offset = repo.rp.startRB_Subchannel_r14 + rbp*repo.rp.sizeSubchannel_r14;

      srslte_chest_sl_estimate_pscch(&chest, fft_buffer, ce[0], SRSLTE_SL_MODE_4, prb_offset);

      if (srslte_pscch_extract_llr(&pscch, fft_buffer, 
                          ce, 0 /*srslte_chest_dl_get_noise_estimate(&chest)*/, 
                          frame_cnt %10, prb_offset)) {
        fprintf(stderr, "Error extracting LLRs\n");
        return -1;
      }

      // @todo: rename
      gettimeofday(&t[1], NULL);
      if(SRSLTE_SUCCESS != srslte_pscch_dci_decode(&pscch, pscch.llr, mdata, pscch.max_bits, 32, &crc_rem)) {
        continue;
      }
      gettimeofday(&t[2], NULL);

      get_time_interval(t);
      printf("DECODED PSCCH in %d us and N_X_ID: %x \n", t[0].tv_usec, crc_rem);

      srslte_repo_sci_decode(&repo, mdata, &sci);

      // set parameters for pssch
      pssch.n_X_ID = crc_rem;

      // generate pssch dmrs
      srslte_refsignal_sl_dmrs_pssch_gen_feron(&chest.dmrs_signal,
                                                sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2,
                                                pssch.n_PSSCH_ssf,
                                                pssch.n_X_ID, // CRC checksum
                                                chest.pilot_known_signal);


      srslte_chest_sl_estimate_pssch(&chest, fft_buffer, ce[0], SRSLTE_SL_MODE_4,
                                      prb_offset + 2,
                                      sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2);

      cf_t *_sf_symbols[SRSLTE_MAX_PORTS]; 
      cf_t *_ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];

      _sf_symbols[0] = fft_buffer; 
      for (int i=0;i<pscch.cell.nof_ports;i++) {
        _ce[i][0] = ce[i]; 
      }

      srslte_softbuffer_rx_reset_tbs(softbuffers_rx[0], (uint32_t) sci.mcs.tbs);

      // allocate buffer to store decoded data, tbs size in bytes plus crc 
      uint8_t *data_rx[SRSLTE_MAX_CODEWORDS];
      data_rx[0] = srslte_vec_malloc(sizeof(uint8_t) * sci.mcs.tbs/8 + 3);
      memset(data_rx[0], 0xFF, sizeof(uint8_t) * sci.mcs.tbs/8 + 3);


      int decode_ret = srslte_pssch_decode_simple(&pssch,
                                                  &sci,
                                                  NULL,//&pdsch_cfg,//srslte_pssch_cfg_t *cfg,
                                                  softbuffers_rx,//srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                                                  _sf_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                                                  _ce,//_ce,//cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                                                  0,//float noise_estimate,
                                                  prb_offset + 2,
                                                  sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2,
                                                  data_rx);//uint8_t *data[SRSLTE_MAX_CODEWORDS],




      if(SRSLTE_SUCCESS == decode_ret) {
        ret = 0;

        srslte_vec_fprint_byte(stdout, data_rx[0], sci.mcs.tbs/8 + 3);
      }

    }

    frame_cnt++;
  } while (frame_cnt <= max_frames);

  printf("Test done. Result: %s\n", 0==ret?"PASS":"FAIL");
  base_free();
  srslte_dft_exit();
  exit(ret);
}
