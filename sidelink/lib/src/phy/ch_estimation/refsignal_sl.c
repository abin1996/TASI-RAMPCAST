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

#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <complex.h>

#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/ch_estimation/refsignal_sl.h"
#include "srslte/phy/ch_estimation/refsignal_common.h"
#include "srslte/phy/utils/vector.h"
#include "srslte/phy/utils/debug.h"
#include "srslte/phy/common/sequence.h"
#include "srslte/phy/dft/dft_precoding.h"


uint32_t psbch_dmrs_symbol_mode4_slot0_cpnorm[2] = {4, 6};
uint32_t psbch_dmrs_symbol_mode4_slot1_cpnorm[1] = {2};


uint32_t pscch_dmrs_symbol_mode4_slot0_cpnorm[2] = {2, 5};
uint32_t pscch_dmrs_symbol_mode4_slot1_cpnorm[2] = {1, 4};


/**
 * @brief Calculates alpha according to 9.8 and 5.5.2.1.1 of 36.211
 * 
 * n_cs only depends on N_id_SL
 * 
 * @param q 
 * @return float 
 */
static float psbch_alpha(srslte_refsignal_sl_t *q) 
{
  uint32_t n_cs = (q->cell.id/2) % 8;
  
  return 2 * M_PI * (n_cs) / 12;

}


/**
 * @brief Calculates alpha according to 9.8 and 5.5.2.1.1 of 36.211
 * 
 * n_cs only depends on N_id_SL
 * 
 * @param q 
 * @return float 
 */
static float pscch_alpha(srslte_refsignal_sl_t *q) 
{
  //@todo: (rl) n_cs is choosen between {0,3,6,9}, is it random?
  uint32_t n_cs = 0;
  
  return 2 * M_PI * (n_cs) / 12;

}



#if 0
/**
 * @brief Retrieves DMRS from a subframe according to 9.8 and 5.5.2.1.2 of 36.211
 * 
 * The number of symobls with DMRS within a subframe varies with sidelink transmittion
 * mode and physical channel.
 * 
 * 
 * 
 * @param q 
 * @param sf_symbols 
 * @param nof_prb 
 * @param n_prb 
 * @param r_pusch 
 */
void srslte_refsignal_sl_dmrs_pusch_get(srslte_refsignal_sl_t *q, cf_t *sf_symbols, uint32_t nof_prb, uint32_t n_prb[2], cf_t *r_pusch) 
{
  for (uint32_t ns_idx=0;ns_idx<2;ns_idx++) {
    INFO("Getting DRMS from n_prb: %d, L: %d, ns_idx: %d\n", n_prb[ns_idx], nof_prb, ns_idx);
    uint32_t L = SRSLTE_REFSIGNAL_UL_L(ns_idx, q->cell.cp);
    memcpy(&r_pusch[ns_idx*SRSLTE_NRE*nof_prb],
           &sf_symbols[SRSLTE_RE_IDX(q->cell.nof_prb, L, n_prb[ns_idx]*SRSLTE_NRE)], 
           nof_prb*SRSLTE_NRE*sizeof(cf_t));    
  }
}
#endif









/**
 * @brief Returns the number of DMRS symbols in a sidelink slot
 * 
 * @param mode sidlink mode
 * @param slot_idx 0 for first slot, 1 for second slot of a subframe
 * @return uint32_t 
 */
uint32_t srslte_refsignal_sl_dmrs_psbch_N_rs(srslte_sl_mode_t mode, uint32_t slot_idx) {
  switch (mode) {
    case SRSLTE_SL_MODE_1:
    case SRSLTE_SL_MODE_2:
      return 1;
    case SRSLTE_SL_MODE_3:
    case SRSLTE_SL_MODE_4:
      // first slot in a subframe has two dmrs symbols, second only one
      return (slot_idx % 2) == 0 ? 2 : 1;
    default:
      fprintf(stderr, "Unsupported sidelink mode %d\n", mode);
      return 0; 
  }
  return 0; 
}


/**
 * @brief Returns the number of DMRS symbols in a sidelink slot
 * 
 * @param mode sidlink mode
 * @param slot_idx 0 for first slot, 1 for second slot of a subframe
 * @return uint32_t 
 */
