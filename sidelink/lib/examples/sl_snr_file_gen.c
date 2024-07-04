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
#include <sys/select.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <getopt.h>
#include <srslte/phy/common/phy_common.h>
#include <srslte/phy/phch/pdsch_cfg.h>

#include "srslte/srslte.h"
#include "srslte/common/crash_handler.h"


#define UE_CRNTI 0x1234
#define M_CRNTI 0xFFFD

#ifndef DISABLE_RF
//#include "srslte/phy/rf/rf.h"
//#include "srslte/phy/common/phy_common.h"
//srslte_rf_t rf;
#else
#warning Compiling pdsch_ue with no RF support
#endif

char *output_file_name = NULL;

#define LEFT_KEY  68
#define RIGHT_KEY 67
#define UP_KEY    65
#define DOWN_KEY  66

srslte_cell_t cell = {
  50,               // nof_prb
  1,                // nof_ports
  301,                // cell_id
  SRSLTE_CP_NORM,   // cyclic prefix
  SRSLTE_PHICH_NORM, // PHICH length
  SRSLTE_PHICH_R_1 // PHICH resources
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
  10,//uint8_t numSubchannel_r14;

  // indicates the lowest RB index of the subchannel with the lowest index
  0,//uint8_t startRB_Subchannel_r14;

  // indicates the lowest RB index of PSCCH pool, this field is absent whe a pool is (pre)configured
  // such tha a UE always transmits SC and data in adjacent RBs in the same subframe
  0,//uint8_t startRB_PSCCH_Pool_r14;
};


// SLOffsetIndicatorSync-r12: Synchronisation resources are present in those SFN and subframes which satisfy the relation: (SFN*10+ Subframe Number) mod 40 = SLOffsetIndicatorSync
uint32_t syncOffsetIndicator_r12 = 0;

uint32_t n_prb_pssch = 0;


uint16_t c = -1;
  
int net_port = -1; // -1 generates random dataThat means there is some problem sending samples to the device

uint32_t cfi = 2;
uint32_t mcs_idx = 1, last_mcs_idx = 1;
int nof_frames = -1;


srslte_tm_t transmission_mode    = SRSLTE_TM1;
uint32_t nof_tb = 1;
uint32_t multiplex_pmi = 0;
uint32_t multiplex_nof_layers = 1;

int mbsfn_area_id = -1;
char *rf_args = "";
float rf_amp = 0.8, rf_gain = 70.0, rf_freq = 2400000000;

float output_file_snr = +INFINITY;

bool null_file_sink=false; 
srslte_filesink_t fsink;
srslte_ofdm_t ifft[SRSLTE_MAX_PORTS];
srslte_ofdm_t ifft_mbsfn;
srslte_psbch_t psbch;
srslte_pscch_t pscch;
srslte_pssch_t pssch;
//srslte_pcfich_t pcfich;
srslte_pdcch_t pdcch;
srslte_pdsch_t pdsch;
srslte_pdsch_cfg_t pdsch_cfg;
srslte_pmch_t pmch;
srslte_pdsch_cfg_t  pmch_cfg;
srslte_softbuffer_tx_t *softbuffers[SRSLTE_MAX_CODEWORDS];
srslte_regs_t regs;
srslte_dci_dl_t dci_dl;
int rvidx[SRSLTE_MAX_CODEWORDS] = {0, 0};

srslte_repo_t repo;

cf_t *sf_buffer[SRSLTE_MAX_PORTS] = {NULL}, *output_buffer [SRSLTE_MAX_PORTS] = {NULL};


int sf_n_re, sf_n_samples;

pthread_t net_thread; 
void *net_thread_fnc(void *arg);
sem_t net_sem;
bool net_packet_ready = false; 
int net_packet_bytes = 8;
srslte_netsource_t net_source; 
srslte_netsink_t net_sink; 
srslte_netsink_t net_source2; 


int prbset_num = 1, last_prbset_num = 1; 
int prbset_orig = 0; 
//#define DATA_BUFF_SZ    1024*128
//uint8_t data[8*DATA_BUFF_SZ], data2[DATA_BUFF_SZ];
//uint8_t data_tmp[DATA_BUFF_SZ];


#define DATA_BUFF_SZ    1024*1024
uint8_t *data[2], data2[DATA_BUFF_SZ];
uint8_t data_tmp[DATA_BUFF_SZ];

