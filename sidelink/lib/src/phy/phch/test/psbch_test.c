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
  6,            // nof_prb
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
  srslte_psbch_t psbch;
  uint8_t bch_payload_tx[SRSLTE_SL_BCH_PAYLOAD_LEN], bch_payload_rx[SRSLTE_SL_BCH_PAYLOAD_LEN];
  int i, j;
  cf_t *ce[SRSLTE_MAX_PORTS];
  int nof_re;
  cf_t *slot1_symbols[SRSLTE_MAX_PORTS];
  uint32_t nof_rx_ports; 

  parse_args(argc,argv);

  nof_re = 2*SRSLTE_SLOT_LEN_RE(cell.nof_prb, SRSLTE_CP_NORM); 

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
    slot1_symbols[i] = malloc(sizeof(cf_t) * nof_re);
    if (!slot1_symbols[i]) {
      perror("malloc");
      exit(-1);
    }

  }
  if (srslte_psbch_init(&psbch)) {
    fprintf(stderr, "Error creating PSBCH object\n");
    exit(-1);
  }
  if (srslte_psbch_set_cell(&psbch, cell)) {
    fprintf(stderr, "Error creating PSBCH object\n");
    exit(-1);
  }

  srand(time(NULL));
  for (i=0;i<SRSLTE_SL_BCH_PAYLOAD_LEN;i++) {
    bch_payload_tx[i] = rand()%2;
  }
  

    // setup dft precoding
    if (srslte_dft_precoding_init(&psbch.dft_precoding, 6/*max_prb*/, true/*is_ue*/)) {
      fprintf(stderr, "Error initiating DFT transform precoding\n");
    }

  srslte_psbch_encode(&psbch, bch_payload_tx, slot1_symbols);

  /* combine outputs */
  for (i=1;i<cell.nof_ports;i++) {
    for (j=0;j<nof_re;j++) {
      slot1_symbols[0][j] += slot1_symbols[i][j];
    }
  }

  // setup dft de-precoding
  if (srslte_dft_precoding_init(&psbch.dft_precoding, 6/*max_prb*/, false/*is_ue*/)) {
    fprintf(stderr, "Error initiating DFT transform precoding\n");
  }
  
  if (1 != srslte_psbch_decode(&psbch, slot1_symbols[0], ce[0], 0, bch_payload_rx)) {
    printf("Error decoding\n");
    exit(-1);
  }
  
  // this is not decoded by SL-MIB
  nof_rx_ports = 1;
  
  srslte_psbch_free(&psbch);

  for (i=0;i<cell.nof_ports;i++) {
    free(ce[i]);
    free(slot1_symbols[i]);
  }
  printf("Tx ports: %d - Rx ports: %d\n", cell.nof_ports, nof_rx_ports);
  printf("Tx payload: ");
  srslte_vec_fprint_hex(stdout, bch_payload_tx, SRSLTE_SL_BCH_PAYLOAD_LEN);
  printf("Rx payload: ");
  srslte_vec_fprint_hex(stdout, bch_payload_rx, SRSLTE_SL_BCH_PAYLOAD_LEN);

  if (nof_rx_ports == cell.nof_ports && !memcmp(bch_payload_rx, bch_payload_tx, sizeof(uint8_t) * SRSLTE_BCH_PAYLOAD_LEN - 1)) {
    printf("OK\n");
    exit(0);
  } else {
    printf("Error\n");
    exit(-1);
  }
}
