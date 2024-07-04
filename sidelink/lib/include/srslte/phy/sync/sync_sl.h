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

/******************************************************************************
 *  File:         sync_sl.h
 *
 *  Description:  Time and frequency synchronization using the PSSS and SSSS signals.
 *
 *                The object is designed to work with signals sampled at 1.92 Mhz
 *                centered at the carrier frequency. Thus, downsampling is required
 *                if the signal is sampled at higher frequencies.
 *
 *                Correlation peak is detected comparing the maximum at the output
 *                of the correlator with a threshold. The comparison accepts two
 *                modes: absolute value or peak-to-mean ratio, which are configured
 *                with the functions sync_pss_det_absolute() and sync_pss_det_peakmean().
 *
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10 Sec. 6.11.1, 6.11.2
 *****************************************************************************/

#ifndef SRSLTE_SYNC_SL_H
#define SRSLTE_SYNC_SL_H

#include <stdbool.h>
#include <math.h>

#include "srslte/config.h"
#include "srslte/phy/sync/psss.h"
#include "srslte/phy/sync/ssss.h"
#include "srslte/phy/sync/cfo.h"
#include "srslte/phy/sync/cp.h"

#define SRSLTE_SYNC_FFT_SZ_MIN    64
#define SRSLTE_SYNC_FFT_SZ_MAX    2048

//typedef enum {SSS_DIFF=0, SSS_PARTIAL_3=2, SSS_FULL=1} sss_alg_t; 

typedef struct SRSLTE_API {
  srslte_psss_t psss;
  srslte_psss_t psss_i[2];
  srslte_ssss_t ssss;
  // srslte_cp_synch_t cp_synch;
  // cf_t *cfo_i_corr[2];
  int decimate;

  // psss corrlation 
  float threshold;
  float peak_value;

  uint32_t N_id_2;
  uint32_t N_id_1;
  uint32_t sf_idx;
  uint32_t fft_size;
  uint32_t frame_size;
  uint32_t max_offset;
  uint32_t nof_symbols;
  uint32_t cp_len;
  float current_cfo_tol;
  sss_alg_t ssss_alg; 

  // no cp detection, as in sl miode 4
  //bool detect_cp;
  srslte_cp_t cp;

  // SSSS
  bool ssss_en;
  uint32_t m0;
  uint32_t m1;
  float m0_value;
  float m1_value;

  // CP detection
  float M_norm_avg; 
  float M_ext_avg; 

  // cf_t  *temp;

  uint32_t max_frame_size;


  // // variables for various CFO estimation methods
  bool cfo_cp_enable;
  bool cfo_pss_enable;
  bool cfo_i_enable;

  bool cfo_cp_is_set;
  bool cfo_psss_is_set;
  // bool cfo_i_initiated;

  float cfo_cp_mean;
  float cfo_psss;
  float cfo_psss_mean;
  // int   cfo_i_value;

  float cfo_ema_alpha;

  uint32_t cfo_cp_nsymbols;

  srslte_cfo_t cfo_corr_frame;
  srslte_cfo_t cfo_corr_symbol;

  bool ssss_channel_equalize;
  bool psss_filtering_enabled;
  cf_t ssss_filt[SRSLTE_SYMBOL_SZ_MAX];
  cf_t psss_filt[SRSLTE_SYMBOL_SZ_MAX];

}srslte_sync_sl_t;

// typedef enum {
//   SRSLTE_SYNC_FOUND = 1, 
//   SRSLTE_SYNC_FOUND_NOSPACE = 2, 
//   SRSLTE_SYNC_NOFOUND = 0, 
//   SRSLTE_SYNC_ERROR = -1  
// } srslte_sync_find_ret_t; 


SRSLTE_API int srslte_sync_sl_init(srslte_sync_sl_t *q, 
                                uint32_t frame_size, 
                                uint32_t max_offset,
                                uint32_t fft_size);

SRSLTE_API int srslte_sync_sl_init_decim(srslte_sync_sl_t *q, 
                                uint32_t frame_size, 
                                uint32_t max_offset,
                                uint32_t fft_size,
                                int decimate);


SRSLTE_API void srslte_sync_sl_free(srslte_sync_sl_t *q);

SRSLTE_API int srslte_sync_sl_resize(srslte_sync_sl_t *q,
                                  uint32_t frame_size,
                                  uint32_t max_offset,
                                  uint32_t fft_size);

SRSLTE_API void srslte_sync_sl_reset(srslte_sync_sl_t *q); 