void usage(char *prog) {
  printf("Usage: %s [agmfoncvpuxb]\n", prog);
#ifndef DISABLE_RF
  printf("\t-a RF args [Default %s]\n", rf_args);
  printf("\t-l RF amplitude [Default %.2f]\n", rf_amp);
  printf("\t-g RF TX gain [Default %.2f dB]\n", rf_gain);
  printf("\t-f RF TX frequency [Default %.1f MHz]\n", rf_freq / 1000000);
#else
  printf("\t   RF is disabled.\n");
#endif
  printf("\t-o output_file [Default use RF board]\n");
  printf("\t-m MCS index [Default %d]\n", mcs_idx);
  printf("\t-n number of frames [Default %d]\n", nof_frames);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-p nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-P nof_prb for PSSCH [Default %d]\n", n_prb_pssch);
  printf("\t-M MBSFN area id [Default %d]\n", mbsfn_area_id);
  printf("\t-x Transmission mode [1-4] [Default %d]\n", transmission_mode + 1);
  printf("\t-b Precoding Matrix Index (multiplex mode only)* [Default %d]\n", multiplex_pmi);
  printf("\t-w Number of codewords/layers (multiplex mode only)* [Default %d]\n", multiplex_nof_layers);
  printf("\t-u listen TCP port for input data (-1 is random) [Default %d]\n", net_port);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
  printf("\t-s output file SNR [Default %f]\n", output_file_snr);
  printf("\n");
  printf("\t*: See 3GPP 36.212 Table  5.3.3.1.5-4 for more information\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "aglfmoncpvutxbwMsP")) != -1) {

    switch (opt) {
    case 'a':
      rf_args = argv[optind];
      break;
    case 'g':
      rf_gain = atof(argv[optind]);
      break;
    case 'l':
      rf_amp = atof(argv[optind]);
      break;
    case 'f':
      rf_freq = atof(argv[optind]);
      break;
    case 'o':
      output_file_name = argv[optind];
      break;
    case 'm':
      mcs_idx = atoi(argv[optind]);
      break;
    case 'u':
      net_port = atoi(argv[optind]);
      break;
    case 'n':
      nof_frames = atoi(argv[optind]);
      break;
    case 'p':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'P':
      n_prb_pssch = atoi(argv[optind]);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'x':
      transmission_mode = (srslte_tm_t)(atoi(argv[optind]) - 1);
      break;
    case 'b':
      multiplex_pmi = (uint32_t) atoi(argv[optind]);
      break;
    case 'w':
      multiplex_nof_layers = (uint32_t) atoi(argv[optind]);
      break;
    case 'M':
      mbsfn_area_id = atoi(argv[optind]);
      break;
    case 'v':
      srslte_verbose++;
      break;
    case 's':
      output_file_snr = atof(argv[optind]);
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
#ifdef DISABLE_RF
  if (!output_file_name) {
    usage(argv[0]);
    exit(-1);
  }
#endif
}
  
void base_init() {
  int i;

  /* Configure cell and PDSCH in function of the transmission mode */
  switch (transmission_mode) {
    case SRSLTE_TM1:
      cell.nof_ports = 1;
      break;
    case SRSLTE_TM2:
    case SRSLTE_TM3:
    case SRSLTE_TM4:
      cell.nof_ports = 2;
      break;
    default:
      ERROR("Transmission mode %d not implemented or invalid\n", transmission_mode);
      exit(-1);
  }

  /* Allocate memory */
  for(i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    data[i] = srslte_vec_malloc(sizeof(uint8_t) * SOFTBUFFER_SIZE);
    if (!data[i]) {
      perror("malloc");
      exit(-1);
    }
    bzero(data[i], sizeof(uint8_t) * SOFTBUFFER_SIZE);
  }

  /* init memory */
  for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
    sf_buffer[i] = srslte_vec_malloc(sizeof(cf_t) * sf_n_re);
    if (!sf_buffer[i]) {
      perror("malloc");
      exit(-1);
    }
  }

  for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
    output_buffer[i] = srslte_vec_malloc(sizeof(cf_t) * sf_n_samples);
    if (!output_buffer[i]) {
      perror("malloc");
      exit(-1);
    }
    bzero(output_buffer[i], sizeof(cf_t) * sf_n_samples);
  }


  /* open file or USRP */
  if (output_file_name) {
    if (strcmp(output_file_name, "NULL")) {
      if (srslte_filesink_init(&fsink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN)) {
        fprintf(stderr, "Error opening file %s\n", output_file_name);
        exit(-1);
      }      
      null_file_sink = false; 
    } else {
      null_file_sink = true; 
    }
  } else {
// #ifndef DISABLE_RF
//     printf("Opening RF device...\n");
//     if (srslte_rf_open_multi(&rf, rf_args, cell.nof_ports)) {
//       fprintf(stderr, "Error opening rf\n");
//       exit(-1);
//     }
// #else
//     printf("Error RF not available. Select an output file\n");
//     exit(-1);
// #endif
  }
  if (net_port > 0) {
    if (srslte_netsource_init(&net_source, "0.0.0.0", net_port, SRSLTE_NETSOURCE_TCP)) {
      fprintf(stderr, "Error creating input UDP socket at port %d\n", net_port);
      exit(-1);
    }
    printf("setting up sink as client on port %d\n", net_port+2);
    if (srslte_netsink_init(&net_source2, "127.0.0.1", net_port+2, SRSLTE_NETSINK_TCP)) {
      fprintf(stderr, "Error source as client\n");
      exit(-1);
    }  
    if (null_file_sink) {
      if (srslte_netsink_init(&net_sink, "127.0.0.1", net_port+1, SRSLTE_NETSINK_TCP)) {
        fprintf(stderr, "Error sink\n");
        exit(-1);
      }      
    }
    if (sem_init(&net_sem, 0, 1)) {
      perror("sem_init");
      exit(-1);
    }
  }

  /* create ifft object */
  for (i = 0; i < cell.nof_ports; i++) {
    if (srslte_ofdm_tx_init(&ifft[i], SRSLTE_CP_NORM, sf_buffer[i], output_buffer[i], cell.nof_prb)) {
      fprintf(stderr, "Error creating iFFT object\n");
      exit(-1);
    }

    srslte_ofdm_set_freq_shift(&ifft[i], 0.5);
    srslte_ofdm_set_normalize(&ifft[i], true);
  }

  if (srslte_ofdm_tx_init_mbsfn(&ifft_mbsfn, SRSLTE_CP_EXT, sf_buffer[0], output_buffer[0], cell.nof_prb)) {
    fprintf(stderr, "Error creating iFFT object\n");
    exit(-1);
  }
  srslte_ofdm_set_non_mbsfn_region(&ifft_mbsfn, 2);
  srslte_ofdm_set_normalize(&ifft_mbsfn, true);
  
  if (srslte_psbch_init(&psbch)) {
    fprintf(stderr, "Error creating PSBCH object\n");
    exit(-1);
  }
  if (srslte_psbch_set_cell(&psbch, cell)) {
    fprintf(stderr, "Error creating PSBCH object\n");
    exit(-1);
  }

  // setup dft precoding
  if (srslte_dft_precoding_init(&psbch.dft_precoding, 6/*max_prb*/, true/*is_ue*/)) {
    fprintf(stderr, "Error initiating DFT transform precoding\n");
  }
  
  if (srslte_pscch_init_ue(&pscch, cell.nof_prb, 1)) {
    fprintf(stderr, "Error creating PSCCH object\n");
    exit(-1);
  }
  if (srslte_pscch_set_cell(&pscch, cell)) {
    fprintf(stderr, "Error creating PSCCH object\n");
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


  if (srslte_repo_init(&repo, cell, respool)) {
    fprintf(stderr, "Error creating Ressource Pool object\n");
    exit(-1);
  }

  if (srslte_regs_init(&regs, cell)) {
    fprintf(stderr, "Error initiating regs\n");
    exit(-1);
  }
  // if (srslte_pcfich_init(&pcfich, 1)) {
  //   fprintf(stderr, "Error creating PBCH object\n");
  //   exit(-1);
  // }
  // if (srslte_pcfich_set_cell(&pcfich, &regs, cell)) {
  //   fprintf(stderr, "Error creating PBCH object\n");
  //   exit(-1);
  // }

  if (srslte_pdcch_init_enb(&pdcch, cell.nof_prb)) {
    fprintf(stderr, "Error creating PDCCH object\n");
    exit(-1);
  }
  if (srslte_pdcch_set_cell(&pdcch, &regs, cell)) {
    fprintf(stderr, "Error creating PDCCH object\n");
    exit(-1);
  }

  if (srslte_pdsch_init_enb(&pdsch, cell.nof_prb)) {
    fprintf(stderr, "Error creating PDSCH object\n");
    exit(-1);
  }
  if (srslte_pdsch_set_cell(&pdsch, cell)) {
    fprintf(stderr, "Error creating PDSCH object\n");
    exit(-1);
  }

  srslte_pdsch_set_rnti(&pdsch, UE_CRNTI);


  if(mbsfn_area_id > -1){
    if (srslte_pmch_init(&pmch, cell.nof_prb, 1)) {
      fprintf(stderr, "Error creating PMCH object\n");
    }
    srslte_pmch_set_area_id(&pmch, mbsfn_area_id);
  }
  
  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    softbuffers[i] = calloc(sizeof(srslte_softbuffer_tx_t), 1);
    if (!softbuffers[i]) {
      fprintf(stderr, "Error allocating soft buffer\n");
      exit(-1);
    }

    if (srslte_softbuffer_tx_init(softbuffers[i], cell.nof_prb)) {
      fprintf(stderr, "Error initiating soft buffer\n");
      exit(-1);
    }
  }
}


