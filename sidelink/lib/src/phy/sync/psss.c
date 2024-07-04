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


#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>

#include "srslte/phy/sync/psss.h"
#include "srslte/phy/utils/debug.h"


int srslte_psss_init_N_id_2(cf_t *psss_signal_freq, cf_t *psss_signal_time,
                            uint32_t N_id_2, uint32_t fft_size, int cfo_i) {
  srslte_dft_plan_t plan;
  cf_t psss_signal_pad[2048];
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  
  // @todo: are we limited to 1024?
  if (srslte_SL_N_id_2_isvalid(N_id_2)    && 
      fft_size                  <= 2048) 
  {
    
    srslte_psss_generate(psss_signal_freq, N_id_2);

    bzero(psss_signal_pad, fft_size * sizeof(cf_t));
    bzero(psss_signal_time, fft_size * sizeof(cf_t));
    memcpy(&psss_signal_pad[(fft_size-SRSLTE_PSSS_LEN)/2+cfo_i], psss_signal_freq, SRSLTE_PSSS_LEN * sizeof(cf_t));

    /* Convert signal into the time domain */    
    if (srslte_dft_plan(&plan, fft_size, SRSLTE_DFT_BACKWARD, SRSLTE_DFT_COMPLEX)) {
      return SRSLTE_ERROR;
    }
    
    srslte_dft_plan_set_mirror(&plan, true);
    srslte_dft_plan_set_dc(&plan, false); // in sidelink we use DC
    srslte_dft_plan_set_norm(&plan, true);
    srslte_dft_run_c(&plan, psss_signal_pad, psss_signal_time);

    // this may be only required for using fft to do pss correlation
    // srslte_vec_conj_cc(psss_signal_time, psss_signal_time, fft_size);
    srslte_vec_sc_prod_cfc(psss_signal_time, 1.0/SRSLTE_PSSS_LEN, psss_signal_time, fft_size);

    // add half carrier shift
    int i;
    for(i=0; i<fft_size; i++) {
      psss_signal_time[i] *= cexpf(I*2*M_PI*((float) i)*0.5/fft_size);
    }

    srslte_dft_plan_free(&plan);

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}


/* Initializes the PSS synchronization object with fft_size=128
 */
int srslte_psss_init(srslte_psss_t *q, uint32_t frame_size) {
  return srslte_psss_init_fft(q, frame_size, 128);
}

int srslte_psss_init_fft(srslte_psss_t *q, uint32_t frame_size, uint32_t fft_size) {
  return srslte_psss_init_fft_offset(q, frame_size, fft_size, 0);
}

int srslte_psss_init_fft_offset(srslte_psss_t *q, uint32_t frame_size, uint32_t fft_size, int offset) {
  return srslte_psss_init_fft_offset_decim(q, frame_size, fft_size,  offset,  1);
}

/* Initializes the PSS synchronization object. 
 * 
 * It correlates a signal of frame_size samples with the PSS sequence in the frequency 
 * domain. The PSS sequence is transformed using fft_size samples. 
 */
int srslte_psss_init_fft_offset_decim(srslte_psss_t *q,
                                           uint32_t max_frame_size, uint32_t max_fft_size,
                                           int offset, int decimate)
{

  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if (q != NULL) {
  
    ret = SRSLTE_ERROR; 
    
    uint32_t N_id_2; 
    uint32_t buffer_size;
    bzero(q, sizeof(srslte_psss_t));
    
    q->N_id_2 = 10;  
    q->ema_alpha = 0.2;

    q->max_fft_size  = max_fft_size;
    q->max_frame_size = max_frame_size;

    q->decimate = decimate;
    uint32_t fft_size = max_fft_size/q->decimate;
    uint32_t frame_size = max_frame_size/q->decimate;
    
    q->fft_size = fft_size;
    q->frame_size = frame_size;

    buffer_size = fft_size + frame_size + 1;

    q->filter_psss_enable = false;
    q->chest_on_filter   = false;

    if(q->decimate > 1) {
        int filter_order = 3;
        srslte_filt_decim_cc_init(&q->filter,q->decimate,filter_order);
        q->filter.filter_output = srslte_vec_malloc((buffer_size) * sizeof(cf_t));
        q->filter.downsampled_input = srslte_vec_malloc((buffer_size + filter_order) * sizeof(cf_t));
        printf("decimation for the PSSS search is %d \n",q->decimate);
    }
      
    if (srslte_dft_plan(&q->dftp_input, fft_size, SRSLTE_DFT_FORWARD, SRSLTE_DFT_COMPLEX)) {
      fprintf(stderr, "Error creating DFT plan \n");
      goto clean_and_exit;
    }
    srslte_dft_plan_set_mirror(&q->dftp_input, true);
    srslte_dft_plan_set_dc(&q->dftp_input, true);
    srslte_dft_plan_set_norm(&q->dftp_input, false);

    if (srslte_dft_plan(&q->idftp_input, fft_size, SRSLTE_DFT_BACKWARD, SRSLTE_DFT_COMPLEX)) {
      fprintf(stderr, "Error creating DFT plan \n");
      goto clean_and_exit;
    }
    srslte_dft_plan_set_mirror(&q->idftp_input, true);
    srslte_dft_plan_set_dc(&q->idftp_input, true);
    srslte_dft_plan_set_norm(&q->idftp_input, false);

    bzero(q->tmp_fft2, sizeof(cf_t)*SRSLTE_SYMBOL_SZ_MAX);

    q->tmp_input = srslte_vec_malloc((buffer_size + frame_size*(q->decimate - 1)) * sizeof(cf_t));
    if (!q->tmp_input) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }

    bzero(&q->tmp_input[q->frame_size], q->fft_size * sizeof(cf_t));

    q->conv_output = srslte_vec_malloc(buffer_size * sizeof(cf_t));
    if (!q->conv_output) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output, sizeof(cf_t) * buffer_size);
    q->conv_output_avg = srslte_vec_malloc(buffer_size * sizeof(float));
    if (!q->conv_output_avg) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output_avg, sizeof(float) * buffer_size);
