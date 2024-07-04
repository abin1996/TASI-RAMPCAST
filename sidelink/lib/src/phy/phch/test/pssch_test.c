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
  sci.frl_n_subCH = 0;
  sci.frl_L_subCH = 1;
  sci.rti = 0;

  for(int n_prb=1; n_prb<=cell.nof_prb; n_prb++) {

    for(int I_mcs=0; I_mcs<=28; I_mcs++) {

      // set some mcs level
      sci.mcs.idx = I_mcs;

      srslte_sl_fill_ra_mcs(&sci.mcs, n_prb);

      uint32_t E = 10*(n_prb)*SRSLTE_NRE*srslte_mod_bits_x_symbol(sci.mcs.mod);

      printf("MCS: %d mod: %d TBS: %d E: %d L: %d nPRB: %d\n", sci.mcs.idx, sci.mcs.mod, sci.mcs.tbs, E, sci.frl_L_subCH, n_prb);
      
      for (i=0; i<sci.mcs.tbs/8; i++) {
        payload_tx[i] = rand();
      }

      srslte_softbuffer_tx_reset_tbs(softbuffers_tx[0], (uint32_t) sci.mcs.tbs);

      int ret_enc = srslte_slsch_encode(&pssch.dl_sch,
                            &sci,//srslte_ra_sl_sci_t *sci,
                            softbuffers_tx[0],//srslte_softbuffer_tx_t *softbuffer,
                            payload_tx,//uint8_t *data,
                            pssch.e[0],// uint8_t *e_bits,
                            E);//uint32_t E);

      // convert formats from hard-bits to soft-bits
      for(i=0; i<E/8; i++) {
        uint8_t byte = ((int8_t *)pssch.e[0])[i];
        for(j=0; j<8; j++) {
          ((int16_t *)pssch.h[0])[i*8 + j] = ((byte >> (7-j))&0x1) == 0 ? -200 : 200;
        }
      }

      srslte_softbuffer_rx_reset_tbs(softbuffers_rx[0], (uint32_t) sci.mcs.tbs);
      // rate-dematching and turbo-decoding
      int ret_dec = srslte_slsch_decode(&pssch.dl_sch, &sci, softbuffers_rx[0], pssch.h[0], E, payload_rx, 0);

      if((sci.mcs.tbs + 24 > E) && (ret_dec != SRSLTE_SUCCESS))  {
        printf("Decode failed, but was expected, as coderate is greater 1!\n");
        continue;
      }

      // grace period, sometimes we fail when we get near to coderate 1
      // @todo: needs to be investigated, currently this only happens for MCS: 22 and some prb constellations
      if((sci.mcs.tbs + 24 > E*0.9) && (ret_dec != SRSLTE_SUCCESS))  {
        printf("Graceperiod: Decode failed, but was expected, as codrate is (nearly) larger 1!\n");

        printf("Tx payload: ");
        srslte_vec_fprint_byte(stdout, payload_tx, (sci.mcs.tbs + 24)/8);
        printf("Rx payload: ");
        srslte_vec_fprint_byte(stdout, payload_rx, (sci.mcs.tbs + 24)/8);

        continue;
      }

      printf("Tx payload: ");
      srslte_vec_fprint_byte(stdout, payload_tx, (sci.mcs.tbs + 24)/8);
      printf("Rx payload: ");
      srslte_vec_fprint_byte(stdout, payload_rx, (sci.mcs.tbs + 24)/8);

      if ((ret_enc == SRSLTE_SUCCESS) && (ret_dec == SRSLTE_SUCCESS) && !memcmp(payload_rx, payload_tx, sizeof(uint8_t) * (sci.mcs.tbs)/8 - 0)) {
        printf("OK\n");
      } else {
        printf("Error\n");
        exit(-1);
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
  return 0;
}