void base_free() {
  int i;
  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    srslte_softbuffer_tx_free(softbuffers[i]);
    if (softbuffers[i]) {
      free(softbuffers[i]);
    }
  }
  srslte_pdsch_free(&pdsch);
  srslte_pdcch_free(&pdcch);
  srslte_regs_free(&regs);
  srslte_psbch_free(&psbch);
  if(mbsfn_area_id > -1){
    srslte_pmch_free(&pmch); 
  }
  srslte_ofdm_tx_free(&ifft_mbsfn);
  for (i = 0; i < cell.nof_ports; i++) {
    srslte_ofdm_tx_free(&ifft[i]);
  }

  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    if (data[i]) {
      free(data[i]);
    }
  }

  for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
    if (sf_buffer[i]) {
      free(sf_buffer[i]);
    }

    if (output_buffer[i]) {
      free(output_buffer[i]);
    }
  }
  if (output_file_name) {
    if (!null_file_sink) {
      srslte_filesink_free(&fsink);      
    }
  } else {
// #ifndef DISABLE_RF
//     srslte_rf_close(&rf);
// #endif
  }
  
  if (net_port > 0) {
    srslte_netsource_free(&net_source);
    sem_close(&net_sem);
  }  
}


bool go_exit = false; 
void sig_int_handler(int signo)
{
  printf("SIGINT received. Exiting...\n");
  if (signo == SIGINT) {
    go_exit = true;
  }
}



unsigned int
reverse(register unsigned int x)
{
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
    return((x >> 16) | (x << 16));

}

uint32_t prbset_to_bitmask() {
  uint32_t mask=0;
  int nb = (int) ceilf((float) cell.nof_prb / srslte_ra_type0_P(cell.nof_prb));
  for (int i=0;i<nb;i++) {
    if (i >= prbset_orig && i < prbset_orig + prbset_num) {
      mask = mask | (0x1<<i);     
    }
  }
  return reverse(mask)>>(32-nb); 
}

int update_radl() {

  ZERO_OBJECT(dci_dl);

  /* Configure cell and PDSCH in function of the transmission mode */
  switch (transmission_mode) {
    case SRSLTE_TM1:
    case SRSLTE_TM2:
      nof_tb = 1;
      dci_dl.format = SRSLTE_DCI_FORMAT1;
      break;
    case SRSLTE_TM3:
      dci_dl.format = SRSLTE_DCI_FORMAT2A;
      nof_tb = 2;
      break;
    case SRSLTE_TM4:
      dci_dl.format = SRSLTE_DCI_FORMAT2;
      nof_tb = multiplex_nof_layers;
      if (multiplex_nof_layers == 1) {
        dci_dl.pinfo = (uint8_t)(multiplex_pmi + 1);
      } else {
        dci_dl.pinfo = (uint8_t)multiplex_pmi;
      }
      break;
    default:
      ERROR("Transmission mode not implemented.");
      exit(-1);
  }

  dci_dl.rnti                    = UE_CRNTI;
  dci_dl.pid                     = 0;
  dci_dl.tb[0].mcs_idx           = mcs_idx;
  dci_dl.tb[0].ndi               = 0;
  dci_dl.tb[0].rv                = rvidx[0];
  dci_dl.tb[0].cw_idx            = 0;
  dci_dl.alloc_type              = SRSLTE_RA_ALLOC_TYPE0;
  dci_dl.type0_alloc.rbg_bitmask = prbset_to_bitmask();

  if (nof_tb > 1) {
    dci_dl.tb[1].mcs_idx = mcs_idx;
    dci_dl.tb[1].ndi     = 0;
    dci_dl.tb[1].rv      = rvidx[1];
    dci_dl.tb[1].cw_idx  = 1;
  } else {
    SRSLTE_DCI_TB_DISABLE(dci_dl.tb[1]);
  }

  srslte_dci_dl_fprint(stdout, &dci_dl, cell.nof_prb);
  if (transmission_mode != SRSLTE_TM1) {
    printf("\nTransmission mode key table:\n");
    printf("   Mode   |   1TB   | 2TB |\n");
    printf("----------+---------+-----+\n");
    printf("Diversity |    x    |     |\n");
    printf("      CDD |         |  z  |\n");
    printf("Multiplex | q,w,e,r | a,s |\n");
    printf("\n");
    printf("Type new MCS index (0-28) or mode key and press Enter: ");
  } else {
    printf("Type new MCS index (0-28) and press Enter: ");
  }
  fflush(stdout);


  return 0; 
}