#ifdef SRSLTE_PSSS_ACCUMULATE_ABS
    q->conv_output_abs = srslte_vec_malloc(buffer_size * sizeof(float));
    if (!q->conv_output_abs) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output_abs, sizeof(float) * buffer_size);
#endif

    for (N_id_2=0;N_id_2<2;N_id_2++) {
      q->psss_signal_time[N_id_2] = srslte_vec_malloc(buffer_size * sizeof(cf_t));
      if (!q->psss_signal_time[N_id_2]) {
        fprintf(stderr, "Error allocating memory\n");
        goto clean_and_exit;
      }
      /* The PSS is translated into the time domain for each N_id_2  */
      if (srslte_psss_init_N_id_2(q->psss_signal_freq[N_id_2], q->psss_signal_time[N_id_2], N_id_2, fft_size, offset)) {
        fprintf(stderr, "Error initiating PSSS detector for N_id_2=%d fft_size=%d\n", N_id_2, fft_size);
        goto clean_and_exit;
      }
      bzero(&q->psss_signal_time[N_id_2][q->fft_size], q->frame_size * sizeof(cf_t));
    }
    #ifdef CONVOLUTION_FFT


    if (srslte_conv_fft_cc_init(&q->conv_fft, frame_size, fft_size)) {
      fprintf(stderr, "Error initiating convolution FFT\n");
      goto clean_and_exit;
    }
    for(N_id_2=0; N_id_2<2; N_id_2++) {
      q->psss_signal_freq_full[N_id_2] = srslte_vec_malloc(buffer_size * sizeof(cf_t));
      srslte_dft_run_c(&q->conv_fft.filter_plan, q->psss_signal_time[N_id_2], q->psss_signal_freq_full[N_id_2]);

      // for fft convolution we need to conjugate one of the signals
      srslte_vec_conj_cc(q->psss_signal_freq_full[N_id_2], q->psss_signal_freq_full[N_id_2], buffer_size);

    }

    #endif

    srslte_psss_reset(q);

    ret = SRSLTE_SUCCESS;
  }

clean_and_exit:
  if (ret == SRSLTE_ERROR) {
    srslte_psss_free(q);
  }
  return ret;

}

/* Initializes the PSSS synchronization object.
 *
 * It correlates a signal of frame_size samples with the PSS sequence in the frequency
 * domain. The PSS sequence is transformed using fft_size samples.
 */