uint32_t srslte_refsignal_sl_dmrs_pscch_N_rs(srslte_sl_mode_t mode, uint32_t slot_idx) {
  switch (mode) {
    case SRSLTE_SL_MODE_1:
    case SRSLTE_SL_MODE_2:
      return 1;
    case SRSLTE_SL_MODE_3:
    case SRSLTE_SL_MODE_4:
      return 2;
    default:
      fprintf(stderr, "Unsupported sidelink mode %d\n", mode);
      return 0; 
  }
  return 0; 
}




/**
 * @brief Get symbol position of m'th DMRS
 * 
 * @param m 
 * @param mode 
 * @param slot 
 * @param cp 
 * @return uint32_t 
 */
uint32_t srslte_refsignal_sl_dmrs_psbch_symbol(uint32_t m, srslte_sl_mode_t mode, uint32_t slot, srslte_cp_t cp) {
  switch (mode) {
    case SRSLTE_SL_MODE_3:
    case SRSLTE_SL_MODE_4:
      if (SRSLTE_CP_ISNORM(cp)) {
        if(slot == 0 && m < srslte_refsignal_sl_dmrs_psbch_N_rs(mode, slot))
        {
          return psbch_dmrs_symbol_mode4_slot0_cpnorm[m];
        }
        
        if(slot == 1 && m < srslte_refsignal_sl_dmrs_psbch_N_rs(mode, slot))
        {
          return psbch_dmrs_symbol_mode4_slot1_cpnorm[m];
        }

      }
    default:
        fprintf(stderr, "Unsupported SL mode %d\n", mode);
        return 0; 
    }
    return 0; 
}


/**
 * @brief Get symbol position of m'th DMRS
 * 
 * @param m 
 * @param mode 
 * @param slot 
 * @param cp 
 * @return uint32_t 
 */
uint32_t srslte_refsignal_sl_dmrs_pscch_symbol(uint32_t m, srslte_sl_mode_t mode, uint32_t slot, srslte_cp_t cp) {
  switch (mode) {
    case SRSLTE_SL_MODE_3:
    case SRSLTE_SL_MODE_4:
      if (SRSLTE_CP_ISNORM(cp)) {
        if(slot == 0 && m < srslte_refsignal_sl_dmrs_pscch_N_rs(mode, slot))
        {
          return pscch_dmrs_symbol_mode4_slot0_cpnorm[m];
        }
        
        if(slot == 1 && m < srslte_refsignal_sl_dmrs_pscch_N_rs(mode, slot))
        {
          return pscch_dmrs_symbol_mode4_slot1_cpnorm[m];
        }

      }
    default:
        fprintf(stderr, "Unsupported SL mode %d\n", mode);
        return 0; 
    }
    return 0; 
}


int srslte_refsignal_sl_dmrs_psbch_cp(srslte_refsignal_sl_t *q, srslte_sl_mode_t sl_mode, cf_t *source, cf_t *dest, bool source_is_grid) 
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS; 
  if (q && source && dest) {
    ret = SRSLTE_ERROR; 
    uint32_t nsymbols = SRSLTE_CP_ISNORM(q->cell.cp)?SRSLTE_CP_NORM_NSYMB:SRSLTE_CP_EXT_NSYMB;
        
    uint32_t N_rs;

    for (uint32_t ns=0;ns<2;ns++) {
      N_rs = srslte_refsignal_sl_dmrs_psbch_N_rs(sl_mode, ns);

      for (uint32_t i=0;i<N_rs;i++) {
        uint32_t l = srslte_refsignal_sl_dmrs_psbch_symbol(i, sl_mode, ns, q->cell.cp);

        if (!source_is_grid) {
          memcpy(&dest[SRSLTE_RE_IDX(q->cell.nof_prb, l+ns*nsymbols, q->cell.nof_prb*SRSLTE_NRE/2 - 36)], 
                source,
                6*SRSLTE_NRE*sizeof(cf_t));
          source += 6*SRSLTE_NRE;
        } else {
          memcpy(dest,
                 &source[SRSLTE_RE_IDX(q->cell.nof_prb, l+ns*nsymbols, q->cell.nof_prb*SRSLTE_NRE/2 - 36)], 
                 6*SRSLTE_NRE*sizeof(cf_t));
          dest += 6*SRSLTE_NRE;
        }
      }
    }
    
    ret = SRSLTE_SUCCESS; 
  }
  return ret;     
}

