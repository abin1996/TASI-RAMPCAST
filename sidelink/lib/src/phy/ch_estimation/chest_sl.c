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
#include <string.h>
#include <complex.h>
#include <math.h>
#include <srslte/srslte.h>
#include <srslte/phy/common/phy_common.h>

#include "srslte/config.h"

#include "srslte/phy/dft/dft_precoding.h"
#include "srslte/phy/ch_estimation/refsignal_sl.h"
#include "srslte/phy/ch_estimation/chest_sl.h"
#include "srslte/phy/utils/vector.h"
#include "srslte/phy/utils/convolution.h"

// number of reference symbols per subframe varies by physical channel
// @todo: current we use max of 4 sysbols per subframe
#define NOF_REFS_SYM    (q->cell.nof_prb*SRSLTE_NRE)
#define NOF_REFS_SF     (NOF_REFS_SYM*4) // 2 reference symbols per subframe

#define MAX_REFS_SYM    (max_prb*SRSLTE_NRE)
#define MAX_REFS_SF     (max_prb*SRSLTE_NRE*4) // 2 reference symbols per subframe


/** 3GPP LTE Downlink channel estimator and equalizer. 
 * Estimates the channel in the resource elements transmitting references and interpolates for the rest
 * of the resource grid. 
 * 
 * The equalizer uses the channel estimates to produce an estimation of the transmitted symbol. 
 * 
 * This object depends on the srslte_refsignal_t object for creating the LTE CSR signal.  
*/

int srslte_chest_sl_init(srslte_chest_sl_t *q, uint32_t max_prb)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if (q                != NULL)
  {
    bzero(q, sizeof(srslte_chest_sl_t));

    ret = srslte_refsignal_ul_init(&q->dmrs_signal, max_prb);
    if (ret != SRSLTE_SUCCESS) {
      fprintf(stderr, "Error initializing CSR signal (%d)\n",ret);
      goto clean_exit;
    }
    
    q->tmp_noise = srslte_vec_malloc(sizeof(cf_t) * MAX_REFS_SF);
    if (!q->tmp_noise) {
      perror("malloc");
      goto clean_exit;
    }
    q->pilot_estimates = srslte_vec_malloc(sizeof(cf_t) * MAX_REFS_SF);
    if (!q->pilot_estimates) {
      perror("malloc");
      goto clean_exit;
    }      
    for (int i=0;i<4;i++) {
      q->pilot_estimates_tmp[i] = srslte_vec_malloc(sizeof(cf_t) * MAX_REFS_SF);
      if (!q->pilot_estimates_tmp[i]) {
        perror("malloc");
        goto clean_exit;
      }      
    }
    q->pilot_recv_signal = srslte_vec_malloc(sizeof(cf_t) * (MAX_REFS_SF+1));
    if (!q->pilot_recv_signal) {
      perror("malloc");
      goto clean_exit;
    }
    
    q->pilot_known_signal = srslte_vec_malloc(sizeof(cf_t) * (MAX_REFS_SF+1));
    if (!q->pilot_known_signal) {
      perror("malloc");
      goto clean_exit;
    }
    
    if (srslte_interp_linear_vector_init(&q->srslte_interp_linvec, MAX_REFS_SYM)) {
      fprintf(stderr, "Error initializing vector interpolator\n");
      goto clean_exit; 
    }

    q->rsrp_neighbour = false;
    q->smooth_filter_len = 7;
    //srslte_chest_sl_set_smooth_filter3_coeff(q, 0.3333);
    srslte_chest_set_rect_filter(q->smooth_filter, q->smooth_filter_len);
  
    q->dmrs_signal_configured = false;

    if (srslte_refsignal_dmrs_pusch_pregen_init(&q->dmrs_pregen, max_prb)) {
      fprintf(stderr, "Error allocating memory for pregenerated signals\n");
      goto clean_exit;
    }
  
  }
    
  ret = SRSLTE_SUCCESS;

clean_exit:
  if (ret != SRSLTE_SUCCESS) {
      srslte_chest_sl_free(q);
  }
  return ret; 
}