int srslte_psss_resize(srslte_psss_t *q, uint32_t frame_size, uint32_t fft_size, int offset) {

  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if (q != NULL) {

    ret = SRSLTE_ERROR;

    if (fft_size > q->max_fft_size || frame_size > q->max_frame_size) {
      fprintf(stderr, "Error in pss_config(): fft_size and frame_size must be lower than initialized\n");
      return SRSLTE_ERROR;
    }

    uint32_t N_id_2;
    uint32_t buffer_size;

    q->N_id_2 = 10;
    q->ema_alpha = 0.2;

    fft_size = fft_size/q->decimate;
    frame_size = frame_size/q->decimate;

    q->fft_size = fft_size;
    q->frame_size = frame_size;

    buffer_size = fft_size + frame_size + 1;

    if (srslte_dft_replan(&q->dftp_input, fft_size)) {
      fprintf(stderr, "Error creating DFT plan \n");
      return SRSLTE_ERROR;
    }

    if (srslte_dft_replan(&q->idftp_input, fft_size)) {
      fprintf(stderr, "Error creating DFT plan \n");
      return SRSLTE_ERROR;
    }

    bzero(q->tmp_fft2, sizeof(cf_t)*SRSLTE_SYMBOL_SZ_MAX);

    bzero(&q->tmp_input[q->frame_size], q->fft_size * sizeof(cf_t));
    bzero(q->conv_output, sizeof(cf_t) * buffer_size);
    bzero(q->conv_output_avg, sizeof(float) * buffer_size);

#ifdef SRSLTE_PSS_ACCUMULATE_ABS
    bzero(q->conv_output_abs, sizeof(float) * buffer_size);
#endif

    // Generate PSS sequences for this FFT size
    for (N_id_2=0; N_id_2<2; N_id_2++) {
      if (srslte_psss_init_N_id_2(q->psss_signal_freq[N_id_2], q->psss_signal_time[N_id_2], N_id_2, fft_size, offset)) {
        fprintf(stderr, "Error initiating PSS detector for N_id_2=%d fft_size=%d\n", N_id_2, fft_size);
        return SRSLTE_ERROR;
      }
      bzero(&q->psss_signal_time[N_id_2][q->fft_size], q->frame_size * sizeof(cf_t));
    }
#ifdef CONVOLUTION_FFT

    if (srslte_conv_fft_cc_replan(&q->conv_fft, frame_size, fft_size)) {
      fprintf(stderr, "Error initiating convolution FFT\n");
      return SRSLTE_ERROR;
    }

    for(N_id_2=0; N_id_2<2; N_id_2++) {
      srslte_dft_run_c(&q->conv_fft.filter_plan, q->psss_signal_time[N_id_2], q->psss_signal_freq_full[N_id_2]);

      // for fft convolution we need to conjugate one of the signals
      srslte_vec_conj_cc(q->psss_signal_freq_full[N_id_2], q->psss_signal_freq_full[N_id_2], buffer_size);
    }

#endif

    srslte_psss_reset(q);

    ret = SRSLTE_SUCCESS;
  }
  return ret;

}

void srslte_psss_free(srslte_psss_t *q) {
  uint32_t i;

  if (q) {
    for (i=0;i<2;i++) {
      if (q->psss_signal_time[i]) {
        free(q->psss_signal_time[i]);
      }
      if(q->psss_signal_freq_full[i]){
        free(q->psss_signal_freq_full[i]);
      }
    }
  #ifdef CONVOLUTION_FFT
    srslte_conv_fft_cc_free(&q->conv_fft);

  #endif
    if (q->tmp_input) {
      free(q->tmp_input);
    }
    if (q->conv_output) {
      free(q->conv_output);
    }
    if (q->conv_output_abs) {
      free(q->conv_output_abs);
    }
    if (q->conv_output_avg) {
      free(q->conv_output_avg);
    }

    srslte_dft_plan_free(&q->dftp_input);
    srslte_dft_plan_free(&q->idftp_input);

    if(q->decimate > 1)
    {
        srslte_filt_decim_cc_free(&q->filter);
        free(q->filter.filter_output);
        free(q->filter.downsampled_input);
    }


    bzero(q, sizeof(srslte_psss_t));
  }
}

