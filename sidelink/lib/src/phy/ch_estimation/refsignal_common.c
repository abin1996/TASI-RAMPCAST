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
#include <srslte/phy/common/phy_common.h>

#include "ul_rs_tables.h"


uint32_t largest_prime_lower_than(uint32_t x) {
  /* get largest prime n_zc<len */
  for (uint32_t i = NOF_PRIME_NUMBERS - 1; i > 0; i--) {
    if (prime_numbers[i] < x) {
      return prime_numbers[i];
    }
  }
  return 0;
}

void srslte_refsignal_r_uv_arg_1prb(float *arg, uint32_t u) {
  for (int i = 0; i < SRSLTE_NRE; i++) {
    arg[i] = phi_M_sc_12[u][i] * M_PI / 4;
  }
}

static void arg_r_uv_2prb(float *arg, uint32_t u) {
  for (int i = 0; i < 2*SRSLTE_NRE; i++) {
    arg[i] = phi_M_sc_24[u][i] * M_PI / 4;
  }  
}

static uint32_t get_q(uint32_t u, uint32_t v, uint32_t N_sz) {
  float q;
  float q_hat;
  float n_sz = (float) N_sz;
  
  q_hat = n_sz *(u + 1) / 31;
  if ((((uint32_t) (2 * q_hat)) % 2) == 0) {
    q = q_hat + 0.5 + v;
  } else {
    q = q_hat + 0.5 - v;
  }
  return (uint32_t) q; 
}

static void arg_r_uv_mprb(float *arg, uint32_t M_sc, uint32_t u, uint32_t v) {

  uint32_t N_sz = largest_prime_lower_than(M_sc);
  if (N_sz > 0) {
    float q = get_q(u,v,N_sz);
    float n_sz = (float) N_sz;
    for (uint32_t i = 0; i < M_sc; i++) {
      float m = (float) (i%N_sz);
      arg[i] =  -M_PI * q * m * (m + 1) / n_sz;
    }
  }
}

/* Computes argument of r_u_v signal */
void srslte_refsignal_compute_r_uv_arg(float *arg, uint32_t nof_prb, uint32_t u, uint32_t v) {
  if (nof_prb == 1) {
    srslte_refsignal_r_uv_arg_1prb(arg, u);
  } else if (nof_prb == 2) {
    arg_r_uv_2prb(arg, u);
  } else {
    arg_r_uv_mprb(arg, SRSLTE_NRE*nof_prb, u, v);
  }
}