void srslte_chest_sl_free(srslte_chest_sl_t *q) 
{
  srslte_refsignal_dmrs_pusch_pregen_free(&q->dmrs_signal, &q->dmrs_pregen);

  srslte_refsignal_ul_free(&q->dmrs_signal);
  if (q->tmp_noise) {
    free(q->tmp_noise);
  }
  srslte_interp_linear_vector_free(&q->srslte_interp_linvec);
  
  if (q->pilot_estimates) {
    free(q->pilot_estimates);
  }      
  for (int i=0;i<4;i++) {
    if (q->pilot_estimates_tmp[i]) {
      free(q->pilot_estimates_tmp[i]);
    }
  }
  if (q->pilot_recv_signal) {
    free(q->pilot_recv_signal);
  }
  if (q->pilot_known_signal) {
    free(q->pilot_known_signal);
  }
  bzero(q, sizeof(srslte_chest_sl_t));
}


int srslte_chest_sl_set_cell(srslte_chest_sl_t *q, srslte_cell_t cell)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  if (q                != NULL &&
      srslte_cell_isvalid(&cell))
  {
    if (cell.id != q->cell.id || q->cell.nof_prb == 0) {
      memcpy(&q->cell, &cell, sizeof(srslte_cell_t));
      ret = srslte_refsignal_ul_set_cell(&q->dmrs_signal, cell);
      if (ret != SRSLTE_SUCCESS) {
        fprintf(stderr, "Error initializing CSR signal (%d)\n",ret);
        return SRSLTE_ERROR;
      }

      if (srslte_interp_linear_vector_resize(&q->srslte_interp_linvec, NOF_REFS_SYM)) {
        fprintf(stderr, "Error initializing vector interpolator\n");
        return SRSLTE_ERROR;
      }
    }
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}


void srslte_chest_sl_set_cfg(srslte_chest_sl_t *q, 
                             srslte_refsignal_dmrs_pusch_cfg_t *pusch_cfg,
                             srslte_pucch_cfg_t *pucch_cfg, 
                             srslte_refsignal_srs_cfg_t *srs_cfg)
{
  // srslte_refsignal_ul_set_cfg(&q->dmrs_signal, pusch_cfg, pucch_cfg, srs_cfg);
  srslte_refsignal_dmrs_pusch_pregen(&q->dmrs_signal, &q->dmrs_pregen, pusch_cfg);
  q->dmrs_signal_configured = true; 
}


#if 0
/* Uses the difference between the averaged and non-averaged pilot estimates */
static float estimate_noise_pilots(srslte_chest_ul_t *q, cf_t *ce, uint32_t nrefs, uint32_t n_prb[2]) 
{
  
  float power = 0; 
  for (int i=0;i<2;i++) {
    power += srslte_chest_estimate_noise_pilots(&q->pilot_estimates[i*nrefs], 
                                                &ce[SRSLTE_REFSIGNAL_UL_L(i, q->cell.cp)*q->cell.nof_prb*SRSLTE_NRE+n_prb[i]*SRSLTE_NRE], 
                                                q->tmp_noise, 
                                                nrefs);
  }

  power/=2; 
  
  if (q->smooth_filter_len == 3) {
    // Calibrated for filter length 3
    float w=q->smooth_filter[0];
    float a=7.419*w*w+0.1117*w-0.005387;
    return (power/(a*0.8)); 
  } else {
    return power;     
  }
}
#endif

//#define cesymb(i) ce[SRSLTE_RE_IDX(q->cell.nof_prb,i,n_prb[0]*SRSLTE_NRE)]
#define cesymb(i) ce[SRSLTE_RE_IDX(q->cell.nof_prb,i,(q->cell.nof_prb-6)*SRSLTE_NRE/2)]
static void interpolate_pilots_psbch_mode4(srslte_chest_sl_t *q, cf_t *ce)
{
  uint32_t L1 = 4;
  uint32_t L2 = 6;//SRSLTE_REFSIGNAL_UL_L(1, q->cell.cp); 
  uint32_t L3 = 9;//SRSLTE_REFSIGNAL_UL_L(1, q->cell.cp); 
  uint32_t NL = 2*SRSLTE_CP_NSYMB(q->cell.cp);

  // PSBCH spans 6 prbs
  const uint32_t nrefs = 6*SRSLTE_NRE;
    
  /* Interpolate in the time domain between symbols */

  // from l1 to left
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb(L2), &cesymb(L1), &cesymb(L1), &cesymb(L1-1), (L2-L1), L1,        false, nrefs);

  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb(L1), &cesymb(L2), NULL,        &cesymb(L1+1), (L2-L1), (L2-L1)-1, true,  nrefs);

  // from l2 to l3
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb(L2), &cesymb(L3), NULL,        &cesymb(L2+1), (L3-L2), (L3-L2)-1, true,  nrefs);

  // from l3 to end
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb(L2), &cesymb(L3), &cesymb(L3), &cesymb(L3+1), (L3-L2), (NL-L3)-1, true,  nrefs);
}