void srslte_psss_reset(srslte_psss_t *q) {
  uint32_t buffer_size = q->fft_size + q->frame_size + 1;
  bzero(q->conv_output_avg, sizeof(float) * buffer_size);
}

/**
 * This function calculates the Zadoff-Chu sequence.
 * @param signal Output array.
 */
int srslte_psss_generate(cf_t *signal, uint32_t N_id_2) {
  int i;
  float arg;
  const float root_value[] = { 26.0, 37.0 };
  int root_idx;

  int sign = -1;

  if (N_id_2 > 1) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return -1;
  }

  root_idx = N_id_2;

  for (i = 0; i < SRSLTE_PSSS_LEN / 2; i++) {
    arg = (float) sign * M_PI * root_value[root_idx]
        * ((float) i * ((float) i + 1.0)) / 63.0;
    __real__ signal[i] = cosf(arg);
    __imag__ signal[i] = sinf(arg);
  }
  for (i = SRSLTE_PSSS_LEN / 2; i < SRSLTE_PSSS_LEN; i++) {
    arg = (float) sign * M_PI * root_value[root_idx]
        * (((float) i + 2.0) * ((float) i + 1.0)) / 63.0;
    __real__ signal[i] = cosf(arg);
    __imag__ signal[i] = sinf(arg);
  }
  return 0;
}


/**
 * @brief  Mapping of SSSS to resource elements 36.211 14.2 section 9.7.1.2
 * 
 * @todo: Apply amplitude scaling, either here or during generation?
 * 
 * @todo: This is only valid for short CP
 * 
 * @param psss_signal   PSSS
 * @param sf            start of subframe in which PSSS is placed
 * @param nof_prb       number of PRB
 * @param cp            CP len
 */
void srslte_psss_put_sf(cf_t *psss_signal, cf_t *sf, uint32_t nof_prb, srslte_cp_t cp) {
  int k;
  // second symbol
  k = (1) * nof_prb * SRSLTE_NRE + (nof_prb * SRSLTE_NRE - SRSLTE_PSSS_LEN)/2;
  memset(&sf[k - 5], 0, 5 * sizeof(cf_t));
  memcpy(&sf[k], psss_signal, SRSLTE_PSSS_LEN * sizeof(cf_t));
  memset(&sf[k + SRSLTE_PSSS_LEN], 0, 5 * sizeof(cf_t));

  // third symbol
  k += nof_prb * SRSLTE_NRE;
  memset(&sf[k - 5], 0, 5 * sizeof(cf_t));
  memcpy(&sf[k], psss_signal, SRSLTE_PSSS_LEN * sizeof(cf_t));
  memset(&sf[k + SRSLTE_PSSS_LEN], 0, 5 * sizeof(cf_t));
}

#if 0
void srslte_pss_get_slot(cf_t *slot, cf_t *pss_signal, uint32_t nof_prb, srslte_cp_t cp) {
  int k;
  k = (SRSLTE_CP_NSYMB(cp) - 1) * nof_prb * SRSLTE_NRE + nof_prb * SRSLTE_NRE / 2 - 31;
  memcpy(pss_signal, &slot[k], SRSLTE_PSS_LEN * sizeof(cf_t));
}
#endif


/** Sets the current N_id_2 value. Returns -1 on error, 0 otherwise
 */
int srslte_psss_set_N_id_2(srslte_psss_t *q, uint32_t N_id_2) {
  if (!srslte_SL_N_id_2_isvalid((N_id_2))) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return -1;
  } else {
    q->N_id_2 = N_id_2;
    return 0;
  }
}

/* Sets the weight factor alpha for the exponential moving average of the PSS correlation output
 */
void srslte_psss_set_ema_alpha(srslte_psss_t *q, float alpha) {
  q->ema_alpha = alpha;
}