/* Maps PUCCH DMRS to the physical resources as defined in 5.5.2.2.2 in 36.211 */
int srslte_refsignal_sl_dmrs_psbch_put(srslte_refsignal_sl_t *q, srslte_sl_mode_t sl_mode, cf_t *r_psbch, cf_t *output) 
{
  return srslte_refsignal_sl_dmrs_psbch_cp(q, sl_mode, r_psbch, output, false);
}

/* Gets PUCCH DMRS from the physical resources as defined in 5.5.2.2.2 in 36.211 */
int srslte_refsignal_sl_dmrs_psbch_get(srslte_refsignal_sl_t *q, srslte_sl_mode_t sl_mode, cf_t *input, cf_t *r_psbch) 
{
  return srslte_refsignal_sl_dmrs_psbch_cp(q, sl_mode, input, r_psbch, true);
}

/* Computes r sequence */
static void compute_r(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t ns, uint32_t delta_ss) {
  // Get group hopping number u 
  uint32_t f_gh=0; 
  // if (q->pusch_cfg.group_hopping_en) {
  //   fprintf(stderr, "Group hopping is not defined in sidelink.\n");
  //   f_gh = q->f_gh[ns];
  // }

  // f_ss changes with channel type table 9.8-1/2/3 in 36.211
  uint32_t f_ss = (q->cell.id / 16) % 30; // PSBCH


  uint32_t u = (f_gh + f_ss)%30;

  // Get sequence hopping number v 
  // in sidelink sequence hoppping is disabled for all physical channels
  uint32_t v = 0;

  // if(q->pusch_cfg.sequence_hopping_en) {
  //   fprintf(stderr, "Sequence hopping is not defined in sidelink.\n");
  // }

  // Compute signal argument 
  srslte_refsignal_compute_r_uv_arg(q->tmp_arg, nof_prb, u, v);

}





static int srslte_refsignal_sl_dmrs_psxch_cp(srslte_refsignal_sl_t *q, srslte_sl_mode_t sl_mode, uint32_t prb_offset, uint32_t N_prbs, cf_t *source, cf_t *dest, bool source_is_grid) 
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS; 
  if (q && source && dest) {
    ret = SRSLTE_ERROR; 
    uint32_t nsymbols = SRSLTE_CP_ISNORM(q->cell.cp)?SRSLTE_CP_NORM_NSYMB:SRSLTE_CP_EXT_NSYMB;

    uint32_t N_rs;

    for (uint32_t ns=0;ns<2;ns++) {
      N_rs = srslte_refsignal_sl_dmrs_pscch_N_rs(sl_mode, ns);

      for (uint32_t i=0;i<N_rs;i++) {
        uint32_t l = srslte_refsignal_sl_dmrs_pscch_symbol(i, sl_mode, ns, q->cell.cp);

        if (!source_is_grid) {
          memcpy(&dest[SRSLTE_RE_IDX(q->cell.nof_prb, l+ns*nsymbols, prb_offset*SRSLTE_NRE)], 
                source,
                N_prbs*SRSLTE_NRE*sizeof(cf_t));
          source += N_prbs*SRSLTE_NRE;
        } else {
          memcpy(dest,
                 &source[SRSLTE_RE_IDX(q->cell.nof_prb, l+ns*nsymbols, prb_offset*SRSLTE_NRE)], 
                 N_prbs*SRSLTE_NRE*sizeof(cf_t));
          dest += N_prbs*SRSLTE_NRE;
        }
      }
    }
    
    ret = SRSLTE_SUCCESS; 
  }
  return ret;     
}

/* Maps PSCCH DMRS to the physical resources as defined in 5.5.2.2.2 in 36.211 */
int srslte_refsignal_sl_dmrs_psxch_put(srslte_refsignal_sl_t *q, srslte_sl_mode_t sl_mode, uint32_t prb_offset, uint32_t N_prbs, cf_t *r_pscch, cf_t *output) 
{
  return srslte_refsignal_sl_dmrs_psxch_cp(q, sl_mode, prb_offset, N_prbs, r_pscch, output, false);
}

/* Gets PSCCH DMRS from the physical resources as defined in 5.5.2.2.2 in 36.211 */
int srslte_refsignal_sl_dmrs_psxch_get(srslte_refsignal_sl_t *q, srslte_sl_mode_t sl_mode, uint32_t prb_offset, uint32_t N_prbs, cf_t *input, cf_t *r_pscch) 
{
  return srslte_refsignal_sl_dmrs_psxch_cp(q, sl_mode, prb_offset, N_prbs, input, r_pscch, true);
}


