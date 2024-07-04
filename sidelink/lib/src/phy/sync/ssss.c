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
#include <stdio.h>
#include <math.h>

#include "srslte/phy/sync/ssss.h"
#include "srslte/phy/dft/dft.h"
#include "srslte/phy/utils/convolution.h"
#include "srslte/phy/utils/vector.h"


/* Actually this is very similar to sss.c, so maybe we should just
 * build wrapper for each function, to avoid using a mixture of
 * srslte_sss* and srslte_ssss* function.
 * */

void generate_sss_all_tables(srslte_sss_tables_t *tables, uint32_t N_id_2);
void convert_tables(srslte_sss_fc_tables_t *fc_tables, srslte_sss_tables_t *in);
void generate_N_id_1_table(uint32_t table[30][30]);

int srslte_ssss_init(srslte_ssss_t *q, uint32_t fft_size) {
  
  if (q                 != NULL  &&
      fft_size          <= 2048)
  {
    uint32_t N_id_2;
    srslte_sss_tables_t sss_tables;


    bzero(q, sizeof(srslte_ssss_t));
    
    if (srslte_dft_plan(&q->dftp_input, fft_size, SRSLTE_DFT_FORWARD, SRSLTE_DFT_COMPLEX)) {
      srslte_sss_free(q);
      return SRSLTE_ERROR;
    }
    srslte_dft_plan_set_mirror(&q->dftp_input, true);
    srslte_dft_plan_set_dc(&q->dftp_input, false);

    q->fft_size = fft_size; 
    q->max_fft_size = fft_size;

    generate_N_id_1_table(q->N_id_1_table);
    
    for (N_id_2=0; N_id_2<2; N_id_2++) {
      generate_sss_all_tables(&sss_tables, N_id_2);
      convert_tables(&q->fc_tables[N_id_2], &sss_tables);
    }
    q->N_id_2 = 0;
    return SRSLTE_SUCCESS;
  } 
  return SRSLTE_ERROR_INVALID_INPUTS;
}

void srslte_ssss_free(srslte_ssss_t *q) {
  srslte_sss_free(q);
}


/** Sets the N_id_2 to search for */
int srslte_ssss_set_N_id_2(srslte_ssss_t *q, uint32_t N_id_2) {
  if (!srslte_SL_N_id_2_isvalid(N_id_2)) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return SRSLTE_ERROR;
  } else {
    q->N_id_2 = N_id_2;
    return SRSLTE_SUCCESS;
  }
}

int srslte_ssss_resize(srslte_ssss_t *q, uint32_t fft_size) {
  return srslte_sss_resize(q, fft_size);
}

/**
 * @brief 
 * 
 * @param signal 
 * @param cell_id 
 */
void srslte_ssss_generate(float *signal, uint32_t cell_id) {

  uint32_t i;
  uint32_t id1 = cell_id % 168;
  uint32_t id2 = cell_id / 168;
  uint32_t m0;
  uint32_t m1;
  int s_t[SRSLTE_SSS_N], c_t[SRSLTE_SSS_N], z_t[SRSLTE_SSS_N];
  int s0[SRSLTE_SSS_N], s1[SRSLTE_SSS_N], c0[SRSLTE_SSS_N], c1[SRSLTE_SSS_N], z1_0[SRSLTE_SSS_N], z1_1[SRSLTE_SSS_N];

  generate_m0m1(id1, &m0, &m1);
  generate_zsc_tilde(z_t, s_t, c_t);

  generate_s(s0, s_t, m0);
  generate_s(s1, s_t, m1);

  generate_c(c0, c_t, id2, 0);
  generate_c(c1, c_t, id2, 1);

  generate_z(z1_0, z_t, m0);
  generate_z(z1_1, z_t, m1);

  for (i = 0; i < SRSLTE_SSS_N; i++) {
    /** Even Resource Elements: Sub-frame 5*/
    signal[2 * i] = (float) (s1[i] * c0[i]);
    /** Odd Resource Elements: Sub-frame 5*/
    signal[2 * i + 1] = (float) (s0[i] * c1[i] * z1_1[i]);
  }
}

/**
 * @brief Mapping of SSSS to resource elements 36.211 14.2 section 9.7.2.2
 * 
 * @todo: Apply amplitude scaling, either here or during generation?
 * 
 * @todo: This is only valid for short CP
 * 
 * @param ssss      real part of SSSS
 * @param sf        start of subframe in which SSSS is placed
 * @param nof_prb   nuber of prb
 * @param cp        CP len
 */
void srslte_ssss_put_sf(float *ssss, cf_t *sf, uint32_t nof_prb, srslte_cp_t cp) {
  uint32_t i, k;

  k = 11 * nof_prb * SRSLTE_NRE + nof_prb * SRSLTE_NRE / 2 - 31;
  memset(&sf[k - 5], 0, 5 * sizeof(cf_t));
  for (i = 0; i < SRSLTE_SSSS_LEN; i++) {
    __real__ sf[k + i] = ssss[i];
    __imag__ sf[k + i] = 0;
  }
  memset(&sf[k + SRSLTE_SSSS_LEN], 0, 5 * sizeof(cf_t));

  // next symbol
  k += nof_prb * SRSLTE_NRE;
  memset(&sf[k - 5], 0, 5 * sizeof(cf_t));
  for (i = 0; i < SRSLTE_SSSS_LEN; i++) {
    __real__ sf[k + i] = ssss[i];
    __imag__ sf[k + i] = 0;
  }
  memset(&sf[k + SRSLTE_SSSS_LEN], 0, 5 * sizeof(cf_t));
}