static int srslte_psss_compute_peak_sidelobe_pos2(float *conv_output, uint32_t corr_peak_pos1, uint32_t corr_peak_pos2, uint32_t conv_output_len)
{

  uint32_t peak_left  = corr_peak_pos1 > corr_peak_pos2 ? corr_peak_pos2 : corr_peak_pos1;
  uint32_t peak_right = corr_peak_pos1 > corr_peak_pos2 ? corr_peak_pos1 : corr_peak_pos2;

  // Find end of right peak lobe to the right
  int pl_ub = peak_right+1;
  while(conv_output[pl_ub+1] <= conv_output[pl_ub] && pl_ub < conv_output_len) {
    pl_ub ++;
  }

  int sl_distance_right = conv_output_len-1-pl_ub;
  if (sl_distance_right < 0) {
    sl_distance_right = 0;
  }
  int sl_right = pl_ub+srslte_vec_max_fi(&conv_output[pl_ub], sl_distance_right);

  // printf("Searching lobe in %d %d -> %d \n", pl_ub, sl_distance_right, sl_right);

  // Find end of left peak lobe to the left
  int pl_lb;
  if (peak_left > 2) {
    pl_lb = peak_left-1;
    while(conv_output[pl_lb-1] <= conv_output[pl_lb] && pl_lb > 1) {
      pl_lb --;
    }
  } else {
    pl_lb = 0;
  }

  int sl_distance_left = pl_lb;
  int sl_left = srslte_vec_max_fi(conv_output, sl_distance_left);

  // printf("Searching lobe in %d %d ->  %d\n", 0, sl_distance_left, sl_left);

  // Find end of right peak lobe to the left
  if (peak_right > 2) {
    pl_lb = peak_right-1;
    while(conv_output[pl_lb-1] <= conv_output[pl_lb] && pl_lb > 1 && pl_lb > peak_left) {
      pl_lb --;
    }
  } else {
    pl_lb = 0;
  }

  // Find end of left peak lobe to the right
  pl_ub = peak_left+1;
  while(conv_output[pl_ub+1] <= conv_output[pl_ub] && pl_ub < peak_right) {
    pl_ub ++;
  }

  int sl_distance_middle = pl_lb-1-pl_ub;
  if (sl_distance_middle < 0) {
    sl_distance_middle = 0;
  }
  int sl_middle = pl_ub+srslte_vec_max_fi(&conv_output[pl_ub], sl_distance_middle);

  // printf("Searching lobe in %d %d -> %d\n", pl_ub, sl_distance_middle, sl_middle);

  // get highest peak
  if(conv_output[sl_right] > conv_output[sl_left]){
    return conv_output[sl_middle] > conv_output[sl_right] ? sl_middle : sl_right;
  } else{
    return conv_output[sl_middle] > conv_output[sl_left] ? sl_middle : sl_left;
  }
}





static int srslte_psss_compute_peak_sidelobe_pos(float *conv_output, uint32_t corr_peak_pos, uint32_t conv_output_len)
{
  // Find end of peak lobe to the right
  int pl_ub = corr_peak_pos+1;
  while(conv_output[pl_ub+1] <= conv_output[pl_ub] && pl_ub < conv_output_len) {
    pl_ub ++;
  }
  // Find end of peak lobe to the left
  int pl_lb;
  if (corr_peak_pos > 2) {
    pl_lb = corr_peak_pos-1;
    while(conv_output[pl_lb-1] <= conv_output[pl_lb] && pl_lb > 1) {
      pl_lb --;
    }
  } else {
    pl_lb = 0;
  }

  int sl_distance_right = conv_output_len-1-pl_ub;
  if (sl_distance_right < 0) {
    sl_distance_right = 0;
  }
  int sl_distance_left = pl_lb;

  int sl_right = pl_ub+srslte_vec_max_fi(&conv_output[pl_ub], sl_distance_right);
  int sl_left = srslte_vec_max_fi(conv_output, sl_distance_left);


  return conv_output[sl_right] > conv_output[sl_left] ? sl_right : sl_left;
}

static int srslte_psss_compute_peak_sidelobe(srslte_psss_t *q, uint32_t corr_peak_pos, uint32_t conv_output_len)
{
  float side_lobe_value = q->conv_output_avg[srslte_psss_compute_peak_sidelobe_pos(q->conv_output_avg, corr_peak_pos, conv_output_len)];

  return q->conv_output_avg[corr_peak_pos]/side_lobe_value;
}

/** Performs time-domain PSS correlation.
 * Returns the index of the PSS correlation peak in a subframe.
 * The frame starts at corr_peak_pos-subframe_size/2.
 * The value of the correlation is stored in corr_peak_value.
 *
 * Input buffer must be subframe_size long.
 */