/* Computes r sequence */
static void compute_r_pscch(srslte_refsignal_sl_t *q, uint32_t nof_prb) {
  // Get group hopping number u 
  uint32_t f_gh=0; 
  // if (q->pusch_cfg.group_hopping_en) {
  //   fprintf(stderr, "Group hopping is not defined in sidelink.\n");
  // }

  // f_ss changes with channel type table 9.8-1/2/3 in 36.211
  uint32_t f_ss = 8; // PSBCH

  // 5.5.1.3 group hopping
  uint32_t u = (f_gh + f_ss)%30;

  // Get sequence hopping number v 
  // in sidelink sequence hoppping is disabled for all physical channels
  uint32_t v = 0;

  // if(q->pusch_cfg.sequence_hopping_en) {
  //   fprintf(stderr, "Sequence hopping is not defined in sidelink.\n");
  // }

  // Compute signal argument 
  srslte_refsignal_compute_r_uv_arg(q->tmp_arg, nof_prb, u, v);

}




// orthogonal sequences for PSBCH Table 9.8-3
// calculate exp(I*w_arg) to get values from table
float w_arg_psbch_mode_1_2[2][2][1] = {{{0}, {0}}, // N_ID_SL % 2 == 0
                                        {{0}, {M_PI}}}; // N_ID_SL % 2 == 1

// values (0) are not used
float w_arg_psbch_mode_3_4[2][2][2] = {{{0, 0}, {0, (0)}}, // N_ID_SL % 2 == 0
                                        {{0, M_PI}, {0, (0)}}}; // N_ID_SL % 2 == 1

float w_arg_pscch_mode_3_4[2][2] = {{0, 0}, {0, 0}};

int srslte_refsignal_sl_dmrs_psbch_gen(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t sf_idx, 
                                    uint32_t cyclic_shift_for_dmrs, cf_t *r_pusch) 
{

  // todo: nof_prb must be 6 !?

  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  //if (srslte_refsignal_dmrs_pusch_cfg_isvalid(q, &q->pusch_cfg, nof_prb)) {
  {
    ret = SRSLTE_ERROR;

    // @todo: mode should be stored in some struct
    srslte_sl_mode_t sl_mode = SRSLTE_SL_MODE_4;

    uint32_t N_rs;
    
    for (uint32_t ns=2*sf_idx;ns<2*(sf_idx+1);ns++) {
      
      N_rs = srslte_refsignal_sl_dmrs_psbch_N_rs(sl_mode, ns);

      for(uint32_t m=0; m<N_rs; m++) {

        //compute_r(q, nof_prb, ns, q->pusch_cfg.delta_ss);
        compute_r(q, nof_prb, ns, NULL);
        
        // Add cyclic prefix alpha
        float alpha = psbch_alpha(q);

        float *w_arg = NULL;

        switch(sl_mode) {
          case SRSLTE_SL_MODE_1:
          case SRSLTE_SL_MODE_2:
            w_arg = w_arg_psbch_mode_1_2[q->cell.id % 2][ns%2];
            break;
          case SRSLTE_SL_MODE_3:
          case SRSLTE_SL_MODE_4:
            w_arg = w_arg_psbch_mode_3_4[q->cell.id % 2][ns%2];
            break;
          default:
            fprintf(stderr, "Unsupported sidelink mode %d\n", sl_mode);
            return SRSLTE_ERROR; 
        }


        // Do complex exponential
        // @todo: adjust amplitude
        for (int i=0;i<SRSLTE_NRE*nof_prb;i++) {
          //r_pusch[(ns%2)*SRSLTE_NRE*nof_prb+i] = cexpf(I*(w_arg[m] + q->tmp_arg[i] + alpha*i));
          *r_pusch++ = cexpf(I*(w_arg[m] + q->tmp_arg[i] + alpha*i));
        }
      }
    }
    ret = 0; 
  }
  return ret; 
}