/* Read new MCS from stdin */
int update_control() {
  char input[128];
  
  fd_set set; 
  FD_ZERO(&set);
  FD_SET(0, &set);
  
  struct timeval to; 
  to.tv_sec = 0; 
  to.tv_usec = 0; 

  int n = select(1, &set, NULL, NULL, &to);
  if (n == 1) {
    // stdin ready
    if (fgets(input, sizeof(input), stdin)) {
      if(input[0] == 27) {
        switch(input[2]) {
          case RIGHT_KEY:
            if (prbset_orig  + prbset_num < (int) ceilf((float) cell.nof_prb / srslte_ra_type0_P(cell.nof_prb)))
              prbset_orig++;
            break;
          case LEFT_KEY:
            if (prbset_orig > 0)
              prbset_orig--;
            break;
          case UP_KEY:
            if (prbset_num < (int) ceilf((float) cell.nof_prb / srslte_ra_type0_P(cell.nof_prb)))
              prbset_num++;
            break;
          case DOWN_KEY:
            last_prbset_num = prbset_num;
            if (prbset_num > 0)
              prbset_num--;          
            break;          
        }
      } else {
        switch (input[0]) {
          case 'q':
            transmission_mode    = SRSLTE_TM4;
            multiplex_pmi        = 0;
            multiplex_nof_layers = 1;
            break;
          case 'w':
            transmission_mode    = SRSLTE_TM4;
            multiplex_pmi        = 1;
            multiplex_nof_layers = 1;
            break;
          case 'e':
            transmission_mode    = SRSLTE_TM4;
            multiplex_pmi        = 2;
            multiplex_nof_layers = 1;
            break;
          case 'r':
            transmission_mode    = SRSLTE_TM4;
            multiplex_pmi        = 3;
            multiplex_nof_layers = 1;
            break;
          case 'a':
            transmission_mode    = SRSLTE_TM4;
            multiplex_pmi        = 0;
            multiplex_nof_layers = 2;
            break;
          case 's':
            transmission_mode    = SRSLTE_TM4;
            multiplex_pmi        = 1;
            multiplex_nof_layers = 2;
            break;
          case 'z':
            transmission_mode = SRSLTE_TM3;
            break;
          case 'x':
            transmission_mode = SRSLTE_TM2;
            break;
          default:
            last_mcs_idx = mcs_idx;
            mcs_idx = atoi(input);
        }

      }
      bzero(input,sizeof(input));
      if (update_radl()) {
        printf("Trying with last known MCS index\n");
        mcs_idx = last_mcs_idx; 
        prbset_num = last_prbset_num; 
        return update_radl();
      }
    }
    return 0; 
  } else if (n < 0) {
    // error
    perror("select");
    return -1; 
  } else {
    return 0; 
  }
}


/** Function run in a separate thread to receive UDP data */
void *net_thread_fnc(void *arg) {
  int n; 
  int rpm = 0, wpm=0; 
  
  do {
    // reset the left in buffer bytes
    rpm = 0;

    //n = srslte_netsource_read(&net_source, &data2[rpm], DATA_BUFF_SZ-rpm);
    n = srslte_netsink_read(&net_source2, &data2[rpm], DATA_BUFF_SZ-rpm);
    if (n > 0) {
      // FIXME: I assume that both transport blocks have same size in case of 2 tb are active
      // int nbytes = 1 + (pdsch_cfg.grant.mcs[0].tbs + pdsch_cfg.grant.mcs[1].tbs - 1) / 8;

      // assume a maximum sdu size of 1064/8 bytes
      int nbytes = 1064/8;

      if(nbytes > n ) {
        nbytes = n;
      }

      rpm += n; 
      INFO("received %d bytes. rpm=%d/%d\n",n,rpm,nbytes);
      wpm = 0; 
      while (rpm >= nbytes) {
        // wait for packet to be transmitted
        sem_wait(&net_sem);

        memcpy(data[0], &data2[wpm], nbytes);
        //memcpy(data[1], &data2[wpm], nbytes / (size_t) 2);
        INFO("Sent %d/%d bytes ready\n", nbytes, rpm);
        rpm -= nbytes;          
        wpm += nbytes; 

        net_packet_bytes = nbytes;
        net_packet_ready = true; 
      }
      if (wpm > 0) {
        INFO("%d bytes left in buffer for next packet\n", rpm);
        memcpy(data2, &data2[wpm], rpm * sizeof(uint8_t));
      }
    } else if (n == 0) {
      rpm = 0; 
    } else {
      fprintf(stderr, "Error receiving from network\n");
      exit(-1);
    }      
  } while(n >= 0);
  return NULL;
}


