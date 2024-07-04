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
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <srslte/phy/common/phy_common.h>
#include "srslte/phy/io/filesink.h"

#include "srslte/srslte.h"
#include "srslte/common/crash_handler.h"

#include "srslte/phy/ue_sl/ue_sl_mib.h"

#define ENABLE_AGC_DEFAULT


//#define STDOUT_COMPACT

char *output_file_name;
// #define PRINT_CHANGE_SCHEDULIGN

//#define CORRECT_SAMPLE_OFFSET

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


/**********************************************************************
 *  Program arguments processing
 ***********************************************************************/
typedef struct {
  int nof_subframes;
  int cpu_affinity;
  bool enable_debug_dumps;
  bool disable_cfo; 
  uint32_t time_offset; 
  int force_N_id_2;
  uint16_t rnti;
  char *input_file_name;
  char *scibits_file_name;
  int file_offset_time; 
  float file_offset_freq;
  uint32_t file_nof_prb;
  uint32_t file_nof_ports;
  uint32_t file_cell_id;
  bool file_lte_rate;
  bool enable_cfo_ref;
  bool average_subframe;
  char *rf_args; 
  uint32_t rf_nof_rx_ant; 
  double rf_freq; 
  float rf_gain;
  int net_port; 
  char *net_address; 
  int net_port_signal; 
  char *net_address_signal;
  int decimate;
  int32_t mbsfn_area_id;
  uint8_t  non_mbsfn_region;
  int verbose;
  bool decode_psbch;
  uint8_t smooth_filter_len;
}prog_args_t;

void args_default(prog_args_t *args) {
  args->enable_debug_dumps = false;
  args->nof_subframes = -1;
  args->rnti = SRSLTE_SIRNTI;
  args->force_N_id_2 = -1; // Pick the best
  args->input_file_name = NULL;
  args->scibits_file_name = NULL;
  args->disable_cfo = false; 
  args->time_offset = 0; 
  args->file_nof_prb = 25; 
  args->file_nof_ports = 1; 
  args->file_cell_id = 0; 
  args->file_offset_time = 0; 
  args->file_offset_freq = 0; 
  args->file_lte_rate = false;
  args->rf_args = "";
  args->rf_freq = -1.0;
  args->rf_nof_rx_ant = 1;
  args->enable_cfo_ref = false;
  args->average_subframe = false;
#ifdef ENABLE_AGC_DEFAULT
  args->rf_gain = -1.0; 
#else
  args->rf_gain = 50.0; 
#endif
  args->net_port = -1; 
  args->net_address = "127.0.0.1";
  args->net_port_signal = -1; 
  args->net_address_signal = "127.0.0.1";
  args->decimate = 0;
  args->cpu_affinity = -1;
  args->mbsfn_area_id = -1;
  args->non_mbsfn_region = 2;
  args->decode_psbch = false;
  args->smooth_filter_len = 7;
}