int srslte_refsignal_sl_dmrs_pscch_gen(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t sf_idx, 
                                    uint32_t cyclic_shift_for_dmrs, cf_t *r_pscch) 
{

  // todo: nof_prb must be 2 !?

  int ret = SRSLTE_ERROR_INVALID_INPUTS;
  //if (srslte_refsignal_dmrs_pusch_cfg_isvalid(q, &q->pusch_cfg, nof_prb)) {
  {
    ret = SRSLTE_ERROR;

    // @todo: mode should be stored in some struct
    srslte_sl_mode_t sl_mode = SRSLTE_SL_MODE_4;

    uint32_t N_rs;
    
    // @ todo: we are independent of subframe number!?
    for (uint32_t ns=2*sf_idx;ns<2*(sf_idx+1);ns++) {
      
      N_rs = srslte_refsignal_sl_dmrs_pscch_N_rs(sl_mode, ns);

      for(uint32_t m=0; m<N_rs; m++) {

        compute_r_pscch(q, nof_prb);
        
        // Add cyclic prefix alpha
        float alpha = pscch_alpha(q);

        float *w_arg = NULL;

        switch(sl_mode) {
          case SRSLTE_SL_MODE_3:
          case SRSLTE_SL_MODE_4:
            w_arg = w_arg_pscch_mode_3_4[ns%2];
            break;

          case SRSLTE_SL_MODE_1:
          case SRSLTE_SL_MODE_2:
          default:
            fprintf(stderr, "Unsupported sidelink mode %d\n", sl_mode);
            return SRSLTE_ERROR; 
        }


        // Do complex exponential
        // @todo: adjust amplitude
        for (int i=0;i<SRSLTE_NRE*nof_prb;i++) {
          //r_pusch[(ns%2)*SRSLTE_NRE*nof_prb+i] = cexpf(I*(w_arg[m] + q->tmp_arg[i] + alpha*i));
          *r_pscch++ = cexpf(I*(w_arg[m] + q->tmp_arg[i] + alpha*i));
        }
      }
    }
    ret = 0; 
  }
  return ret; 
}



/**
 * @brief Calculates alpha for sidelink PSSCH
 * 
 * @param q 
 * @param n_ID_x 
 * @return float 
 */
static float pssch_alpha(srslte_refsignal_sl_t *q, uint32_t n_ID_x)
{
  //Cyclic shift is calculated according to table 9.8-1 in 36.211 release 14
  uint32_t n_cs = (n_ID_x/2) % 8;

  return 2 * M_PI * (n_cs) / 12;
}



/** Computes sequence-group pattern f_gh according to 5.5.1.3 of 36.211,
 *  SRSLT_NSLOTS_X_FRAME should be changed with nPSSCHSS
 **/
static int srslte_group_hopping_f_gh_pssch(uint32_t f_gh[SRSLTE_NSLOTS_X_FRAME], uint32_t n_id_RS) {
  srslte_sequence_t seq; 
  bzero(&seq, sizeof(srslte_sequence_t));
  
  if (srslte_sequence_LTE_pr(&seq, 160, n_id_RS / 30)) {
    return SRSLTE_ERROR;
  }
  
  for (uint32_t ns=0;ns<SRSLTE_NSLOTS_X_FRAME;ns++) {
    f_gh[ns] = 0;
    for (int i = 0; i < 8; i++) {
      f_gh[ns] += (((uint32_t) seq.c[8 * ns + i]) << i);
    }
    f_gh[ns] = f_gh[ns] % 30;
  }

  srslte_sequence_free(&seq);
  return SRSLTE_SUCCESS;
}




/* Computes r sequence */
void compute_r_pssch(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t ns, uint32_t n_ID_RS) {

  // Get group hopping pattern
  uint32_t f_gh = q->f_gh[ns];

  //perform Group hopping (only option for pssch)
  uint32_t u = (f_gh + (n_ID_RS / 16) % 30) % 30;

  // Get sequence hopping number v 
  // in sidelink sequence hoppping is disabled for all physical channels
  uint32_t v = 0;

  // if(q->pusch_cfg.sequence_hopping_en) {
  //   fprintf(stderr, "Sequence hopping is not defined in sidelink.\n");
  // }

  // Compute signal argument 
  srslte_refsignal_compute_r_uv_arg(q->tmp_arg, nof_prb, u, v);
}


// argument of orthogonal sequence for pssch table 9.8-1
const float w_arg_pssch_mode_3_4[2][2][2] = {{{0, 0}, {0, (0)}}, // N_ID_X % 2 == 0
                                              {{0, M_PI}, {0, M_PI}}}; // N_ID_X % 2 == 1


int getnPSSCHSS_feron(int n_PSSCH_ssf, int slot) {
  // feron workaround is to set n_PSSCH_ss equal for both slots and dont double nPSSCHssf
  return 1*n_PSSCH_ssf + 0;
}