#define cesymb_pscch(i) ce[SRSLTE_RE_IDX(q->cell.nof_prb,i,prb_offset*SRSLTE_NRE)]
static void interpolate_pilots_psxch_mode4(srslte_chest_sl_t *q, cf_t *ce, uint32_t prb_offset, uint32_t nrefs) 
{
  uint32_t L1 = 2;
  uint32_t L2 = 5;//SRSLTE_REFSIGNAL_UL_L(1, q->cell.cp); 
  uint32_t L3 = 8;//SRSLTE_REFSIGNAL_UL_L(1, q->cell.cp); 
  uint32_t L4 = 11;//SRSLTE_REFSIGNAL_UL_L(1, q->cell.cp); 
  uint32_t NL = 2*SRSLTE_CP_NSYMB(q->cell.cp);
    
  /* Interpolate in the time domain between symbols */

  // from l1 to left
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb_pscch(L2), &cesymb_pscch(L1), &cesymb_pscch(L1), &cesymb_pscch(L1-1),
                               (L2-L1), L1,        false, nrefs);

  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb_pscch(L1), &cesymb_pscch(L2), NULL,        &cesymb_pscch(L1+1),
                               (L2-L1), (L2-L1)-1, true,  nrefs);

  // from l2 to l3
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb_pscch(L2), &cesymb_pscch(L3), NULL,        &cesymb_pscch(L2+1),
                               (L3-L2), (L3-L2)-1, true,  nrefs);

  // from l3 to l4
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb_pscch(L3), &cesymb_pscch(L4), NULL,        &cesymb_pscch(L3+1),
                               (L4-L3), (L4-L3)-1, true,  nrefs);

  // from l4 to end
  srslte_interp_linear_vector3(&q->srslte_interp_linvec, 
                               &cesymb_pscch(L3), &cesymb_pscch(L4), &cesymb_pscch(L4), &cesymb_pscch(L4+1),
                               (L4-L3), (NL-L4)-1, true,  nrefs);
}

#if 0
void srslte_chest_ul_set_smooth_filter(srslte_chest_ul_t *q, float *filter, uint32_t filter_len) {
  if (filter_len < SRSLTE_CHEST_MAX_SMOOTH_FIL_LEN) {
    if (filter) {
      memcpy(q->smooth_filter, filter, filter_len*sizeof(float));    
      q->smooth_filter_len = filter_len; 
    } else {
      q->smooth_filter_len = 0; 
    }
  } else {
    fprintf(stderr, "Error setting smoothing filter: filter len exceeds maximum (%d>%d)\n", 
      filter_len, SRSLTE_CHEST_MAX_SMOOTH_FIL_LEN);
  }
}
#endif

void srslte_chest_sl_set_smooth_filter3_coeff(srslte_chest_sl_t* q, float w)
{
  srslte_chest_set_smooth_filter3_coeff(q->smooth_filter, w); 
  q->smooth_filter_len = 3; 
}

#if 0
static void average_pilots(srslte_chest_ul_t *q, cf_t *input, cf_t *ce, uint32_t nrefs, uint32_t n_prb[2]) {
  for (int i=0;i<2;i++) {
    srslte_chest_average_pilots(&input[i*nrefs], 
                                &ce[SRSLTE_REFSIGNAL_UL_L(i, q->cell.cp)*q->cell.nof_prb*SRSLTE_NRE+n_prb[i]*SRSLTE_NRE], 
                                q->smooth_filter, nrefs, 1, q->smooth_filter_len);
  }
}

