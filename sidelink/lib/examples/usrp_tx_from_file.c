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
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <complex.h>

#include "srslte/phy/rf/rf.h"
#include "srslte/srslte.h"

uint32_t nof_frames = 20;
uint32_t number_of_tx_samples = 15360;

float rf_rx_gain=40, srslte_rf_tx_gain=40, rf_freq=2.4e9;
float rf_rate = 15.36e6;
char *rf_args="";
char *output_filename = NULL;
char *input_filename = NULL;

static bool ctrl_c = false;

void int_handler(int dummy) {
  printf("captured ctrl-c\n");
  ctrl_c = true;
}

void usage(char *prog) {
  printf("Usage: %s -i [tx_signal_file]\n", prog);
  printf("\t-a RF args [Default %s]\n", rf_args);
  printf("\t-f RF TX/RX frequency [Default %.2f MHz]\n", rf_freq/1e6);
  printf("\t-g RF RX gain [Default %.1f dB]\n", rf_rx_gain);
  printf("\t-G RF TX gain [Default %.1f dB]\n", srslte_rf_tx_gain);
  printf("\t-i File name to read signal from\n");
  printf("\t-N Numbert of IQ samples to read from file [Default %d]\n", number_of_tx_samples);

  printf("\t-r RF Rate [Default %.6f Hz]\n", rf_rate);
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "ioafgGNr")) != -1) {
    switch (opt) {
    case 'a':
      rf_args = argv[optind];
      break;
    case 'o':
      output_filename = argv[optind];
      break;
    case 'i':
      input_filename = argv[optind];
      break;
    case 'N':
      number_of_tx_samples = atoi(argv[optind]);
      break;
    case 'f':
      rf_freq = atof(argv[optind]);
      break;
    case 'r':
      rf_rate = atof(argv[optind]);
      break;
    case 'g':
      rf_rx_gain = atof(argv[optind]);
      break;
    case 'G':
      srslte_rf_tx_gain = atof(argv[optind]);
      break;
    default:
      printf("Failed to match option: %c\n", opt);
      usage(argv[0]);
      exit(-1);
    }
  }
  if(input_filename == NULL)
  {
    printf("Failed to specify a file to read samples from.\n");
    usage(argv[0]);
    exit(-1);
  }
}

int main(int argc, char **argv) {
  signal(SIGINT, int_handler);

  parse_args(argc, argv);
  
  cf_t *tx_buffer[SRSLTE_MAX_PORTS];

  uint32_t flen = rf_rate/1000;

  if(number_of_tx_samples % flen != 0)
  {
    printf("Error: Number of IQ-samples(%d) must be an integer multiple of subframe length(%d).\n", number_of_tx_samples, flen);
    exit(-1);
  }
  
  tx_buffer[0] = malloc(sizeof(cf_t)*number_of_tx_samples);
  if (!tx_buffer[0]) {
    perror("malloc");
    exit(-1);
  }
  bzero(tx_buffer[0], sizeof(cf_t)*number_of_tx_samples);

  // read samples from file into memory
  srslte_filesource_t fsource;

  srslte_filesource_init(&fsource, input_filename, SRSLTE_COMPLEX_FLOAT_BIN);

  int rr = srslte_filesource_read_multi(&fsource, (void **)tx_buffer, number_of_tx_samples, 1);

  srslte_filesource_free(&fsource);

  if(rr < number_of_tx_samples)
  {
    printf("Warning: Only read %d of %d samples from the file.\n", rr, number_of_tx_samples);
  }

  int i;
  for(i=0; i<20; i++){
    printf("sample %d @%p %g+%gi\n", i, &(((cf_t *)(tx_buffer[0]))[i]), __real__ ((cf_t *)(tx_buffer[0]))[i],__imag__ ((cf_t *)(tx_buffer[0]))[i]);
  }

  for(i=number_of_tx_samples-20; i<number_of_tx_samples; i++){
    printf("sample %d @%p %g+%gi\n", i, &(((cf_t *)(tx_buffer[0]))[i]), __real__ ((cf_t *)(tx_buffer[0]))[i],__imag__ ((cf_t *)(tx_buffer[0]))[i]);
  }


  // Send through RF 
  srslte_rf_t rf; 
  printf("Opening RF device...%s\n", rf_args);
  if (srslte_rf_open(&rf, rf_args)) {
    fprintf(stderr, "Error opening rf\n");
    exit(-1);
  }


  srslte_rf_set_tx_srate(&rf, (double) rf_rate);
  
  printf("Subframe len:   %d samples\n", flen);
  printf("Set TX/RX rate: %.2f MHz\n", (float) rf_rate / 1000000);
  printf("Set RX gain:    %.1f dB\n", srslte_rf_set_rx_gain(&rf, rf_rx_gain));
  printf("Set TX gain:    %.1f dB\n", srslte_rf_set_tx_gain(&rf, srslte_rf_tx_gain));
  printf("Set TX/RX freq: %.2f MHz\n", srslte_rf_set_rx_freq(&rf, 0, rf_freq) / 1000000);
  srslte_rf_set_tx_freq(&rf, 0, rf_freq);
  
  sleep(1);
  
  srslte_rf_start_rx_stream(&rf, false);

  int sample_count = 0;

  printf("Starting transmit loop\n");

  // first send is with "start of burst" parameter
  srslte_rf_send_multi(&rf, (void **)tx_buffer, 1024, true, true, false);

  int loops = 0;

  while(!ctrl_c)
  {

    cf_t *tx_bb[SRSLTE_MAX_PORTS];

    // assign our moved buffer pointer to first entry
    tx_bb[0] = (cf_t *)(tx_buffer[0]) + sample_count;
    tx_bb[1] = NULL;
    tx_bb[2] = NULL;
    tx_bb[3] = NULL;

    // send chunks of subframe length 
    int r = srslte_rf_send_multi(&rf, (void **)tx_bb, flen, false, false, false);

    if(r != flen)
    {
      printf("Warning: Only sent %d samples of %d\n", r, flen);
    }

    //printf("loop %d, with %d samples and buffer %p\n", i, r, tx_bb[0]);

    sample_count = (sample_count + r) % number_of_tx_samples;

    if(sample_count == 0){
      printf(".");
      fflush(stdout);

      if((++loops %  100) == 0) printf("\nLoops: %d", loops);

    }

  }

  free(tx_buffer[0]);

  printf("Done\n");
  exit(0);
}
