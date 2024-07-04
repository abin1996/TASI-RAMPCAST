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
 *  File:         pss.h
 *
 *  Description:  Primary sidelink synchronization signal (PSSS) generation and detection.
 *
 *                The srslte_psss_t object provides functions for fast
 *                computation of the crosscorrelation between the PSS and received
 *                signal and CFO estimation. Also, the function srslte_psss_tperiodic()
 *                is designed to be called periodically every subframe, taking
 *                care of the correct data alignment with respect to the PSS sequence.
 *
 *                The object is designed to work with signals sampled at 1.92 Mhz
 *                centered at the carrier frequency. Thus, downsampling is required
 *                if the signal is sampled at higher frequencies.
 *
 *  Reference:    3GPP TS 36.211 version 14.2.0 Release 14 Sec. 9.7.1
 *****************************************************************************/

#ifndef SRSLTE_PSSS_H
#define SRSLTE_PSSS_H

#include <stdint.h>
#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/utils/convolution.h"
#include "srslte/phy/utils/filter.h"

#define CONVOLUTION_FFT

#define SRSLTE_PSSS_LEN     62
#define SRSLTE_PSSS_RE      (6*12)


/* PSS processing options */

#define SRSLTE_PSSS_ACCUMULATE_ABS   // If enabled, accumulates the correlation absolute value on consecutive calls to _srslte_psss_find_pss

#define SRSLTE_PSSS_RETURN_PSR  // If enabled returns peak to side-lobe ratio, otherwise returns absolute peak value


/* Low-level API */
typedef struct SRSLTE_API {

#ifdef CONVOLUTION_FFT
  srslte_conv_fft_cc_t conv_fft;
  srslte_filt_cc_t filter;

#endif
  int decimate;

  uint32_t max_frame_size;
  uint32_t max_fft_size;

  uint32_t frame_size;
  uint32_t N_id_2;
  uint32_t fft_size;
  cf_t *psss_signal_freq_full[2];

  cf_t *psss_signal_time[2];
  cf_t *psss_signal_time_scale[2];

  cf_t psss_signal_freq[2][SRSLTE_PSSS_LEN]; // One sequence for each N_id_2
  cf_t *tmp_input;
  cf_t *conv_output;
  float *conv_output_abs;
  float ema_alpha;
  float *conv_output_avg;
  float peak_value;

  bool filter_psss_enable;
  srslte_dft_plan_t dftp_input;
  srslte_dft_plan_t idftp_input;
  cf_t tmp_fft[SRSLTE_SYMBOL_SZ_MAX];
  cf_t tmp_fft2[SRSLTE_SYMBOL_SZ_MAX];

  cf_t tmp_ce[SRSLTE_PSSS_LEN];

  bool chest_on_filter;

}srslte_psss_t;

typedef enum { PSSS_TX, PSSS_RX } psss_direction_t;

/* Basic functionality */
SRSLTE_API int srslte_psss_init_fft(srslte_psss_t *q,
                                         uint32_t frame_size, 
                                         uint32_t fft_size);

SRSLTE_API int srslte_psss_init_fft_offset(srslte_psss_t *q,
                                                uint32_t frame_size, 
                                                uint32_t fft_size, 
                                                int cfo_i);

SRSLTE_API int srslte_psss_init_fft_offset_decim(srslte_psss_t *q,
                                                uint32_t frame_size, 
                                                uint32_t fft_size, 
                                                int cfo_i,
                                                int decimate);

SRSLTE_API int srslte_psss_resize(srslte_psss_t *q, uint32_t frame_size,
                                       uint32_t fft_size,
                                       int offset);

SRSLTE_API int srslte_psss_init(srslte_psss_t *q,
                                     uint32_t frame_size);

SRSLTE_API void srslte_psss_free(srslte_psss_t *q);

SRSLTE_API void srslte_psss_reset(srslte_psss_t *q);

SRSLTE_API void _srslte_psss_filter_enable(srslte_psss_t *q,
                                               bool enable);

SRSLTE_API void _srslte_psss_sic(srslte_psss_t *q,
                                     cf_t *input);

SRSLTE_API void srslte_psss_filter(srslte_psss_t *q,
                                        const cf_t *input,
                                        cf_t *output);

SRSLTE_API int srslte_psss_generate(cf_t *signal, 
                                   uint32_t N_id_2);

SRSLTE_API void _srslte_psss_get_slot(cf_t *slot, 
                                    cf_t *pss_signal, 
                                    uint32_t nof_prb, 
                                    srslte_cp_t cp); 

SRSLTE_API void srslte_psss_put_sf(cf_t *psss_signal, 
                                    cf_t *sf, 
                                    uint32_t nof_prb, 
                                    srslte_cp_t cp);

SRSLTE_API void srslte_psss_set_ema_alpha(srslte_psss_t *q,
                                          float alpha); 

SRSLTE_API int srslte_psss_set_N_id_2(srslte_psss_t *q,
                                           uint32_t N_id_2);

SRSLTE_API int srslte_psss_find_psss(srslte_psss_t *q,
                                         const cf_t *input,
                                         float *corr_peak_value);

SRSLTE_API int _srslte_psss_chest(srslte_psss_t *q,
                                      const cf_t *input,
                                      cf_t ce[SRSLTE_PSSS_LEN]); 

SRSLTE_API float srslte_psss_cfo_compute(srslte_psss_t* q,
                                              const cf_t *pss_recv);

SRSLTE_API float srslte_psss_cfo_compute_with_psss(srslte_psss_t* q,
                                                        const cf_t *subframe);

SRSLTE_API float srslte_psss_cfo_compute_with_ssss(srslte_psss_t* q,
                                                        const cf_t *subframe);

#endif // SRSLTE_PSSS_H