void usage(prog_args_t *args, char *prog) {
  printf("Usage: %s [agpPoOcildFRDnruMNv] -f rx_frequency (in Hz) | -i input_file\n", prog);
#ifndef DISABLE_RF
  printf("\t-a RF args [Default %s]\n", args->rf_args);
  printf("\t-A Number of RX antennas [Default %d]\n", args->rf_nof_rx_ant);
#ifdef ENABLE_AGC_DEFAULT
  printf("\t-g RF fix RX gain [Default AGC]\n");
#else
  printf("\t-g Set RX gain [Default %.1f dB]\n", args->rf_gain);
#endif  
#else
  printf("\t   RF is disabled.\n");
#endif
  printf("\t-i input_file [Default use RF board]\n");
  printf("\t-o offset frequency correction (in Hz) for input file [Default %.1f Hz]\n", args->file_offset_freq);
  printf("\t-O offset samples for input file [Default %d]\n", args->file_offset_time);
  printf("\t-p nof_prb for input file [Default %d]\n", args->file_nof_prb);
  printf("\t-P nof_ports for input file [Default %d]\n", args->file_nof_ports);
  printf("\t-c cell_id for input file [Default %d]\n", args->file_cell_id);
  printf("\t-L use LTE sampling rate on file(instead of 3/4) [Default %d]\n", args->file_lte_rate);
  printf("\t-r RNTI in Hex [Default 0x%x]\n",args->rnti);
  printf("\t-l Force N_id_2 [Default best]\n");
  printf("\t-C Disable CFO correction [Default %s]\n", args->disable_cfo?"Disabled":"Enabled");
  printf("\t-F Enable RS-based CFO correction [Default %s]\n", !args->enable_cfo_ref?"Disabled":"Enabled");
  printf("\t-R Average channel estimates on 1 ms [Default %s]\n", !args->average_subframe?"Disabled":"Enabled");
  printf("\t-t Add time offset [Default %d]\n", args->time_offset);
  printf("\t-D Enable dumping of various debug vectors[Default %s]\n", args->enable_debug_dumps?"Disabled":"Enabled");
  printf("\t-y set the cpu affinity mask [Default %d] \n  ",args->cpu_affinity);
  printf("\t-n nof_subframes [Default %d]\n", args->nof_subframes);
  printf("\t-s frequency channel smooth filter length, must be odd [Default %d]\n", args->smooth_filter_len);
  printf("\t-S SCI-Bits filename, which indicates, no pscch decoding is required\n");
  printf("\t-u remote TCP port to send data (-1 does nothing with it) [Default %d]\n", args->net_port);
  printf("\t-U remote TCP address to send data [Default %s]\n", args->net_address);
  printf("\t-M MBSFN area id [Default %d]\n", args->mbsfn_area_id);
  printf("\t-N Non-MBSFN region [Default %d]\n", args->non_mbsfn_region);
  printf("\t-B Enable PSBCH decoding [Default %d]\n", args->decode_psbch);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(prog_args_t *args, int argc, char **argv) {
  int opt;
  args_default(args);
  while ((opt = getopt(argc, argv, "aABoglLipPcOCtdDFRnvrfuUsSZyWMN")) != -1) {
    switch (opt) {
    case 'B':
      args->decode_psbch = true;
      break;
    case 'i':
      args->input_file_name = argv[optind];
      break;
    case 'S':
      args->scibits_file_name = argv[optind];
      break;
    case 'p':
      args->file_nof_prb = atoi(argv[optind]);
      break;
    case 'P':
      args->file_nof_ports = atoi(argv[optind]);
      break;
    case 'o':
      args->file_offset_freq = atof(argv[optind]);
      break;
    case 'O':
      args->file_offset_time = atoi(argv[optind]);
      break;
    case 'c':
      args->file_cell_id = atoi(argv[optind]);
      break;
    case 'L':
      args->file_lte_rate = true;
      break;
    case 'a':
      args->rf_args = argv[optind];
      break;
    case 'A':
      args->rf_nof_rx_ant = atoi(argv[optind]);
      break;
    case 'g':
      args->rf_gain = atof(argv[optind]);
      break;
    case 'C':
      args->disable_cfo = true;
      break;
    case 'F':
      args->enable_cfo_ref = true;
      break;
    case 'R':
      args->average_subframe = true;
      break;
    case 't':
      args->time_offset = atoi(argv[optind]);
      break;
    case 'f':
      args->rf_freq = strtod(argv[optind], NULL);
      break;
    case 'n':
      args->nof_subframes = atoi(argv[optind]);
      break;
    case 'r':
      args->rnti = strtol(argv[optind], NULL, 16);
      break;
    case 'l':
      args->force_N_id_2 = atoi(argv[optind]);
      break;
    case 'u':
      args->net_port = atoi(argv[optind]);
      break;
    case 'U':
      args->net_address = argv[optind];
      break;
    case 's':
      args->smooth_filter_len = atoi(argv[optind]);
      break;
    case 'D':
      args->enable_debug_dumps = true;
      break;
    case 'v':
      srslte_verbose++;
      args->verbose = srslte_verbose;
      break;
    case 'Z':
      args->decimate = atoi(argv[optind]);
      break;
    case 'y':
      args->cpu_affinity = atoi(argv[optind]);
      break;
    case 'W':
      output_file_name = argv[optind];
      break;
    case 'M':
      args->mbsfn_area_id = atoi(argv[optind]);
      break;
    case 'N':
      args->non_mbsfn_region = atoi(argv[optind]);
      break;
    default:
      usage(args, argv[0]);
      exit(-1);
    }
  }
  if (args->rf_freq < 0 && args->input_file_name == NULL) {
    usage(args, argv[0]);
    exit(-1);
  }
}
/**********************************************************************/

/* TODO: Do something with the output data */
uint8_t *data[SRSLTE_MAX_CODEWORDS];

bool go_exit = false; 
void sig_int_handler(int signo)
{
  printf("SIGINT received. Exiting...\n");
  if (signo == SIGINT) {
    go_exit = true;
  } else if (signo == SIGSEGV) {
    exit(1);
  }
}

cf_t *sf_buffer[SRSLTE_MAX_PORTS] = {NULL};
uint32_t sf_buffer_max_num = 0;



extern float mean_exec_time;

enum receiver_state { DECODE_MIB, DECODE_PDSCH} state; 

srslte_ue_sl_sync_t ue_sl_sync;
srslte_ue_sl_mib_t ue_sl_mib;
prog_args_t prog_args; 

srslte_repo_t repo;

uint32_t sfn = 0; // system frame number
srslte_netsink_t net_sink, net_sink_signal;
/* Useful macros for printing lines which will disappear */

#define PRINT_LINE_INIT() int this_nof_lines = 0; static int prev_nof_lines = 0
#define PRINT_LINE(_fmt, ...) printf("\033[K" _fmt "\n", ##__VA_ARGS__); this_nof_lines++
#define PRINT_LINE_RESET_CURSOR() printf("\033[%dA", this_nof_lines); prev_nof_lines = this_nof_lines
#define PRINT_LINE_ADVANCE_CURSOR() printf("\033[%dB", prev_nof_lines + 1)

int main(int argc, char **argv) {
  struct timeval t[3];
  int ret;
  int decimate = 1;
  srslte_cell_t cell;  
  int64_t sf_cnt;
  
  uint32_t nof_trials = 0; 
  int sfn_offset;
  float cfo = 0; 

  srslte_debug_handle_crash(argc, argv);

  parse_args(&prog_args, argc, argv);

  // set lte symbol size
  srslte_use_standard_symbol_size(prog_args.file_lte_rate);

  for (int i = 0; i< SRSLTE_MAX_CODEWORDS; i++) {
    data[i] = srslte_vec_malloc(sizeof(uint8_t)*1500*8);
    if (!data[i]) {
      ERROR("Allocating data");
      go_exit = true;
    }
  }

  
  if(prog_args.cpu_affinity > -1) {
    
    cpu_set_t cpuset;
    pthread_t thread;
    
    thread = pthread_self();
    for(int i = 0; i < 8;i++){
      if(((prog_args.cpu_affinity >> i) & 0x01) == 1){
        printf("Setting pdsch_ue with affinity to core %d\n", i);
        CPU_SET((size_t) i , &cpuset);        
      }
      if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset)){
      fprintf(stderr, "Error setting main thread affinity to %d \n", prog_args.cpu_affinity);
      exit(-1);
      }
    }    
  }
  
  if (prog_args.net_port > 0) {
    if (srslte_netsink_init(&net_sink, prog_args.net_address, prog_args.net_port, SRSLTE_NETSINK_TCP)) {
      fprintf(stderr, "Error initiating UDP socket to %s:%d\n", prog_args.net_address, prog_args.net_port);
      exit(-1);
    }
    srslte_netsink_set_nonblocking(&net_sink);
  }
  if (prog_args.net_port_signal > 0) {
    if (srslte_netsink_init(&net_sink_signal, prog_args.net_address_signal, 
      prog_args.net_port_signal, SRSLTE_NETSINK_UDP)) {
      fprintf(stderr, "Error initiating UDP socket to %s:%d\n", prog_args.net_address_signal, prog_args.net_port_signal);
      exit(-1);
    }
    srslte_netsink_set_nonblocking(&net_sink_signal);
  }
  
  /* If reading from file, go straight to PDSCH decoding. Otherwise, decode MIB first */
  if (prog_args.input_file_name) {
    /* preset cell configuration */
    cell.id = prog_args.file_cell_id; 
    cell.cp = SRSLTE_CP_NORM; 
    cell.phich_length = SRSLTE_PHICH_NORM;
    cell.phich_resources = SRSLTE_PHICH_R_1;
    cell.nof_ports = prog_args.file_nof_ports; 
    cell.nof_prb = prog_args.file_nof_prb; 
    
    if (srslte_ue_sl_sync_init_file_multi(&ue_sl_sync, prog_args.file_nof_prb, 
      prog_args.input_file_name, prog_args.file_offset_time, prog_args.file_offset_freq, prog_args.rf_nof_rx_ant)) {
      fprintf(stderr, "Error initiating ue_sync\n");
      exit(-1); 
    }
    if (srslte_ue_sl_sync_set_cell(&ue_sl_sync, cell))
    {
      fprintf(stderr, "Error initiating ue_sl_sync\n");
      exit(-1);
    }
  }

  sf_buffer_max_num = 3 * SRSLTE_SF_LEN_PRB(cell.nof_prb);
  for (int i=0;i<prog_args.rf_nof_rx_ant;i++) {
    sf_buffer[i] = srslte_vec_cf_malloc(sf_buffer_max_num);
  }
  if (srslte_ue_sl_mib_init(&ue_sl_mib, sf_buffer, cell.nof_prb)) {
    fprintf(stderr, "Error initaiting UE MIB decoder\n");
    exit(-1);
  }
  if (srslte_ue_sl_mib_set_cell(&ue_sl_mib, cell)) {
    fprintf(stderr, "Error initaiting UE MIB decoder\n");
    exit(-1);
  }

  if (srslte_repo_init(&repo, cell, respool)) {
    fprintf(stderr, "Error creating Ressource Pool object\n");
    exit(-1);
  }


  // Disable CP based CFO estimation during find
  ue_sl_sync.cfo_current_value = cfo/15000;
  ue_sl_sync.cfo_is_copied = true;
  ue_sl_sync.cfo_correct_enable_find = true;
  srslte_sync_sl_set_cfo_cp_enable(&ue_sl_sync.sfind, false, 0);

  /* Initialize subframe counter */
  sf_cnt = 0;


  // Variables for measurements 
  uint32_t nframes=0;
  uint8_t ri = 0, pmi = 0;
  float rsrp0=0.0, rsrp1=0.0, rsrq=0.0, noise=0.0, enodebrate = 0.0, uerate = 0.0, procrate = 0.0,
      sinr[SRSLTE_MAX_LAYERS][SRSLTE_MAX_CODEBOOKS], cn = 0.0;
  bool decode_pdsch = false; 

  for (int i = 0; i < SRSLTE_MAX_LAYERS; i++) {
    bzero(sinr[i], sizeof(float)*SRSLTE_MAX_CODEBOOKS);
  }