int getnPSSCHSS_matlab(int n_PSSCH_ssf, int slot) {
  // matlab workaround is to set n_PSSCH_ssf = 0
  return 2*0 + slot;
}

int getnPSSCHSS_3gpp(int n_PSSCH_ssf, int slot) {
  // this is what the documents 36.211 says
  return 2*n_PSSCH_ssf + slot;
}

/* Generate DRMS for PSSCH signal */
int srslte_refsignal_sl_dmrs_pssch_gen_multi(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t n_PSSCH_ssf,
                                        uint32_t n_X_ID, cf_t *r_pssch, int (*getnPSSCHSS)(int,int))
{
  // @todo: mode should be stored in some struct
  srslte_sl_mode_t sl_mode = SRSLTE_SL_MODE_4;

  // number of current slot in the subframe pool [0..19], derived from n_PSSCH_ssf
  int n_PSSCH_ss;

  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  {//if (srslte_refsignal_dmrs_pssch_cfg_isvalid(q, &q->pusch_cfg, nof_prb)) {
    ret = SRSLTE_ERROR;

    // generate f_gh as it depend on CRC and therefor must be generated for each subframe
    srslte_group_hopping_f_gh_pssch(q->f_gh, n_X_ID);

    // generate cyclic shift value, which is constant for one subframe
    float alpha = pssch_alpha(q, n_X_ID);

    // iterate over both slots
    for (uint32_t slot=0; slot<2; slot++){

      float const *w_arg = NULL;

      switch(sl_mode) {
        case SRSLTE_SL_MODE_3:
        case SRSLTE_SL_MODE_4:
          w_arg = w_arg_pssch_mode_3_4[n_X_ID % 2][slot%2];
          break;
        case SRSLTE_SL_MODE_1:
        case SRSLTE_SL_MODE_2:
        default:
          fprintf(stderr, "Unsupported sidelink mode %d\n", sl_mode);
          return SRSLTE_ERROR; 
      }

      // call provided function to generate the sidelink slot for group hopping
      n_PSSCH_ss = getnPSSCHSS(n_PSSCH_ssf, slot);

      // iterate over both DMRS symbols per slot
      // @todo: actually need to increase slot number
      for (uint32_t n_s=2*n_PSSCH_ss; n_s<=2*n_PSSCH_ss+1; n_s++) {
        
        compute_r_pssch(q, nof_prb, n_s, n_X_ID);

        // Do complex exponential and adjust amplitude, the formula is different from that in release 8.
        // Refer to table 9.8-1 in release 14
        for (int i=0;i<SRSLTE_NRE*nof_prb;i++) {
          *r_pssch++ = cexpf(I*(w_arg[n_s%2] + q->tmp_arg[i] + alpha*i));
        }      
      }
    }
    ret = 0; 
  }
  return ret; 
}


/**
 * @brief Default PSSCH dmrs generation function
 * 
 * @Note: This has been added to account for the various DMRS generations function
 *        we found, which only differ by n_PSSCH_ss.
 * 
 * @param q 
 * @param nof_prb 
 * @param n_PSSCH_ssf 
 * @param n_X_ID 
 * @param r_pssch 
 * @return int 
 */
int srslte_refsignal_sl_dmrs_pssch_gen(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t n_PSSCH_ssf,
                                              uint32_t n_X_ID, cf_t *r_pssch)
{
  return srslte_refsignal_sl_dmrs_pssch_gen_multi(q, nof_prb, n_PSSCH_ssf, n_X_ID, r_pssch, getnPSSCHSS_matlab);
}


int srslte_refsignal_sl_dmrs_pssch_gen_feron(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t n_PSSCH_ssf,
                                              uint32_t n_X_ID, cf_t *r_pssch)
{
  return srslte_refsignal_sl_dmrs_pssch_gen_multi(q, nof_prb, n_PSSCH_ssf, n_X_ID, r_pssch, getnPSSCHSS_feron);
}

int srslte_refsignal_sl_dmrs_pssch_gen_matlab(srslte_refsignal_sl_t *q, uint32_t nof_prb, uint32_t n_PSSCH_ssf,
                                              uint32_t n_X_ID, cf_t *r_pssch)
{
  return srslte_refsignal_sl_dmrs_pssch_gen_multi(q, nof_prb, n_PSSCH_ssf, n_X_ID, r_pssch, getnPSSCHSS_matlab);
}

