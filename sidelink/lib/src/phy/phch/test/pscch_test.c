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
  srslte_pscch_t pscch;
  uint8_t payload_tx[SRSLTE_SCI1_MAX_BITS+16], payload_rx[SRSLTE_SCI1_MAX_BITS+16];
  int i, j;
  cf_t *ce[SRSLTE_MAX_PORTS];
  int nof_re;
  cf_t *subframe_symbols[SRSLTE_MAX_PORTS];

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
    subframe_symbols[i] = malloc(sizeof(cf_t) * nof_re);
    if (!subframe_symbols[i]) {
      perror("malloc");
      exit(-1);
    }

  }

  if (srslte_pscch_init_ue(&pscch, cell.nof_prb, 1)) {
    fprintf(stderr, "Error creating PSCCH object\n");
    exit(-1);
  }
  if (srslte_pscch_set_cell(&pscch, cell)) {
    fprintf(stderr, "Error creating PSCCH object\n");
    exit(-1);
  }

  srand(time(NULL));
  for (i=0;i<SRSLTE_SCI1_MAX_BITS;i++) {
    payload_tx[i] = rand()%2;
  }
  

  if(SRSLTE_SUCCESS != srslte_pscch_sci_encode(&pscch, payload_tx, pscch.e, SRSLTE_SCI1_MAX_BITS, pscch.max_bits)) {
    printf("srslte_pscch_sci_encode failed\n");
    exit(-1);
  }

  // copy bits from uint8(0/1) to float(-0.5/0.5)
  for(int i=0; i<pscch.max_bits; i++ ) {
    pscch.llr[i] = (float)pscch.e[i] - 0.5f;
  }

  if(SRSLTE_SUCCESS != srslte_pscch_dci_decode(&pscch, pscch.llr, payload_rx, pscch.max_bits, SRSLTE_SCI1_MAX_BITS, NULL)) {
    printf("srslte_pscch_sci_encode failed\n");
    exit(-1);
  }

  printf("Tx payload: ");
  srslte_vec_fprint_hex(stdout, payload_tx, SRSLTE_SCI1_MAX_BITS+16);
  printf("Rx payload: ");
  srslte_vec_fprint_hex(stdout, payload_rx, SRSLTE_SCI1_MAX_BITS+16);

  if (!memcmp(payload_rx, payload_tx, sizeof(uint8_t) * SRSLTE_SCI1_MAX_BITS - 0)) {
    printf("OK\n");
  } else {
    printf("Error\n");
    exit(-1);
  }


  // check all possible pssch location, these may also include invalid ones
  for(int prb_offset=0; prb_offset<pscch.cell.nof_prb-2; prb_offset++) {

    for (i=0;i<SRSLTE_SCI1_MAX_BITS;i++) {
      payload_tx[i] = rand()%2;
    }

    if(SRSLTE_SUCCESS != srslte_pscch_encode(&pscch, payload_tx, 0, subframe_symbols)) {
      printf("Failed to encode PSCCH for PRB offset: %d \n", prb_offset);
      exit(-1);
    }

    if (srslte_pscch_extract_llr(&pscch, subframe_symbols[0], 
                      ce, 0 /*srslte_chest_dl_get_noise_estimate(&chest)*/, 
                      0 %10, 0)) {
    fprintf(stderr, "Error extracting LLRs\n");
    return -1;
    }

    if(SRSLTE_SUCCESS != srslte_pscch_dci_decode(&pscch, pscch.llr, payload_rx, pscch.max_bits, SRSLTE_SCI1_MAX_BITS, NULL)) {
      printf("Failed to decode SCI\n");
      exit(-1);
    }

    printf("Tx payload: ");
    srslte_vec_fprint_hex(stdout, payload_tx, SRSLTE_SCI1_MAX_BITS+16);
    printf("Rx payload: ");
    srslte_vec_fprint_hex(stdout, payload_rx, SRSLTE_SCI1_MAX_BITS+16);

    if (!memcmp(payload_rx, payload_tx, sizeof(uint8_t) * SRSLTE_SCI1_MAX_BITS - 0)) {
      printf("OK for prb offset %d\n", prb_offset);
    } else {
      printf("Error\n");
      exit(-1);
    }

  }

  
  // srslte_psbch_free(&psbch);
  srslte_pscch_free(&pscch);

  for (i=0;i<cell.nof_ports;i++) {
    free(ce[i]);
    free(subframe_symbols[i]);
  }
  return 0;
}
