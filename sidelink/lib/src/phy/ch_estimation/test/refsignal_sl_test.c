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
#include <strings.h>
#include <unistd.h>
#include <complex.h>

#include "srslte/srslte.h"

srslte_cell_t cell_n = {
  100,            // nof_prb
  SRSLTE_MAX_PORTS,    // nof_ports
  1,         // cell_n_id
  SRSLTE_CP_NORM,        // cyclic prefix
  SRSLTE_PHICH_NORM,
  SRSLTE_PHICH_R_1_6
};

void usage_n(char *prog) {
  printf("Usage: %s [recv]\n", prog);

  printf("\t-r nof_prb [Default %d]\n", cell_n.nof_prb);
  printf("\t-e extended cyclic prefix [Default normal]\n");

  printf("\t-c cell_id (1000 tests all). [Default %d]\n", cell_n.id);

  printf("\t-v increase verbosity\n");
}

void parse_args_n(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "recv")) != -1) {
    switch(opt) {
    case 'r':
      cell_n.nof_prb = atoi(argv[optind]);
      break;
    case 'e':
      cell_n.cp = SRSLTE_CP_EXT;
      break;
    case 'c':
      cell_n.id = atoi(argv[optind]);
      break;
    case 'v':
      srslte_verbose++;
      break;
    default:
      usage_n(argv[0]);
      exit(-1);
    }
  }
}

int main(int argc, char **argv) {
  srslte_refsignal_sl_t refs;
  // srslte_refsignal_dmrs_pusch_cfg_t pusch_cfg;
  cf_t *signal = NULL;
  int ret = -1;
 // int flag=0;
  // int count=0;
 // int cshift_dmrs=0;
  
  parse_args_n(argc,argv);

  if (srslte_refsignal_ul_init(&refs, cell_n.nof_prb)) {
    fprintf(stderr, "Error initializing SL reference signal\n");
    goto do_exit;
  }

  if (srslte_refsignal_ul_set_cell(&refs, cell_n)) {
    fprintf(stderr, "Error initializing SL reference signal\n");
    goto do_exit;
  }

  // Lenght is 12*[SCs/RB] * N_PRB [RBs] * 2 [slots/subframe] * 2 [DMRS/slot]
  signal = malloc(2 * SRSLTE_NRE * cell_n.nof_prb * sizeof(cf_t)*2);
  if (!signal) {
    perror("malloc");
    goto do_exit;
  }
  printf("Running tests for %d PRB\n", cell_n.nof_prb);
    
  for (int n=3;n<=cell_n.nof_prb;n++) {
    //for (int delta_ss=0;delta_ss<SRSLTE_NOF_DELTA_SS;delta_ss++)
    {
      //for (int cshift=0;cshift<SRSLTE_NOF_CSHIFT;cshift++)
      {
        //for (int n_PSSCH_ssf=0;n_PSSCH_ssf<10;n_PSSCH_ssf++)
        {

          // as running all combinations takes too long, we randomly
          // select one for each prb set
          int delta_ss = rand() % SRSLTE_NOF_DELTA_SS;
          int cshift = rand() % SRSLTE_NOF_CSHIFT;
          int n_PSSCH_ssf = rand() % 10;

          // cyclic shift and delta_ss are directly derived from 
          uint32_t n_X_ID = 16*delta_ss + cshift*2;
          uint32_t nof_prb = n;
          printf("nof_prb: %d, ",nof_prb);
          printf("cyclic_shift: %d, ", cshift);
//            printf("cyclic_shift_for_dmrs: %d, ", cshift_dmrs);
          printf("delta_ss: %d, ", delta_ss);
          printf("SF_idx: %d\n", n_PSSCH_ssf);
          struct timeval t[3]; 
          
          gettimeofday(&t[1], NULL);
          if(SRSLTE_SUCCESS != srslte_refsignal_sl_dmrs_pssch_gen(&refs, nof_prb, n_PSSCH_ssf, n_X_ID, signal)) {
            ret = -1;
            goto do_exit;
          }

//	    if(flag==0){
          // for(count=0;count<12/*(12*n*2*2)*/;count++){
          //   printf("%f+i%f  ",creal(signal[count]),cimag(signal[count]));
          // }
//            flag=1;
          // printf("\n The length of DMRS is %d\n",48*nof_prb);
//	    }
          gettimeofday(&t[2], NULL);
          get_time_interval(t);
          printf("DMRS ExecTime: %ld us\n", t[0].tv_usec);

/*            gettimeofday(&t[1], NULL);
          srslte_refsignal_srs_gen(&refs, sf_idx, signal);
          gettimeofday(&t[2], NULL);
          get_time_interval(t);
          printf("SRS ExecTime: %ld us\n", t[0].tv_usec);*/
        }    
      }
    }
  }

  ret = 0;

do_exit:

  if (signal) {
    free(signal);
  }

  srslte_refsignal_ul_free(&refs);
  
  if (!ret) {
    printf("OK\n");
  } 
  exit(ret);
}
