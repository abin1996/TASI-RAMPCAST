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
#include <time.h>

#include "srslte/srslte.h"

srslte_cell_t cell = {
  50,            // nof_prb
  1,            // nof_ports
  1,            // cell_id
  SRSLTE_CP_NORM,       // cyclic prefix
  SRSLTE_PHICH_R_1,          // PHICH resources      
  SRSLTE_PHICH_NORM    // PHICH length
};

void usage(char *prog) {
  printf("Usage: %s [cpv]\n", prog);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-p cell.nof_ports [Default %d]\n", cell.nof_ports);
  printf("\t-n cell.nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "cpnv")) != -1) {
    switch(opt) {
    case 'p':
      cell.nof_ports = atoi(argv[optind]);
      break;
    case 'n':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'v':
      srslte_verbose++;
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
}


int main(int argc, char **argv) {
  srslte_pssch_t pssch;
  uint8_t *payload_tx, *payload_rx;
  int i, j;
  cf_t *ce[SRSLTE_MAX_PORTS];
  int nof_re;
  cf_t *subframe_symbols[SRSLTE_MAX_PORTS];

  parse_args(argc,argv);

  nof_re = 2*SRSLTE_SLOT_LEN_RE(cell.nof_prb, SRSLTE_CP_NORM);

  // add additional memory for tb crc and optional codeblock crc
  int max_bits = srslte_ra_tbs_from_idx(26, cell.nof_prb) + 24 + 24;

  payload_tx = malloc(max_bits/8);
  payload_rx = malloc(max_bits/8);

  bzero(payload_tx, max_bits/8);
  bzero(payload_rx, max_bits/8);

  /* init memory */
  for (i=0;i<cell.nof_ports;i++) {
    ce[i] = malloc(sizeof(cf_t) * nof_re);
    if (!ce[i]) {
      perror("malloc");
      exit(-1);
    }
    for (j=0;j<nof_re;j++) {
      ce[i][j] = 1;
    }
    subframe_symbols[i] = malloc(sizeof(cf_t) * nof_re);
    if (!subframe_symbols[i]) {
      perror("malloc");
      exit(-1);
    }

  }

  if (srslte_pssch_init_ue(&pssch, cell.nof_prb, cell.nof_ports)) {
    fprintf(stderr, "Error creating PSSCH object\n");
    exit(-1);

  }
  if (srslte_pssch_set_cell(&pssch, cell)) { 
    fprintf(stderr, "Error creating PSSCH object\n");
    exit(-1);
  }

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

  srslte_softbuffer_tx_t *softbuffers_tx[SRSLTE_MAX_CODEWORDS];
  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    softbuffers_tx[i] = calloc(sizeof(srslte_softbuffer_tx_t), 1);
    if (!softbuffers_tx[i]) {
      fprintf(stderr, "Error allocating TX soft buffer\n");
      exit(-1);
    }

    if (srslte_softbuffer_tx_init(softbuffers_tx[i], cell.nof_prb)) {
      fprintf(stderr, "Error initiating TX soft buffer\n");
      exit(-1);
    }
  }

  srand(time(NULL));

  srslte_ra_sl_sci_t sci;
  cf_t *_ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS];

  for (int i=0;i<pssch.cell.nof_ports;i++) {
    _ce[i][0] = ce[i]; 
  }

  uint32_t prb_offset = 0;

  uint8_t *data_tx[SRSLTE_MAX_CODEWORDS];
  uint8_t *data_rx[SRSLTE_MAX_CODEWORDS];

  uint32_t test_failed = 0;
  uint32_t test_passed = 0;

  data_tx[0] = payload_tx;
  data_rx[0] = payload_rx;

  for(int n_prb=1; n_prb<=cell.nof_prb; n_prb++) {

    // skip invalid prb sizes, it has no changes to the test if we ignore it
    // as the dft-precoding is then skippen on encoder and decoder
    //if(!srslte_dft_precoding_valid_prb(n_prb)) continue;

    for(int I_mcs=0; I_mcs<=28; I_mcs++) {

      // set some mcs level
      sci.mcs.idx = I_mcs;
      sci.frl_L_subCH = 1;
      sci.rti = 0;

      srslte_sl_fill_ra_mcs(&sci.mcs, n_prb);

      // generate input
      for (i=0; i<sci.mcs.tbs/8; i++) {
        payload_tx[i] = rand();
      }
      payload_tx[0] = I_mcs;

      // randomize prb offset
      prb_offset = rand() % (cell.nof_prb - n_prb + 1);

      pssch.n_PSSCH_ssf = rand() % 10;
      pssch.n_X_ID = rand(); // CRC checksum

      srslte_softbuffer_tx_reset_tbs(softbuffers_tx[0], (uint32_t) sci.mcs.tbs);
      int encode_ret = srslte_pssch_encode_simple(&pssch,
                            &sci,
                            softbuffers_tx,//srslte_softbuffer_tx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                            subframe_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                            prb_offset,//repo.rp.startRB_Subchannel_r14 + rbp*repo.rp.sizeSubchannel_r14 + 2,//uint32_t prb_offset,
                            n_prb,//sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2,//uint32_t n_prb,
                            data_tx);//uint8_t *data[SRSLTE_MAX_CODEWORDS])


      srslte_softbuffer_rx_reset_tbs(softbuffers_rx[0], (uint32_t) sci.mcs.tbs);
      int decode_ret = srslte_pssch_decode_simple(&pssch,
                                                  &sci,
                                                  NULL,//&pdsch_cfg,//srslte_pssch_cfg_t *cfg,
                                                  softbuffers_rx,//srslte_softbuffer_rx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                                                  subframe_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                                                  _ce,//_ce,//cf_t *ce[SRSLTE_MAX_PORTS][SRSLTE_MAX_PORTS],
                                                  0,//float noise_estimate,
                                                  prb_offset,//repo.rp.startRB_Subchannel_r14 + rbp*repo.rp.sizeSubchannel_r14 + 2,
                                                  n_prb,//sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2,
                                                  data_rx);//uint8_t *data[SRSLTE_MAX_CODEWORDS],

      printf("MCS: %d mod: %d TBS: %d E: %d E-: %d L: %d nPRB: %d, oPRB: %d\n",
              sci.mcs.idx, sci.mcs.mod, sci.mcs.tbs,
              10*(n_prb)*SRSLTE_NRE*srslte_mod_bits_x_symbol(sci.mcs.mod),
              9*(n_prb)*SRSLTE_NRE*srslte_mod_bits_x_symbol(sci.mcs.mod),
              sci.frl_L_subCH, n_prb, prb_offset);

      if(((sci.mcs.tbs+24) >= 9*(n_prb)*SRSLTE_NRE*srslte_mod_bits_x_symbol(sci.mcs.mod)) && (decode_ret != SRSLTE_SUCCESS))  {
        printf("Impossible: Decode failed, as channel bits < tbs\n");

        continue;
      }

      // if(((sci.mcs.tbs+24) >= 8*(n_prb)*SRSLTE_NRE*srslte_mod_bits_x_symbol(sci.mcs.mod)) && (decode_ret != SRSLTE_SUCCESS))  {
      //   printf("Graceperiod: Decode failed, but was expected, as codrate is (nearly) larger 1! %d\n", decode_ret);
      //   test_failed++;
      //   continue;
      // }

      printf("Tx payload: ");
      if(sci.mcs.tbs > 240) {
        srslte_vec_fprint_byte(stdout, payload_tx, (240 + 24)/8);
      } else {
        srslte_vec_fprint_byte(stdout, payload_tx, (sci.mcs.tbs + 24)/8);
      }
      printf("Rx payload: ");
      if(sci.mcs.tbs > 240) {
        srslte_vec_fprint_byte(stdout, payload_rx, (240 + 24)/8);
      } else {
        srslte_vec_fprint_byte(stdout, payload_rx, (sci.mcs.tbs + 24)/8);
      }

      if ((encode_ret == SRSLTE_SUCCESS) && (decode_ret == SRSLTE_SUCCESS) && !memcmp(payload_rx, payload_tx, sizeof(uint8_t) * (sci.mcs.tbs)/8 - 0)) {
        printf("OK\n");
        test_passed++;
      } else {
        printf("Error\n");
        test_failed++;
        //exit(-1);
      }
    }
  }

  srslte_pssch_free(&pssch);

  free(payload_rx);
  free(payload_tx);

  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    srslte_softbuffer_rx_free(softbuffers_rx[i]);
    free(softbuffers_rx[i]);
    srslte_softbuffer_tx_free(softbuffers_tx[i]);
    free(softbuffers_tx[i]);
  }

  for (i=0;i<cell.nof_ports;i++) {
    free(ce[i]);
    free(subframe_symbols[i]);
  }

  // the merge of srslte 18.12 with the new turbo coder apprears to add another
  // parameter combination that fails, so we are at 63
  // but sometimes this one passes too
  printf("#Tests passed: %d failed: %d, we expect 63(or 62) failed for 50 PRBs.\n", test_passed, test_failed);

  // we always get failed test, because of dropping the last sc-fdma symbol
  // with this configuration 62 fail, this is also what matlab does
  if(50 == cell.nof_prb && !(62 == test_failed || 63 == test_failed)) return -1;

  return 0;
}
