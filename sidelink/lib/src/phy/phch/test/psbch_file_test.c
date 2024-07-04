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
#include "srslte/phy/phch/psbch.h"
#include "srslte/phy/ch_estimation/chest_ul.h"
#include "srslte/phy/ch_estimation/chest_sl.h"
#include <srslte/phy/common/phy_common.h>
#include "srslte/srslte.h"
#include <math.h>
#include "srslte/phy/ch_estimation/refsignal_sl.h"

srslte_cell_t cell = {
  6,            // nof_prb
  1,             //nof_ports
  301,          // cell_id  
  SRSLTE_CP_NORM,       // cyclic prefix 
  SRSLTE_PHICH_R_1,          // PHICH resources      
  SRSLTE_PHICH_NORM    // PHICH length  
};



char *input_file_name = NULL;
char *input_file_name;
int nof_frames=10;
float corr_peak_threshold=25.0;


#define CFO_AUTO  -9999.0
float force_cfo = CFO_AUTO; //0.45;//CFO_AUTO;
srslte_cfo_t cfocorr;

#define FLEN  (10*SRSLTE_SF_LEN(srslte_symbol_sz(cell.nof_prb)))

//#define FLEN   SRSLTE_SLOT_LEN(srslte_symbol_sz(cell.nof_prb)) * 2 




void usage(char *prog) {
  printf("Usage: %s [olntsNfcv] -i input_file\n", prog);
  printf("\t-n cell.nof_ports [Default %d]\n", cell.nof_prb);
  printf("\t-c cell.id [Default %d] \n", cell.id);
  printf("\t-p cell.nof_ports [Default %d] \n", cell.nof_ports);
  printf("\t-t correlation threshold [Default %g]\n", corr_peak_threshold);
  printf("\t-f force_cfo [Default disabled]\n");
  printf("\t-v srslte_verbose\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "inlptsNfcv")) != -1) {
    switch(opt) {
    case 'i':
      input_file_name = argv[optind];
      break;
    case 'n':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'c':
      cell.id = atof(argv[optind]);
      break;
    case 'p':
      cell.nof_ports = atof(argv[optind]);
      break;
    case 'v':
      srslte_verbose++;
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


srslte_filesource_t fsrc;
srslte_psbch_t psbch;
int frame_cnt;


int main(int argc, char **argv) {
  uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN];
  cf_t *input;
  int n_port=0; 
  float *cfo;
  cf_t *signal = NULL;

  {
  int i;
  for(i=0; i <argc; i++)
  {
    printf("%d %s\n", i, argv[i]);
  }
  }

  if (argc < 3) {
    usage(argv[0]);
    exit(-1);
  }

  parse_args(argc,argv);

  //base initialization //
  if (srslte_filesource_init(&fsrc, input_file_name, SRSLTE_COMPLEX_FLOAT_BIN)) {
    fprintf(stderr, "Error opening file %s\n", input_file_name);
    exit(-1);
  }

  input = malloc(FLEN*sizeof(cf_t));
  if (!input) {
    perror("malloc");
    exit(-1);
  }

  cfo = malloc(nof_frames*sizeof(float));
  if (!cfo) {
    perror("malloc");
    exit(-1);
  }
  
  if (srslte_cfo_init(&cfocorr, FLEN)) {
    fprintf(stderr, "Error initiating CFO\n");
    return -1;
  }

  signal = malloc(2 * SRSLTE_NRE * cell.nof_prb * sizeof(cf_t));
  if (!signal) {
    perror("malloc");
  }



  // fft initialization

  srslte_ofdm_t fft;
  cf_t *infft, *outfft;

  infft = srslte_vec_malloc(sizeof(cf_t) * FLEN);
  if (!infft) {
    perror("malloc");
    exit(-1);
  }

  outfft = srslte_vec_malloc(sizeof(cf_t) * SRSLTE_CP_NSYMB(cell.cp) * cell.nof_prb * SRSLTE_NRE * 2);
  if (!outfft) {
    perror("malloc");
    exit(-1);
  }

  if (srslte_ofdm_rx_init(&fft, cell.cp, infft, outfft, cell.nof_prb)) {
    fprintf(stderr, "Error initializing FFT\n");
    exit(-1);
  }


  srslte_ofdm_set_normalize(&fft, false);
  srslte_ofdm_set_freq_shift(&fft, -0.5);


  // channel estimation for psbch
  
  srslte_chest_sl_t est;
  cf_t *ce = NULL;


  ce = srslte_vec_malloc( 2 * cell.nof_prb * SRSLTE_NRE * SRSLTE_CP_NSYMB(cell.cp) * sizeof(cf_t));
  if (!ce) {
    perror("srslte_vec_malloc");
    exit(-1);
  }
  memset(ce, 0x00, 2 * cell.nof_prb * SRSLTE_NRE * SRSLTE_CP_NSYMB(cell.cp) * sizeof(cf_t));


  printf("srslte_chest_sl_init(est, nof_prfb=%d)\n", cell.nof_prb);
  if (srslte_chest_sl_init(&est, cell.nof_prb)) {
    fprintf(stderr, "Error initializing equalizer\n");
    exit(-1);
  }


  // psbch init 
   if (srslte_psbch_init(&psbch)) {
    fprintf(stderr, "Error creating PSBCH object\n");
    exit(-1);
  }


  // read the file and no of frames
  frame_cnt = 0;
  while (FLEN == srslte_filesource_read(&fsrc, input, FLEN)
      && frame_cnt < nof_frames) {

    if(frame_cnt == 0){
      int i;
      for(i=0; i<10; i++){
        printf("sample %d @%p %g+%gi\n", i, &(((cf_t *)(input))[i]), __real__ ((cf_t *)(input))[i],__imag__ ((cf_t *)(input))[i]);
        frame_cnt++;
      }

    }
  }


  // copy subframe into fft input buffer, we do it here, because currently we manually fixing the freqeuncy shift
  float myCFO = 0.0;
  
  // fft operation

      // correct cfo
  for(int s=0;s<SRSLTE_SLOT_LEN(srslte_symbol_sz(cell.nof_prb)) * 2; s++){
    infft[s] = input[s] * cexpf(I*2*M_PI*(s)*myCFO/srslte_symbol_sz(cell.nof_prb));
  }


  srslte_ofdm_rx_sf(&fft);
  //srslte_ofdm_rx_slot(&fft, 0);
  
  printf("srslte_chest_sl_set_cell(est, cell_id=%d rb: %d)\n", cell.id, cell.nof_prb);
  if (srslte_chest_sl_set_cell(&est, cell)) {
        fprintf(stderr, "Error initializing equalizer\n");
        exit(-1);
  }

  // Estimate channel
  srslte_chest_sl_estimate_psbch(&est,
      fft.out_buffer,//&input[peak_pos[N_id_2]-9-128-10],
      ce,
      SRSLTE_SL_MODE_4,
      n_port, 
      0); 



  if (srslte_psbch_set_cell(&psbch, cell)) {
    fprintf(stderr, "Error creating PSBCH object\n");
    exit(-1);
  }
       
  
  //srslte_psbch_decode_reset(&psbch);
  if (1 != srslte_psbch_decode(&psbch, fft.out_buffer, ce, 0, bch_payload)) {
    printf("Failed to decode PSBCH\n");
  }

  else { 
    //printf BCH Payload 
    srslte_vec_fprint_hex(stdout, bch_payload, SRSLTE_SL_BCH_PAYLOAD_LEN);
  }

}