int srslte_psss_find_psss(srslte_psss_t *q, const cf_t *input, float *corr_peak_value)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL  &&
      input             != NULL)
  {

    uint32_t corr_peak_pos;
    uint32_t conv_output_len;

    if (!srslte_SL_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSSS peak, Must set N_id_2 first\n");
      return SRSLTE_ERROR;
    }

    /* Correlate input with PSS sequence
     *
     * We do not reverse time-domain PSS signal because it's conjugate is symmetric.
     * The conjugate operation on pss_signal_time has been done in srslte_pss_init_N_id_2
     * This is why we can use FFT-based convolution
     */
    if (q->frame_size >= q->fft_size) {
    #ifdef CONVOLUTION_FFT
      memcpy(q->tmp_input, input, (q->frame_size * q->decimate) * sizeof(cf_t));
      if(q->decimate > 1) {
        srslte_filt_decim_cc_execute(&(q->filter), q->tmp_input, q->filter.downsampled_input, q->filter.filter_output , (q->frame_size * q->decimate));
        conv_output_len = srslte_conv_fft_cc_run_opt(&q->conv_fft, q->filter.filter_output, q->psss_signal_freq_full[q->N_id_2], q->conv_output);
      } else {
        conv_output_len = srslte_conv_fft_cc_run_opt(&q->conv_fft, q->tmp_input, q->psss_signal_freq_full[q->N_id_2], q->conv_output);
      }

    #else
      conv_output_len = srslte_conv_cc(input, q->psss_signal_time[q->N_id_2], q->conv_output, q->frame_size, q->fft_size);
    #endif
    } else {
      for (int i=0;i<q->frame_size;i++) {
        //q->conv_output[i] = srslte_vec_dot_prod_ccc(q->psss_signal_time[q->N_id_2], &input[i], q->fft_size);
        q->conv_output[i] = srslte_vec_dot_prod_conj_ccc(&input[i], q->psss_signal_time[q->N_id_2], q->fft_size);
        
      }
      conv_output_len = q->frame_size;
    }

    // Compute modulus square
    srslte_vec_abs_square_cf(q->conv_output, q->conv_output_abs, conv_output_len-1);

    /* Find maximum of the absolute value of the correlation */
    corr_peak_pos = srslte_vec_max_fi(q->conv_output_abs, conv_output_len-1);


    if (q->frame_size >= q->fft_size) {
      // i assume we are in find operation, so we are looking for two peaks
      int next_peak = srslte_psss_compute_peak_sidelobe_pos(q->conv_output_abs, corr_peak_pos, conv_output_len);

      // printf("Peak pos is %d with value %f\n", corr_peak_pos, q->conv_output_abs[corr_peak_pos]);
      // printf("\t next Peak pos is %d with value %f\n", next_peak, q->conv_output_abs[next_peak]);

      if(abs(abs(corr_peak_pos - next_peak) - (q->fft_size + SRSLTE_CP_LEN((q->fft_size),SRSLTE_CP_NORM_LEN))) > 5){
        printf("PSSS peaks are too far apart: [%d] = %f [%d] = %f\n",
                corr_peak_pos, q->conv_output_abs[corr_peak_pos],
                next_peak, q->conv_output_abs[next_peak]);
        *corr_peak_value = 0;
        return 0;
      }

      // check if both peaks are in same magnitude range
      float ratio = q->conv_output_abs[corr_peak_pos] / q->conv_output_abs[next_peak];
      if (ratio > 2.0 || ratio < 0.5) {
        printf("PSSS peaks values are not similar: [%d] = %f [%d] = %f\n",
                corr_peak_pos, q->conv_output_abs[corr_peak_pos],
                next_peak, q->conv_output_abs[next_peak]);
        *corr_peak_value = 0;
        return 0;
      }


      int psr = srslte_psss_compute_peak_sidelobe_pos2(q->conv_output_abs, corr_peak_pos, next_peak, conv_output_len);

      printf("psr: [%d] = %f corr_peak_pos: [%d] = %f next_peaks [%d] = %f\n",
              psr, q->conv_output_abs[psr],
              corr_peak_pos, q->conv_output_abs[corr_peak_pos],
              next_peak, q->conv_output_abs[next_peak]);

      *corr_peak_value = q->conv_output_abs[corr_peak_pos] / q->conv_output_abs[psr];

      q->peak_value = q->conv_output_abs[corr_peak_pos];

      return corr_peak_pos < next_peak ? corr_peak_pos : next_peak;


    } else {
      // we are in tracking state, so we check for absolute peak

      memcpy(q->conv_output_avg, q->conv_output_abs, sizeof(float)*(conv_output_len-1));

      // save absolute value
      q->peak_value = q->conv_output_avg[corr_peak_pos];

      *corr_peak_value = srslte_psss_compute_peak_sidelobe(q, corr_peak_pos, conv_output_len);

      return corr_peak_pos;

    }


    // If enabled, average the absolute value from previous calls
    if (q->ema_alpha < 1.0 && q->ema_alpha > 0.0) {
      srslte_vec_sc_prod_fff(q->conv_output_abs, q->ema_alpha, q->conv_output_abs, conv_output_len-1);
      srslte_vec_sc_prod_fff(q->conv_output_avg, 1-q->ema_alpha, q->conv_output_avg, conv_output_len-1);

      srslte_vec_sum_fff(q->conv_output_abs, q->conv_output_avg, q->conv_output_avg, conv_output_len-1);
    } else {
      memcpy(q->conv_output_avg, q->conv_output_abs, sizeof(float)*(conv_output_len-1));
    }

    /* Find maximum of the absolute value of the correlation */
    corr_peak_pos = srslte_vec_max_fi(q->conv_output_avg, conv_output_len-1);

    // save absolute value
    q->peak_value = q->conv_output_avg[corr_peak_pos];

#ifdef SRSLTE_PSSS_RETURN_PSR
    if (corr_peak_value) {
      *corr_peak_value = srslte_psss_compute_peak_sidelobe(q, corr_peak_pos, conv_output_len);
    }
#else
    if (corr_peak_value) {
      *corr_peak_value = q->conv_output_avg[corr_peak_pos];
    }
#endif

    if(q->decimate >1) {
      int decimation_correction = (q->filter.num_taps - 2);
      corr_peak_pos = corr_peak_pos - decimation_correction;
      corr_peak_pos = corr_peak_pos*q->decimate;
    }

    if (q->frame_size >= q->fft_size) {
      ret = (int) corr_peak_pos;
    } else {
      ret = (int) corr_peak_pos + q->fft_size;
    }
  }
  return ret;
}