int main(int argc, char **argv) {
  int nf=0, sf_idx=0, N_id_2=0;
  cf_t psss_signal[SRSLTE_PSSS_LEN];
  float ssss_signal[SRSLTE_SSSS_LEN];
  uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN];
  int i;
  cf_t *sf_symbols[SRSLTE_MAX_PORTS];
  // srslte_dci_msg_t dci_msg;
  // srslte_dci_location_t locations[SRSLTE_NOF_SF_X_FRAME][30];
  uint32_t sfn; 
  //srslte_refsignal_t csr_refs;
  // srslte_refsignal_t mbsfn_refs;

  // srslte_use_standard_symbol_size(true);

  parse_args(argc, argv);

  srslte_refsignal_ul_t refs;

  cf_t *psbch_dmrs = NULL;
  cf_t *pscch_dmrs = NULL;
  cf_t *pssch_dmrs = NULL;

  if (srslte_refsignal_ul_init(&refs, cell.nof_prb)) {
    fprintf(stderr, "Error initializing UL reference signal\n");
    exit(-1);
  }

  if (srslte_refsignal_ul_set_cell(&refs, cell)) {
    fprintf(stderr, "Error initializing UL reference signal\n");
    exit(-1);
  }

  psbch_dmrs = malloc(3 * SRSLTE_NRE * 6 * sizeof(cf_t));
  if (!psbch_dmrs) {
    perror("malloc");
    exit(-1);
  }

  // generate psbch dmrs
  srslte_refsignal_sl_dmrs_psbch_gen(&refs, 6, 0, 0, psbch_dmrs);

  pscch_dmrs = malloc(4 * SRSLTE_NRE * 2 * sizeof(cf_t));
  if (!pscch_dmrs) {
    perror("pscch_dmrs");
    exit(-1);
  }

  // generate pscch dmrs
  srslte_refsignal_sl_dmrs_pscch_gen(&refs, 2, 0, 0, pscch_dmrs) ;


  pssch_dmrs = malloc(4 * SRSLTE_NRE * cell.nof_prb * sizeof(cf_t));
  if (!pssch_dmrs) {
    perror("pssch_dmrs");
    exit(-1);
  }


  srslte_debug_handle_crash(argc, argv);

#ifdef DISABLE_RF
  if (argc < 3) {
    usage(argv[0]);
    exit(-1);
  }
#endif

  N_id_2 = cell.id / 168;
  sf_n_re = 2 * SRSLTE_CP_NORM_NSYMB * cell.nof_prb * SRSLTE_NRE;
  sf_n_samples = 2 * SRSLTE_SLOT_LEN(srslte_symbol_sz(cell.nof_prb));

  cell.phich_length = SRSLTE_PHICH_NORM;
  cell.phich_resources = SRSLTE_PHICH_R_1;
  sfn = 0;

  prbset_num = (int) ceilf((float) cell.nof_prb / srslte_ra_type0_P(cell.nof_prb)); 
  last_prbset_num = prbset_num; 
  
  /* this *must* be called after setting slot_len_* */
  base_init();

  /* Generate PSSS/SSS signals */
  srslte_psss_generate(psss_signal, N_id_2);
  srslte_ssss_generate(ssss_signal, cell.id);
  
  // set sync period to 5, as this is what the current sync implementations expects
  repo.syncPeriod = 5;
  srslte_repo_build_resource_pool(&repo);


  /* Generate reference signals */
  // if(srslte_refsignal_cs_init(&csr_refs, cell.nof_prb)) {
  //   fprintf(stderr, "Error initializing equalizer\n");
  //   exit(-1);
  // }
  // if(mbsfn_area_id > -1) {
  //   if(srslte_refsignal_mbsfn_init(&mbsfn_refs, cell, mbsfn_area_id)) {
  //     fprintf(stderr, "Error initializing equalizer\n");
  //     exit(-1);
  //   }
  // }
  
  // if(srslte_refsignal_cs_set_cell(&csr_refs, cell)){
  //   fprintf(stderr, "Error setting cell\n");
  //   exit(-1);
  // }
  
  
  for (i = 0; i < SRSLTE_MAX_PORTS; i++) {
    sf_symbols[i] = sf_buffer[i%cell.nof_ports];
  }


// #ifndef DISABLE_RF


//   sigset_t sigset;
//   sigemptyset(&sigset);
//   sigaddset(&sigset, SIGINT);
//   sigprocmask(SIG_UNBLOCK, &sigset, NULL);
//   signal(SIGINT, sig_int_handler);

//   if (!output_file_name) {
    
//     int srate = srslte_sampling_freq_hz(cell.nof_prb);    
//     if (srate != -1) {  
//       if (srate < 10e6) {          
//         srslte_rf_set_master_clock_rate(&rf, 4*srate);        
//       } else {
//         srslte_rf_set_master_clock_rate(&rf, srate);        
//       }
//       printf("Setting sampling rate %.2f MHz\n", (float) srate/1000000);
//       float srate_rf = srslte_rf_set_tx_srate(&rf, (double) srate);
//       if (srate_rf != srate) {
//         fprintf(stderr, "Could not set sampling rate\n");
//         exit(-1);
//       }
//     } else {
//       fprintf(stderr, "Invalid number of PRB %d\n", cell.nof_prb);
//       exit(-1);
//     }
//     printf("Set TX gain: %.1f dB\n", srslte_rf_set_tx_gain(&rf, rf_gain));
//     printf("Set TX freq: %.2f MHz\n",
//         srslte_rf_set_tx_freq(&rf, rf_freq) / 1000000);
//   }
// #endif

  // if (update_radl(sf_idx)) {
  //   exit(-1);
  // }
  
  if (net_port > 0) {
    if (pthread_create(&net_thread, NULL, net_thread_fnc, NULL)) {
      perror("pthread_create");
      exit(-1);
    }
  }
  
  // /* Initiate valid DCI locations */
  // for (i=0;i<SRSLTE_NOF_SF_X_FRAME;i++) {
  //   srslte_pdcch_ue_locations(&pdcch, locations[i], 30, i, cfi, UE_CRNTI);
  // }
    
  nf = 0;
  
  // bool send_data = false; 
  for (i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    srslte_softbuffer_tx_reset(softbuffers[i]);
  }