/* Finds a correlation peak in the input signal around position find_offset */
SRSLTE_API srslte_sync_find_ret_t srslte_sync_sl_find(srslte_sync_sl_t *q, 
                                                      const cf_t *input,
                                                      uint32_t find_offset,
                                                      uint32_t *peak_position);

#if 0
/* Estimates the CP length */
SRSLTE_API srslte_cp_t srslte_sync_detect_cp(srslte_sync_sl_t *q, 
                                             const cf_t *input,
                                             uint32_t peak_pos);
#endif

/* Sets the threshold for peak comparison */
SRSLTE_API void srslte_sync_sl_set_threshold(srslte_sync_sl_t *q, 
                                              float threshold);

/* Gets the subframe idx (0 or 5) */
SRSLTE_API uint32_t srslte_sync_sl_get_sf_idx(srslte_sync_sl_t *q);

/* Gets the peak value */
SRSLTE_API float srslte_sync_sl_get_peak_value(srslte_sync_sl_t *q);
#if 0
/* Choose SSS detection algorithm */
SRSLTE_API void srslte_sync_set_sss_algorithm(srslte_sync_sl_t *q, 
                                              sss_alg_t alg); 
#endif
/* Sets PSS exponential averaging alpha weight */
SRSLTE_API void srslte_sync_sl_set_em_alpha(srslte_sync_sl_t *q, 
                                            float alpha);

/* Sets the N_id_2 to search for */
SRSLTE_API int srslte_sync_sl_set_N_id_2(srslte_sync_sl_t *q, 
                                          uint32_t N_id_2);

/* Gets the Physical CellId from the last call to synch_run() */
SRSLTE_API int srslte_sync_sl_get_cell_id(srslte_sync_sl_t *q);

/* Enables/disables filtering of the central PRBs before PSS CFO estimation or SSS correlation*/
SRSLTE_API void srslte_sync_sl_set_psss_filt_enable(srslte_sync_sl_t *q,
                                                    bool enable);

SRSLTE_API void srslte_sync_sl_set_ssss_eq_enable(srslte_sync_sl_t *q,
                                                  bool enable);

/* Gets the CFO estimation from the last call to synch_run() */
SRSLTE_API float srslte_sync_sl_get_cfo(srslte_sync_sl_t *q);

/* Resets internal CFO state */
SRSLTE_API void srslte_sync_sl_cfo_reset(srslte_sync_sl_t *q);

#if 0
/* Copies CFO internal state from another object to avoid long transients */
SRSLTE_API void srslte_sync_copy_cfo(srslte_sync_sl_t *q,
                                     srslte_sync_sl_t *src_obj);
#endif
/* Enable different CFO estimation stages */
SRSLTE_API void srslte_sync_sl_set_cfo_i_enable(srslte_sync_sl_t *q,
                                                bool enable);

SRSLTE_API void srslte_sync_sl_set_cfo_cp_enable(srslte_sync_sl_t *q,
                                                  bool enable,
                                                  uint32_t nof_symbols);

SRSLTE_API void srslte_sync_sl_set_cfo_psss_enable(srslte_sync_sl_t *q,
                                                    bool enable);

/* Sets CFO correctors tolerance (in Hz) */
SRSLTE_API void srslte_sync_sl_set_cfo_tol(srslte_sync_sl_t *q,
                                            float tol);

/* Sets the exponential moving average coefficient for CFO averaging */
SRSLTE_API void srslte_sync_sl_set_cfo_ema_alpha(srslte_sync_sl_t *q, 
                                                  float alpha);

/* Gets the CP length estimation from the last call to synch_run() */
SRSLTE_API srslte_cp_t srslte_sync_sl_get_cp(srslte_sync_sl_t *q);

/* Sets the CP length estimation (must do it if disabled) */
SRSLTE_API void srslte_sync_sl_set_cp(srslte_sync_sl_t *q, 
                                      srslte_cp_t cp);

/* Enables/Disables SSS detection  */
SRSLTE_API void srslte_sync_sl_ssss_en(srslte_sync_sl_t *q, 
                                        bool enabled);

SRSLTE_API srslte_psss_t* srslte_sync_sl_get_cur_psss_obj(srslte_sync_sl_t *q);

SRSLTE_API bool srslte_sync_sl_ssss_detected(srslte_sync_sl_t *q);

// /* Enables/Disables CP detection  */
// SRSLTE_API void srslte_sync_sl_cp_en(srslte_sync_sl_t *q, 
//                                   bool enabled);

#endif // SRSLTE_SYNC_SL_H