int srslte_chest_ul_estimate(srslte_chest_ul_t *q, cf_t *input, cf_t *ce, 
                             uint32_t nof_prb, uint32_t sf_idx, uint32_t cyclic_shift_for_dmrs, uint32_t n_prb[2]) 
{
  if (!q->dmrs_signal_configured) {
    fprintf(stderr, "Error must call srslte_chest_ul_set_cfg() before using the UL estimator\n");
    return SRSLTE_ERROR; 
  }
  
  if (!srslte_dft_precoding_valid_prb(nof_prb)) {
    fprintf(stderr, "Error invalid nof_prb=%d\n", nof_prb);
    return SRSLTE_ERROR_INVALID_INPUTS;
  }
  
  int nrefs_sym = nof_prb*SRSLTE_NRE; 
  int nrefs_sf  = nrefs_sym*2; 
  
  /* Get references from the input signal */
  srslte_refsignal_dmrs_pusch_get(&q->dmrs_signal, input, nof_prb, n_prb, q->pilot_recv_signal);
  
  /* Use the known DMRS signal to compute Least-squares estimates */
  srslte_vec_prod_conj_ccc(q->pilot_recv_signal, q->dmrs_pregen.r[cyclic_shift_for_dmrs][sf_idx][nof_prb], 
                           q->pilot_estimates, nrefs_sf);
  
  if (n_prb[0] != n_prb[1]) {
    printf("ERROR: intra-subframe frequency hopping not supported in the estimator!!\n");
  }
  
  if (ce != NULL) {
    if (q->smooth_filter_len > 0) {
      average_pilots(q, q->pilot_estimates, ce, nrefs_sym, n_prb);
      interpolate_pilots(q, ce, nrefs_sym, n_prb);        
      
      /* If averaging, compute noise from difference between received and averaged estimates */
      q->noise_estimate = estimate_noise_pilots(q, ce, nrefs_sym, n_prb);
    } else {
      // Copy estimates to CE vector without averaging
      for (int i=0;i<2;i++) {
        memcpy(&ce[SRSLTE_REFSIGNAL_UL_L(i, q->cell.cp)*q->cell.nof_prb*SRSLTE_NRE+n_prb[i]*SRSLTE_NRE], 
               &q->pilot_estimates[i*nrefs_sym], 
               nrefs_sym*sizeof(cf_t));        
      }
      interpolate_pilots(q, ce, nrefs_sym, n_prb);                  
      q->noise_estimate = 0;              
    }
  }
  
  // Estimate received pilot power 
  q->pilot_power = srslte_vec_avg_power_cf(q->pilot_recv_signal, nrefs_sf); 
  return 0;
}
#endif

int srslte_chest_sl_estimate_psbch(srslte_chest_sl_t *q, cf_t *input, cf_t *ce, srslte_sl_mode_t sl_mode, 
                                                 uint32_t port_id, uint32_t rxant_id) //JL-s_rsrp, added last two input.
                                  //  srslte_pucch_format_t format, uint32_t n_pucch, uint32_t sf_idx, 
                                  //  uint8_t *pucch2_ack_bits) 
{
  // if (!q->dmrs_signal_configured) {
  //   fprintf(stderr, "Error must call srslte_chest_ul_set_cfg() before using the UL estimator\n");
  //   fprintf(stderr, "I will go on, because in SL this seems not to be required\n");
  //   //return SRSLTE_ERROR; 
  // }
    
  int n_rs = srslte_refsignal_sl_dmrs_psbch_N_rs(sl_mode, 0) + srslte_refsignal_sl_dmrs_psbch_N_rs(sl_mode, 1);
  if (!n_rs) {
    fprintf(stderr, "Error computing N_rs\n");
    return SRSLTE_ERROR; 
  }

  const int nof_prb_psbch = 6;

  uint32_t nsymbols = SRSLTE_CP_ISNORM(q->cell.cp)?SRSLTE_CP_NORM_NSYMB:SRSLTE_CP_EXT_NSYMB;
  int nrefs_sf = nof_prb_psbch*SRSLTE_NRE*n_rs;
  
  /* Get references from the input signal */
  srslte_refsignal_sl_dmrs_psbch_get(&q->dmrs_signal, sl_mode, input, q->pilot_recv_signal);

    /* JL-s_rsrp, Compute s_rsrp for the channel estimates in this port */
  if (q->rsrp_neighbour) {
    double energy = cabs(srslte_vec_acc_cc(q->pilot_estimates, nrefs_sf)/nrefs_sf);
    q->rsrp_corr[rxant_id][port_id] = energy*energy;
  }
  q->s_rsrp[rxant_id][port_id] = srslte_vec_avg_power_cf(q->pilot_recv_signal, nrefs_sf);
  
  // generate psbch dmrs
  srslte_refsignal_sl_dmrs_psbch_gen(&q->dmrs_signal, nof_prb_psbch, 0, 0, q->pilot_known_signal);

  /* Use the known DMRS signal to compute Least-squares estimates */
  srslte_vec_prod_conj_ccc(q->pilot_recv_signal, q->pilot_known_signal, q->pilot_estimates, nrefs_sf);

  float power = 0;
  float est_power = 0.0;

  // generate channel estimates for the whole subframe
  if (ce != NULL) {
    if (q->smooth_filter_len > 0) {

      int dmrs_c = 0;
      for (int ns=0;ns<2;ns++) {
        uint32_t rs = srslte_refsignal_sl_dmrs_psbch_N_rs(sl_mode, ns);
        for(int l=0; l<rs; l++) {
          int dmrs_pos = srslte_refsignal_sl_dmrs_psbch_symbol(l, sl_mode, ns, q->cell.cp);

          srslte_chest_average_pilots(&q->pilot_estimates[dmrs_c*nof_prb_psbch*SRSLTE_NRE], 
                                &ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, q->cell.nof_prb*SRSLTE_NRE/2 - 36)],
                                q->smooth_filter, nof_prb_psbch*SRSLTE_NRE, 1, q->smooth_filter_len);

          // compute noise from difference between received and averaged estimates
          power += srslte_chest_estimate_noise_pilots(&q->pilot_estimates[dmrs_c*nof_prb_psbch*SRSLTE_NRE], 
                      &ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, q->cell.nof_prb*SRSLTE_NRE/2 - 36)],
                      q->tmp_noise, 
                      nof_prb_psbch*SRSLTE_NRE);

          // get power of averaged ce
          est_power += srslte_vec_avg_power_cf(&ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, q->cell.nof_prb*SRSLTE_NRE/2 - 36)], nof_prb_psbch*SRSLTE_NRE);

          dmrs_c++;
        }
      }

      interpolate_pilots_psbch_mode4(q, ce);
      
      /* If averaging, compute noise from difference between received and averaged estimates */
      //q->noise_estimate = estimate_noise_pilots(q, ce, nrefs_sym, n_prb);

      q->noise_estimate = power/dmrs_c;
      q->ce_power_estimate = est_power/dmrs_c;
    } else {

      // Copy estimates to CE vector without averaging

      int dmrs_c = 0;
      for (int ns=0;ns<2;ns++) {
        uint32_t rs = srslte_refsignal_sl_dmrs_psbch_N_rs(sl_mode, ns);
        for(int l=0; l<rs; l++) {
          int dmrs_pos = srslte_refsignal_sl_dmrs_psbch_symbol(l, sl_mode, ns, q->cell.cp);

          memcpy(&ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, q->cell.nof_prb*SRSLTE_NRE/2 - 36)],
                &q->pilot_estimates[dmrs_c*nof_prb_psbch*SRSLTE_NRE],
                nof_prb_psbch*SRSLTE_NRE*sizeof(cf_t));

          dmrs_c++;
        }
      }

      interpolate_pilots_psbch_mode4(q, ce);
                     
      q->noise_estimate = 0;              
    }
  }

  q->pilot_power = srslte_vec_avg_power_cf(q->pilot_recv_signal, nrefs_sf);

  return 0;
}