#ifdef PRINT_CHANGE_SCHEDULIGN
  srslte_ra_dl_dci_t old_dl_dci; 
  bzero(&old_dl_dci, sizeof(srslte_ra_dl_dci_t));
#endif

  ue_sl_sync.cfo_correct_enable_track = !prog_args.disable_cfo;

  // change width of smooth filter
  ue_sl_mib.chest.smooth_filter_len = prog_args.smooth_filter_len;//5;//3;
  srslte_chest_set_rect_filter(ue_sl_mib.chest.smooth_filter, ue_sl_mib.chest.smooth_filter_len);

  // read in optional sci bits
  uint8_t sci_assumed[SRSLTE_SCI1_MAX_BITS];
  srslte_filesource_t scibits_file;

  if(prog_args.scibits_file_name != NULL) {
    srslte_filesource_init(&scibits_file, prog_args.scibits_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
  }

  INFO("\nEntering main loop...\n\n");
  /* Main loop */
  while (!go_exit && (sf_cnt < prog_args.nof_subframes || prog_args.nof_subframes == -1)) {
    bool acks [SRSLTE_MAX_CODEWORDS] = {false};
    char input[128];
    PRINT_LINE_INIT();

    fd_set set;
    FD_ZERO(&set);
    FD_SET(0, &set);

    struct timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;

    /* Set default verbose level */
    srslte_verbose = prog_args.verbose;
    int n = select(1, &set, NULL, NULL, &to);
    if (n == 1) {
      /* If a new line is detected set verbose level to Debug */
      if (fgets(input, sizeof(input), stdin)) {
        srslte_verbose = SRSLTE_VERBOSE_DEBUG;
        nof_trials = 0;
      }
    }

    // move samples into CP
    cf_t *sf_buffer_n[SRSLTE_MAX_PORTS] = {NULL};
    sf_buffer_n[0] = sf_buffer[0];// + SRSLTE_CP_LEN(srslte_symbol_sz(cell.nof_prb), SRSLTE_CP_NORM_LEN) / 8;

    ret = srslte_ue_sl_sync_zerocopy_multi(&ue_sl_sync, sf_buffer_n, sf_buffer_max_num);
    if (ret < 0) {
      fprintf(stderr, "Error calling srslte_ue_sync_work()\n");
    }

    printf("Getting subframe %d/%d ret: %d\n", sf_cnt, prog_args.nof_subframes, ret);

#ifdef CORRECT_SAMPLE_OFFSET
    float sample_offset = (float) srslte_ue_sync_get_last_sample_offset(&ue_sync)+srslte_ue_sync_get_sfo(&ue_sync)/1000; 
    srslte_ue_dl_set_sample_offset(&ue_dl, sample_offset);
#endif
    
    /* srslte_ue_sync_get_buffer returns 1 if successfully read 1 aligned subframe */
    if (ret == 1) {

      uint32_t sfidx = srslte_ue_sl_sync_get_sfidx(&ue_sl_sync);

      if(prog_args.decode_psbch) {

        printf("Decoding PSBCH\n");

        uint8_t bch_payload[SRSLTE_SL_BCH_PAYLOAD_LEN];
        uint32_t nof_tx_ports = 1;
        int sfn_offset = 0;

        if(SRSLTE_UE_SL_MIB_FOUND == srslte_ue_sl_mib_decode(&ue_sl_mib, bch_payload, &nof_tx_ports, &sfn_offset)) {
          srslte_vec_fprint_byte(stdout, bch_payload, SRSLTE_SL_BCH_PAYLOAD_LEN);

          uint32_t dfn;
          uint32_t dsfn;
          srslte_psbch_mib_unpack(bch_payload, &cell, &dfn, &dsfn);
        }

        printf("PSBCH SNR: %f\n", 10*log10(ue_sl_mib.chest.pilot_power / ue_sl_mib.chest.noise_estimate));

      } else {
        //@todo: cleanup this section, as well as then whole file
        int32_t decoded_bytes = 0;

        if(prog_args.scibits_file_name != NULL) {
          // sci-1 bits are given, we only decode pssch
          size_t read_scibits = fread(&sci_assumed[0], sizeof(uint8_t), SRSLTE_SCI1_MAX_BITS, scibits_file.f);

          if(read_scibits<=0) {
            printf("Failed to read SCI-1 bits from file %s\n", prog_args.scibits_file_name);
            exit(-1);
          }

          srslte_ue_sl_pssch_decode(&ue_sl_mib,
                                    &repo,
                                    sci_assumed,
                                    data[0],
                                    &decoded_bytes);

        } else {
          // always try to decode PSCCH and PSSCH

          // currently we should only the first subframe which has n_PSSCH_ssf = 0
          // which is still hardcoded in srslte_ue_sl_pscch_decode()
          // ue_sl_mib.pssch.n_PSSCH_ssf = sfidx;

          srslte_ue_sl_pscch_decode(&ue_sl_mib,
                                    &repo,
                                    data[0],
                                    &decoded_bytes);

        }

        if(prog_args.enable_debug_dumps) {
          // store some debug informations
          srslte_filesink_t fsink;
          
          srslte_filesink_init(&fsink, "snr_dec_fft_out", SRSLTE_COMPLEX_FLOAT_BIN);
          srslte_filesink_write(&fsink, (void*) ue_sl_mib.fft.out_buffer, cell.nof_prb * SRSLTE_NRE * 14);
          srslte_filesink_free(&fsink);

          srslte_filesink_init(&fsink, "snr_dec_fft_in", SRSLTE_COMPLEX_FLOAT_BIN);
          srslte_filesink_write(&fsink, (void*) ue_sl_mib.fft.in_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(cell.nof_prb)));
          srslte_filesink_free(&fsink);

          srslte_filesink_init(&fsink, "snr_dec_pscch_const", SRSLTE_COMPLEX_FLOAT_BIN);
          srslte_filesink_write(&fsink, (void*) ue_sl_mib.pscch.d, 2 * SRSLTE_NRE * 9);
          srslte_filesink_free(&fsink);

          // we do not know how many symbols we decoded, so we dump all of them
          srslte_filesink_init(&fsink, "snr_dec_pssch_const", SRSLTE_COMPLEX_FLOAT_BIN);
          srslte_filesink_write(&fsink, (void*) ue_sl_mib.pssch.d[0], ue_sl_mib.pssch.max_re);
          srslte_filesink_free(&fsink);

          srslte_filesink_init(&fsink, "snr_dec_ce", SRSLTE_COMPLEX_FLOAT_BIN);
          srslte_filesink_write(&fsink, (void*) ue_sl_mib.ce, cell.nof_prb * SRSLTE_NRE * 14);
          srslte_filesink_free(&fsink);
        }
      }

      if (sfidx == 9) {
        sfn++; 
        if (sfn == 1024) {
          sfn = 0; 
          PRINT_LINE_ADVANCE_CURSOR();
        } 
      }
      
    } else if (ret == 0) {
      printf("Finding PSS... Peak: %8.1f, FrameCnt: %d, State: %d\r", 
        srslte_sync_sl_get_peak_value(&ue_sl_sync.sfind), 
        ue_sl_sync.frame_total_cnt, ue_sl_sync.state);      
    }
        
    sf_cnt++;                  
  } // Main loop
  
  srslte_ue_sl_mib_free(&ue_sl_mib);
  srslte_ue_sl_sync_free(&ue_sl_sync);
  for (int i = 0; i < SRSLTE_MAX_CODEWORDS; i++) {
    if (data[i]) {
      free(data[i]);
    }
  }
  for (int i = 0; i < prog_args.rf_nof_rx_ant; i++) {
    if (sf_buffer[i]) {
      free(sf_buffer[i]);
    }
  }

  printf("\nBye\n");
  exit(0);
}