// #ifndef DISABLE_RF
//   bool start_of_burst = true; 
// #endif

  while ((nf < nof_frames || nof_frames == -1) && !go_exit) {
    for (sf_idx = 0; sf_idx < SRSLTE_NOF_SF_X_FRAME && (nf < nof_frames || nof_frames == -1); sf_idx++) {
      /* Set Antenna port resource elements to zero */
      bzero(sf_symbols[0], sizeof(cf_t) * sf_n_re);

      // adjust to periodicity and directSubframenumber-r12
      //if ( ((sfn*SRSLTE_NOF_SF_X_FRAME + sf_idx) % repo.syncPeriod) == repo.syncOffsetIndicator_r12) {
      if(0){
        srslte_psss_put_sf(psss_signal, sf_symbols[0], cell.nof_prb, SRSLTE_CP_NORM);
        srslte_ssss_put_sf(ssss_signal, sf_symbols[0], cell.nof_prb, SRSLTE_CP_NORM);

        srslte_refsignal_sl_dmrs_psbch_put(&refs, SRSLTE_SL_MODE_4, psbch_dmrs, sf_symbols[0]);

        srslte_psbch_mib_pack(&cell, sfn, sf_idx, bch_payload);
        srslte_psbch_encode(&psbch, bch_payload, sf_symbols);

      }

      int ssf = srslte_repo_get_n_PSSCH_ssf(&repo, sfn*SRSLTE_NOF_SF_X_FRAME + sf_idx);
      //if(-1 != ssf) {


      net_packet_bytes = 3496/8;//20;
      for(i=0; i<net_packet_bytes; i++) {
        data[0][i] = 0xFF;
      }


      if(1){
      //if ( net_port > 0 && net_packet_ready && (((sfn*SRSLTE_NOF_SF_X_FRAME + sf_idx) % repo.syncPeriod) != repo.syncOffsetIndicator_r12)) {
      //if ( (((sfn*SRSLTE_NOF_SF_X_FRAME + sf_idx) % repo.syncPeriod) != repo.syncOffsetIndicator_r12)) {
        // here comes pscch and pssch processing
        printf("ssf: %d\n",ssf);

        // fill in required values for sci
        srslte_ra_sl_sci_t sci;
        sci.priority = 0;
        sci.resource_reservation = 0;

        sci.time_gap = 0;
        sci.rti = 0;


        // payload size including mac headers
        uint32_t sdu_size = net_packet_bytes*8;// 88; 
        bool found_mcs = false;

        uint8_t L_subch = 10;//1;
        uint8_t n_subCH_start = 0;

        if(n_prb_pssch > 0) {
          // this section is used to simulate the clipping on transmit signals
          // it will break decoding capability as we change the resource pool configuration

          L_subch = n_prb_pssch + 2;
          repo.rp.sizeSubchannel_r14 = 1;
          repo.rp.numSubchannel_r14 = 50;

          sci.mcs.idx = 8;
          srslte_sl_fill_ra_mcs(&sci.mcs, n_prb_pssch);

          found_mcs = true;

          net_packet_bytes = sci.mcs.tbs/8;
          
        } else {


          // get valid prb sizes by checking all subchannel sizes
          for(L_subch = 1; L_subch<=repo.rp.numSubchannel_r14; L_subch++) {
            int n_prb = L_subch*repo.rp.sizeSubchannel_r14 - 2;

            // check if number of prb fullfills 2^a2*3^a3*5^a5, see14.1.1.4C
            if(!srslte_dft_precoding_valid_prb(n_prb)) {
              continue;
            }

            for(int mcs=0; mcs<12; mcs++) {
              sci.mcs.idx = mcs;
              srslte_sl_fill_ra_mcs(&sci.mcs, n_prb);

              if(sci.mcs.tbs >= sdu_size) {
                found_mcs = true;
                break;
              }
            }

            if(found_mcs) {
              break;
            }
          }
        }

        // clear padding bits
        memset(&data[0][net_packet_bytes], 0x00, sci.mcs.tbs/8 - net_packet_bytes);
        //printf("clear %d bytes\n", sci.mcs.tbs/8 - net_packet_bytes);


        if(!found_mcs) {
          printf("Failed to aquired a valid MCS for sdu size %d\n", sdu_size);
          continue;
        }
        
        float c_bits = 9*12*srslte_mod_bits_x_symbol(sci.mcs.mod)*(L_subch*repo.rp.sizeSubchannel_r14 - 2);
        float i_bits = sci.mcs.tbs;
        printf("TBS: %d PRB: %d Channel-Bits: %.0f Coderate: %f (+CRC: %f)\n",
                sci.mcs.tbs,
                (L_subch*repo.rp.sizeSubchannel_r14 - 2),
                c_bits,
                i_bits/c_bits,
                (i_bits+24)/c_bits);
        
        // select random between valid values
        n_subCH_start = rand() % (repo.rp.numSubchannel_r14 - L_subch + 1);

        // set RIV
        srslte_repo_encode_frl(&repo, &sci, L_subch, n_subCH_start);

        uint8_t sci_buffer[SRSLTE_SCI1_MAX_BITS + 16];

        memset(sci_buffer, 0x00, SRSLTE_SCI1_MAX_BITS);

        srslte_repo_sci_encode(&repo, sci_buffer, &sci);

        if(SRSLTE_SUCCESS != srslte_pscch_encode(&pscch, sci_buffer, repo.rp.startRB_Subchannel_r14 + n_subCH_start*repo.rp.sizeSubchannel_r14, sf_symbols)) {
          printf("Failed to encode PSCCH for PRB offset: %d \n", repo.rp.startRB_Subchannel_r14 + n_subCH_start*repo.rp.sizeSubchannel_r14);
          exit(-1);
        }

        srslte_refsignal_sl_dmrs_psxch_put(&refs, SRSLTE_SL_MODE_4, repo.rp.startRB_Subchannel_r14 + n_subCH_start*repo.rp.sizeSubchannel_r14, 2, pscch_dmrs, sf_symbols[0]);

        // retrieve crc for pssch dmrs generation
        uint8_t *x = &sci_buffer[SRSLTE_SCI1_MAX_BITS];
        uint16_t pscch_crc = (uint16_t) srslte_bit_pack(&x, 16);

        srslte_vec_fprint_hex(stdout, sci_buffer, SRSLTE_SCI1_MAX_BITS + 16);
        
        pssch.n_PSSCH_ssf = sf_idx;//0;//ssf;
        pssch.n_X_ID = pscch_crc;

        printf("n_X_ID(CRC): %04x n_PSSCH_ssf: %d\n", pssch.n_X_ID, pssch.n_PSSCH_ssf);

        srslte_softbuffer_tx_reset_tbs(softbuffers[0], (uint32_t) sci.mcs.tbs);
        int encode_ret = srslte_pssch_encode_simple(&pssch,
                              &sci,
                              softbuffers,//srslte_softbuffer_tx_t *softbuffers[SRSLTE_MAX_CODEWORDS],
                              sf_symbols,//cf_t *sf_symbols[SRSLTE_MAX_PORTS],
                              repo.rp.startRB_Subchannel_r14 + n_subCH_start*repo.rp.sizeSubchannel_r14 + 2,//repo.rp.startRB_Subchannel_r14 + rbp*repo.rp.sizeSubchannel_r14 + 2,//uint32_t prb_offset,
                              L_subch * repo.rp.sizeSubchannel_r14 - 2,//sci.frl_L_subCH*repo.rp.sizeSubchannel_r14 - 2,//uint32_t n_prb,
                              data);//uint8_t *data[SRSLTE_MAX_CODEWORDS])


        // generate pssch dmrs
        srslte_refsignal_sl_dmrs_pssch_gen(&refs,
                                            L_subch * repo.rp.sizeSubchannel_r14 - 2,
                                            pssch.n_PSSCH_ssf,
                                            pssch.n_X_ID, // CRC checksum
                                            pssch_dmrs);

        srslte_refsignal_sl_dmrs_psxch_put(&refs, SRSLTE_SL_MODE_4,
                                            repo.rp.startRB_Subchannel_r14 + n_subCH_start*repo.rp.sizeSubchannel_r14 + 2,
                                            L_subch * repo.rp.sizeSubchannel_r14 - 2,
                                            pssch_dmrs, sf_symbols[0]);  

        printf("srslte_pssch_encode_simple %d\n", encode_ret);


        net_packet_ready = false; 
        sem_post(&net_sem);


      }


      
      /* Copy zeros, SSS, PSS into the rest of antenna ports */
      for (i = 1; i < cell.nof_ports; i++) {
        memcpy(sf_symbols[i], sf_symbols[0], sizeof(cf_t) * sf_n_re);
      }
      
      // if(sf_idx == 1 && mbsfn_area_id > -1){
      //   srslte_refsignal_mbsfn_put_sf(cell, 0,csr_refs.pilots[0][sf_idx], mbsfn_refs.pilots[0][sf_idx],  sf_symbols[0]);
      // } else { 
      //   for (i = 0; i < cell.nof_ports; i++) {
      //     srslte_refsignal_cs_put_sf(cell, (uint32_t) i, csr_refs.pilots[i / 2][sf_idx], sf_symbols[i]);
      //   }
      // }
      

      // srslte_pcfich_encode(&pcfich, cfi, sf_symbols, sf_idx);

      /* Update DL resource allocation from control port */
      // if (update_control(sf_idx)) {
      //   fprintf(stderr, "Error updating parameters from control port\n");
      // }
      
      #if 0
      /* Transmit PDCCH + PDSCH only when there is data to send */
      if (net_port > 0) {
        send_data = net_packet_ready; 
        if (net_packet_ready) {
          INFO("Transmitting packet\n");
        }
      } else {
        INFO("SF: %d, Generating %d random bits\n", sf_idx, pdsch_cfg.grant.mcs[0].tbs + pdsch_cfg.grant.mcs[1].tbs);
        for (uint32_t tb = 0; tb < SRSLTE_MAX_CODEWORDS; tb++) {
          if (pdsch_cfg.grant.tb_en[tb]) {
            for (i = 0; i < pdsch_cfg.grant.mcs[tb].tbs / 8; i++) {
              data[tb][i] = (uint8_t) rand();
            }
          }
        }
        /* Uncomment this to transmit on sf 0 and 5 only  */
        if (sf_idx != 0 && sf_idx != 5) {
          send_data = true; 
        } else {
          send_data = false;           
        }
      }      
      
      if (send_data) {
        if(sf_idx != 1 || mbsfn_area_id < 0) { // PDCCH + PDSCH
          srslte_dci_format_t dci_format;
          switch(pdsch_cfg.mimo_type) {
            case SRSLTE_MIMO_TYPE_SINGLE_ANTENNA:
              dci_format = SRSLTE_DCI_FORMAT1;
              break;
            case SRSLTE_MIMO_TYPE_TX_DIVERSITY:
            case SRSLTE_MIMO_TYPE_CDD:
              dci_format = SRSLTE_DCI_FORMAT2A;
              break;
            case SRSLTE_MIMO_TYPE_SPATIAL_MULTIPLEX:
              dci_format = SRSLTE_DCI_FORMAT2;
              if (multiplex_nof_layers == 1) {
                ra_dl.pinfo = (uint8_t) (multiplex_pmi + 1);
              } else {
                ra_dl.pinfo = (uint8_t) multiplex_pmi;
              }
              break;
            default:
              fprintf(stderr, "Wrong MIMO configuration\n");
              exit(SRSLTE_ERROR);
          }
          /* Encode PDCCH */
          INFO("Putting DCI to location: n=%d, L=%d\n", locations[sf_idx][0].ncce, locations[sf_idx][0].L);
          srslte_dci_msg_pack_pdsch(&ra_dl, dci_format, &dci_msg, cell.nof_prb, cell.nof_ports, false);
          if (srslte_pdcch_encode(&pdcch, &dci_msg, locations[sf_idx][0], UE_CRNTI, sf_symbols, sf_idx, cfi)) {
            fprintf(stderr, "Error encoding DCI message\n");
            exit(-1);
          }

          /* Configure pdsch_cfg parameters */
          srslte_ra_dl_grant_t grant; 
          srslte_ra_dl_dci_to_grant(&ra_dl, cell.nof_prb, UE_CRNTI, &grant);        
          if (srslte_pdsch_cfg_mimo(&pdsch_cfg, cell, &grant, cfi, sf_idx, rvidx, pdsch_cfg.mimo_type, multiplex_pmi)) {
            fprintf(stderr, "Error configuring PDSCH\n");
            exit(-1);
          }

          /* Encode PDSCH */
          if (srslte_pdsch_encode(&pdsch, &pdsch_cfg, softbuffers, data, UE_CRNTI, sf_symbols)) {
            fprintf(stderr, "Error encoding PDSCH\n");
            exit(-1);
          }        
          if (net_port > 0 && net_packet_ready) {
            if (null_file_sink) {
              for (uint32_t tb = 0; tb < SRSLTE_MAX_CODEWORDS; tb++) {
                srslte_bit_pack_vector(data[tb], data_tmp, pdsch_cfg.grant.mcs[tb].tbs);
                if (srslte_netsink_write(&net_sink, data_tmp, 1 + (pdsch_cfg.grant.mcs[tb].tbs - 1) / 8) < 0) {
                  fprintf(stderr, "Error sending data through UDP socket\n");
                }
              }
            }
            net_packet_ready = false; 
            sem_post(&net_sem);
          }
        }else{ // We're sending MCH on subframe 1 - PDCCH + PMCH

          /* Encode PDCCH */
          INFO("Putting DCI to location: n=%d, L=%d\n", locations[sf_idx][0].ncce, locations[sf_idx][0].L);
          srslte_dci_msg_pack_pdsch(&ra_dl, SRSLTE_DCI_FORMAT1, &dci_msg, cell.nof_prb, cell.nof_ports, false);
          if (srslte_pdcch_encode(&pdcch, &dci_msg, locations[sf_idx][0], M_CRNTI, sf_symbols, sf_idx, cfi)) {
              fprintf(stderr, "Error encoding DCI message\n");
              exit(-1);
          }
          /* Configure pmch_cfg parameters */
          srslte_ra_dl_grant_t grant;
          grant.tb_en[0] = true;
          grant.tb_en[1] = false;
          grant.mcs[0].idx = 2;
          grant.mcs[0].mod = SRSLTE_MOD_QPSK;
          grant.nof_prb = cell.nof_prb;
          grant.sf_type = SRSLTE_SF_MBSFN;
          grant.Qm[0] = srslte_mod_bits_x_symbol(grant.mcs[0].mod);
          srslte_dl_fill_ra_mcs(&grant.mcs[0], cell.nof_prb);
          for(int i = 0; i < 2; i++){
            for(int j = 0; j < grant.nof_prb; j++){
              grant.prb_idx[i][j] = true;
            }
          }
          

          if (srslte_pmch_cfg(&pmch_cfg, cell, &grant, cfi, sf_idx)) {
            fprintf(stderr, "Error configuring PMCH\n");
            exit(-1);
          }
          /* Encode PMCH */
          if (srslte_pmch_encode(&pmch, &pmch_cfg, softbuffers[0], data[0], mbsfn_area_id, sf_symbols)) {
            fprintf(stderr, "Error encoding PDSCH\n");
            exit(-1);
          }
          if (net_port > 0 && net_packet_ready) {
            if (null_file_sink) {
              srslte_bit_pack_vector(data[0], data_tmp, pmch_cfg.grant.mcs[0].tbs);
              if (srslte_netsink_write(&net_sink, data_tmp, 1+(pmch_cfg.grant.mcs[0].tbs-1)/8) < 0) {
                fprintf(stderr, "Error sending data through UDP socket\n");
              }
            }
            net_packet_ready = false;
            sem_post(&net_sem);
          }
        }
      }
      #endif

      #if 0
      // add noise in f-domain
      float scaling = 1.0;
      float snr = 10.0;
      float std_dev = powf(10, - (snr + 3.0f) / 20.0f) * scaling;

      cf_t* noise = srslte_vec_malloc(sizeof(cf_t) * sf_n_re);
      if (!noise) {
        perror("malloc");
        exit(-1);
      }
      bzero(noise, sizeof(cf_t) * sf_n_re);

      srslte_ch_awgn_c(noise, noise, std_dev, sf_n_re);

      srslte_vec_sub_ccc(sf_symbols[0], noise, sf_symbols[0], sf_n_re);
      #endif


      /* Transform to OFDM symbols */
      if(sf_idx != 1 || mbsfn_area_id < 0){
        for (i = 0; i < cell.nof_ports; i++) {
          srslte_ofdm_tx_sf(&ifft[i]);
        }
      }else{
        srslte_ofdm_tx_sf(&ifft_mbsfn);
      }
      
      /* send to file or usrp */
      if (output_file_name) {
        if (!null_file_sink) {
          /* Apply AWGN */
          if (output_file_snr != +INFINITY) {
            float var = powf(10.0f, -(output_file_snr + 3.0f) / 20.0f);
            for (int k = 0; k < cell.nof_ports; k++) {
              srslte_ch_awgn_c(output_buffer[k], output_buffer[k], var, sf_n_samples);
            }
          }
          srslte_filesink_write_multi(&fsink, (void**) output_buffer, sf_n_samples, cell.nof_ports);
        }
        usleep(1000);
      } else {
// #ifndef DISABLE_RF
//       // float norm_factor = (float) cell.nof_prb/15/sqrtf(pdsch_cfg.grant.nof_prb);
//       // for (i = 0; i < cell.nof_ports; i++) {
//       //   srslte_vec_sc_prod_cfc(output_buffer[i], rf_amp * norm_factor, output_buffer[i], SRSLTE_SF_LEN_PRB(cell.nof_prb));
//       // }
//       srslte_rf_send_multi(&rf, (void**) output_buffer, sf_n_samples, true, start_of_burst, false);
//       start_of_burst=false;
// #endif
      }
    }
    nf++;
    sfn = (sfn + 1) % 1024;
  }

  base_free();

  printf("Done\n");
  exit(0);
}