float srslte_chest_sl_get_noise_estimate(srslte_chest_sl_t *q) {
  return q->noise_estimate;
}


float srslte_chest_sl_get_snr(srslte_chest_sl_t *q) {
  return q->pilot_power/srslte_chest_sl_get_noise_estimate(q);
}

float srslte_chest_sl_get_snr_db(srslte_chest_sl_t *q) {
  // we have two different methods to estimate the SNR and the currently used
  // one, shows empirically the best result
  // 10*log10((q->chest.pilot_power - q->noise_estimate) / q->noise_estimate)
  return 10*log10(q->ce_power_estimate / q->noise_estimate);
}

int srslte_chest_sl_estimate_psxch(srslte_chest_sl_t *q, cf_t *input, cf_t *ce, srslte_sl_mode_t sl_mode,
                                    uint32_t prb_offset,
                                    uint32_t prb_n)
                                  //  srslte_pucch_format_t format, uint32_t n_pucch, uint32_t sf_idx, 
                                  //  uint8_t *pucch2_ack_bits) 
{
  // if (!q->dmrs_signal_configured) {
  //   fprintf(stderr, "Error must call srslte_chest_ul_set_cfg() before using the UL estimator\n");
  //   fprintf(stderr, "I will go on, because in SL this seems not to be required\n");
  //   //return SRSLTE_ERROR; 
  // }
    
  int n_rs = srslte_refsignal_sl_dmrs_pscch_N_rs(sl_mode, 0) + srslte_refsignal_sl_dmrs_pscch_N_rs(sl_mode, 1);
  if (!n_rs) {
    fprintf(stderr, "Error computing N_rs\n");
    return SRSLTE_ERROR; 
  }

  uint32_t nsymbols = SRSLTE_CP_ISNORM(q->cell.cp)?SRSLTE_CP_NORM_NSYMB:SRSLTE_CP_EXT_NSYMB;

  int nrefs_sym = prb_n*SRSLTE_NRE;
  int nrefs_sf = prb_n*SRSLTE_NRE*n_rs;
  
  /* Get references from the input signal */
  srslte_refsignal_sl_dmrs_psxch_get(&q->dmrs_signal, sl_mode, prb_offset, prb_n, input, q->pilot_recv_signal);
  
  // generate psbch dmrs
  // srslte_refsignal_sl_dmrs_pscch_gen(&q->dmrs_signal, prb_n, 0, 0, q->pilot_known_signal);

  /* Use the known DMRS signal to compute Least-squares estimates */
  srslte_vec_prod_conj_ccc(q->pilot_recv_signal, q->pilot_known_signal, q->pilot_estimates, nrefs_sf);

  float power = 0;
  float est_power = 0.0;

  // generate channel estimates for the whole subframe
  if (ce != NULL) {
    if (q->smooth_filter_len > 0) {

      int dmrs_c = 0;
      for (int ns=0;ns<2;ns++) {
        uint32_t rs = srslte_refsignal_sl_dmrs_pscch_N_rs(sl_mode, ns);
        for(int l=0; l<rs; l++) {
          int dmrs_pos = srslte_refsignal_sl_dmrs_pscch_symbol(l, sl_mode, ns, q->cell.cp);

          srslte_chest_average_pilots(&q->pilot_estimates[dmrs_c*prb_n*SRSLTE_NRE], 
                                &ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, prb_offset*SRSLTE_NRE)],
                                q->smooth_filter, prb_n*SRSLTE_NRE, 1, q->smooth_filter_len);

          // compute noise from difference between received and averaged estimates
          power += srslte_chest_estimate_noise_pilots(&q->pilot_estimates[dmrs_c*prb_n*SRSLTE_NRE], 
                      &ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, prb_offset*SRSLTE_NRE)],
                      q->tmp_noise, 
                      prb_n*SRSLTE_NRE);

          // get power of averaged ce
          est_power += srslte_vec_avg_power_cf(&ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, prb_offset*SRSLTE_NRE)], prb_n*SRSLTE_NRE);

          dmrs_c++;
        }
      }

      interpolate_pilots_psxch_mode4(q, ce, prb_offset, nrefs_sym);

      q->noise_estimate = power/dmrs_c;

      q->ce_power_estimate = est_power/dmrs_c;
    } else {

      // Copy estimates to CE vector without averaging

      int dmrs_c = 0;
      for (int ns=0;ns<2;ns++) {
        uint32_t rs = srslte_refsignal_sl_dmrs_pscch_N_rs(sl_mode, ns);
        for(int l=0; l<rs; l++) {
          int dmrs_pos = srslte_refsignal_sl_dmrs_pscch_symbol(l, sl_mode, ns, q->cell.cp);

          memcpy(&ce[SRSLTE_RE_IDX(q->cell.nof_prb, dmrs_pos+ns*nsymbols, prb_offset*SRSLTE_NRE)],
                &q->pilot_estimates[dmrs_c*nrefs_sym],
                nrefs_sym*sizeof(cf_t));

          dmrs_c++;
        }
      }

      interpolate_pilots_psxch_mode4(q, ce, prb_offset, nrefs_sym);
                     
      q->noise_estimate = 0;              
    }
  }


  q->pilot_power = srslte_vec_avg_power_cf(q->pilot_recv_signal, nrefs_sf);

  return 0;
}


int srslte_chest_sl_estimate_pscch(srslte_chest_sl_t *q, cf_t *input, cf_t *ce, srslte_sl_mode_t sl_mode, uint32_t prb_offset) {

  uint32_t prb_n = 2;

  // generate pscch dmrs
  srslte_refsignal_sl_dmrs_pscch_gen(&q->dmrs_signal, prb_n, 0, 0, q->pilot_known_signal);

  return srslte_chest_sl_estimate_psxch(q, input, ce, sl_mode, prb_offset, prb_n);
}


int srslte_chest_sl_estimate_pssch(srslte_chest_sl_t *q, cf_t *input, cf_t *ce, srslte_sl_mode_t sl_mode, uint32_t prb_offset, uint32_t prb_n) {

  // generate pssch dmrs
  // srslte_refsignal_sl_dmrs_pssch_gen(&q->dmrs_signal, prb_n, 0, 0, q->pilot_known_signal);

  return srslte_chest_sl_estimate_psxch(q, input, ce, sl_mode, prb_offset, prb_n);
}