#if 0
/* Computes frequency-domain channel estimation of the PSS symbol
 * input signal is in the time-domain.
 * ce is the returned frequency-domain channel estimates.
 */
int srslte_pss_chest(srslte_pss_t *q, const cf_t *input, cf_t ce[SRSLTE_PSS_LEN]) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL  &&
      input             != NULL)
  {

    if (!srslte_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSS peak, Must set N_id_2 first\n");
      return SRSLTE_ERROR;
    }

    /* Transform to frequency-domain */
    srslte_dft_run_c(&q->dftp_input, input, q->tmp_fft);

    /* Compute channel estimate taking the PSS sequence as reference */
    srslte_vec_prod_conj_ccc(&q->tmp_fft[(q->fft_size-SRSLTE_PSS_LEN)/2], q->pss_signal_freq[q->N_id_2], ce, SRSLTE_PSS_LEN);

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

/* input points to beginning of last OFDM symbol of slot 0 of subframe 0 or 5
 * It must be called after calling srslte_pss_cfo_compute() with filter enabled
 */
void srslte_pss_sic(srslte_pss_t *q, cf_t *input) {
  if (q->chest_on_filter) {

    bzero(q->tmp_fft, sizeof(cf_t)*q->fft_size);

    // Pass transmitted PSS sequence through the channel
    srslte_vec_prod_ccc(q->pss_signal_freq[q->N_id_2], q->tmp_ce, &q->tmp_fft[(q->fft_size-SRSLTE_PSS_LEN)/2], SRSLTE_PSS_LEN);

    // Get time-domain version of the received PSS
    srslte_dft_run_c(&q->idftp_input, q->tmp_fft, q->tmp_fft2);

    // Substract received PSS from this N_id_2 from the input signal
    srslte_vec_sc_prod_cfc(q->tmp_fft2, 1.0/q->fft_size, q->tmp_fft2, q->fft_size);
    srslte_vec_sub_ccc(input, q->tmp_fft2, input, q->fft_size);

  } else {
    fprintf(stderr, "Error calling srslte_pss_sic(): need to enable channel estimation on filtering\n");
  }
}
#endif

// Frequency-domain filtering of the central 64 sub-carriers
void srslte_psss_filter(srslte_psss_t *q, const cf_t *input, cf_t *output)
{
  srslte_dft_run_c(&q->dftp_input, input, q->tmp_fft);

  memcpy(&q->tmp_fft2[q->fft_size/2-SRSLTE_PSSS_LEN/2],
         &q->tmp_fft[q->fft_size/2-SRSLTE_PSSS_LEN/2],
         sizeof(cf_t)*SRSLTE_PSSS_LEN);

  if (q->chest_on_filter) {
    srslte_vec_prod_conj_ccc(&q->tmp_fft[(q->fft_size-SRSLTE_PSSS_LEN)/2], q->psss_signal_freq[q->N_id_2], q->tmp_ce, SRSLTE_PSSS_LEN);
  }

  srslte_dft_run_c(&q->idftp_input, q->tmp_fft2, output);
}


/* Returns the CFO estimation given a PSS received sequence
 *
 * Source: An Efﬁcient CFO Estimation Algorithm for the Downlink of 3GPP-LTE
 *       Feng Wang and Yu Zhu
 */
float srslte_psss_cfo_compute(srslte_psss_t* q, const cf_t *pss_recv) {
  cf_t y0, y1;

  const cf_t *pss_ptr = pss_recv;

  if (q->filter_psss_enable) {
    srslte_psss_filter(q, pss_recv, q->tmp_fft);
    pss_ptr = (const cf_t*) q->tmp_fft;
  }

  // the psss needs be conjugated, this differs from pss implmentation where the conjugation
  // is already done during initialisation
  y0 = srslte_vec_dot_prod_conj_ccc(pss_ptr, q->psss_signal_time[q->N_id_2], q->fft_size/2);
  y1 = srslte_vec_dot_prod_conj_ccc(&pss_ptr[q->fft_size/2], &q->psss_signal_time[q->N_id_2][q->fft_size/2], q->fft_size/2);

  return carg(conjf(y0) * y1)/M_PI;
}


/**
 * @brief Computes CFO by comparing both PSSS.
 * 
 * @param q 
 * @param subframe Pointer to start of first psss sequence
 * @return float 
 */
float srslte_psss_cfo_compute_with_psss(srslte_psss_t* q, const cf_t *subframe){

  uint32_t short_symbol_size = SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_LEN) + q->fft_size;
  // result += x[i] * conjf(y[i]);
  // we used the first psss as reference, and therefore this one needs to be conjugated
  // cf_t xc = srslte_vec_dot_prod_conj_ccc(&subframe[SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_0_LEN) + 2*short_symbol_size],
  //                                         &subframe[SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_0_LEN) + 1*short_symbol_size],
  //                                         q->fft_size);

  cf_t xc = srslte_vec_dot_prod_conj_ccc(&subframe[1*short_symbol_size],
                                          &subframe[0*short_symbol_size],
                                          q->fft_size);

  float cfo = carg(xc)*q->fft_size/short_symbol_size/2/M_PI;

  return cfo;
}



/**
 * @brief Computes CFO by comparing both SSSS.
 * 
 * @param q 
 * @param subframe Pointer to start of subframe where SSSS is found in.
 * @return float 
 */
float srslte_psss_cfo_compute_with_ssss(srslte_psss_t* q, const cf_t *subframe){

  uint32_t short_symbol_size = SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_LEN) + q->fft_size;
  uint32_t long_symbol_size = SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_0_LEN) + q->fft_size;

  cf_t xc = srslte_vec_dot_prod_conj_ccc(&subframe[2*long_symbol_size + 10*short_symbol_size + SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_LEN)],
                                          &subframe[2*long_symbol_size + 9*short_symbol_size + SRSLTE_CP_LEN(q->fft_size, SRSLTE_CP_NORM_LEN)],
                                          q->fft_size);

  float cfo = carg(xc)*q->fft_size/short_symbol_size/2/M_PI;

  printf("srslte_psss_cfo_compute_with_ssss returned %f\n", cfo);

  return cfo;
}
