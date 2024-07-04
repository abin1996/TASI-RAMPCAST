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
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "srslte/phy/common/phy_common.h"
#include "srslte/phy/phch/repo.h"
#include "srslte/phy/utils/debug.h"
#include "srslte/phy/utils/bit.h"
#include "srslte/phy/utils/vector.h"
#include "srslte/phy/dft/dft_precoding.h"


void srslte_repo_free(srslte_repo_t *q) {

  bzero(q, sizeof(srslte_repo_t));

  if (q->subframe_rp) {
    free(q->subframe_rp);
  }
}


int srslte_repo_init(srslte_repo_t *q, srslte_cell_t cell, SL_CommResourcePoolV2X_r14 pool) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL &&
      srslte_cell_isvalid(&cell))
  {
    bzero(q, sizeof(srslte_repo_t));
    ret = SRSLTE_ERROR;

    q->cell = cell;

    q->rp = pool;

    q->subframe_rp = srslte_vec_malloc(sizeof(short) * 10240);
    if (!q->subframe_rp) {
      goto clean;
    }

    // check if max number of prbs is valid for this cell
    if(q->rp.startRB_Subchannel_r14 + q->rp.numSubchannel_r14*q->rp.sizeSubchannel_r14 > q->cell.nof_prb) {
      printf("This resourcepool configuration does not fit into this cell.\n");
      // set some default values
      q->rp.startRB_Subchannel_r14 = 0;
      q->rp.numSubchannel_r14 = q->cell.nof_prb/q->rp.sizeSubchannel_r14;
    }

    // sync symbol are expected in subframe 0
    q->syncOffsetIndicator_r12 = 0;
    // for V2X we have a periodicity of 160 subframes for the sync
    q->syncPeriod = 160;

    srslte_repo_build_resource_pool(q);

    ret = SRSLTE_SUCCESS;
  }
 clean:
  if (q) {
    if (ret != SRSLTE_SUCCESS) {
      srslte_repo_free(q);
    }
  }
  return ret;
}


/**
 * @brief Return number of bits to encode RIV/Frequency resource location of trans-
 * mission and retransmission bits of SCI-1.
 * 
 * See 36.212 5.4.3.1.2
 * 
 * @param q 
 * @return int 
 */
int srslte_repo_frl_len(srslte_repo_t *q) {
  return ceil(log2(q->rp.numSubchannel_r14*(q->rp.numSubchannel_r14+1)/2));
}
 

/**
 * @brief Encode starting subchannel index and contiguous allocated sub-channels
 * 
 * See 36.213 14.1.1.4C
 * 
 * @param q 
 * @param sci 
 * @param L_subCH 
 * @param n_subCH 
 */
void srslte_repo_encode_frl(srslte_repo_t *q, srslte_ra_sl_sci_t *sci, uint8_t L_subCH, uint8_t n_subCH) {

  if((L_subCH-1) < q->rp.numSubchannel_r14/2) {
    sci->frl = q->rp.numSubchannel_r14*(L_subCH-1) + n_subCH;
  } else {
    sci->frl = q->rp.numSubchannel_r14*(q->rp.numSubchannel_r14 - L_subCH + 1) + (q->rp.numSubchannel_r14 - 1 - n_subCH);
  }

  // save values, this is optional
  sci->frl_L_subCH = L_subCH;
  sci->frl_n_subCH = n_subCH;

  // printf("Encoded RIV from L:%d n:%d to %x\n", L_subCH, n_subCH, sci->frl);

}


/**
 * @brief Decoded starting subchannel index and contiguous allocated sub-channels
 * from SCI-1 frl value
 * 
 * See 36.213 14.1.1.4C
 * 
 * @param q 
 * @param frl 
 */
void srslte_repo_decode_frl(srslte_repo_t *q, srslte_ra_sl_sci_t *sci) {
  // we do  not know if Lsuch <= Nsubch/2 so we try first
  
  uint8_t n_subCH = sci->frl % q->rp.numSubchannel_r14;
  uint8_t L_subCH = floor((sci->frl + q->rp.numSubchannel_r14)/q->rp.numSubchannel_r14);

  // check if these recoverd value make sense
  if (n_subCH + L_subCH > q->rp.numSubchannel_r14) {
    n_subCH = (q->rp.numSubchannel_r14*q->rp.numSubchannel_r14 + 2*q->rp.numSubchannel_r14 - 1 - sci->frl) % q->rp.numSubchannel_r14;
    L_subCH   = floor((q->rp.numSubchannel_r14*q->rp.numSubchannel_r14 + 2*q->rp.numSubchannel_r14 - 1 - sci->frl) / q->rp.numSubchannel_r14);
    // validate
    if (n_subCH + L_subCH > q->rp.numSubchannel_r14) {
      printf("srslte_repo_decode_frl: Error in RIV demapping\n");
      n_subCH = 0;
      L_subCH = 0;
    }

  }

  sci->frl_L_subCH = L_subCH;
  sci->frl_n_subCH = n_subCH;

}

/**
 * @brief Decodes resource reservation from SCI-1 bit format
 * 
 * See 36.213 Table 14.2.1-1
 * 
 * @param rr value from SCI-1
 * @return Resource reservation interval
 */
uint8_t srslte_repo_decode_rr(uint32_t rr) {
  if(rr>=1 && rr<=10) {
    return rr*100;
  }

  if(rr == 11) {
    return 50;
  }

  if(rr == 12) {
    return 20;
  }

  return 0;
}

/**
 * @brief Encodes Resource reservation interval into SCI-1 format
 * 
 * See 36.213 Table 14.2.1-1
 * 
 * @param rri Resource reservation interval
 * @return rr Resource reservationin SCI-1 format
 */
uint32_t srslte_repo_encode_rr(uint8_t rri) {
  if(1*100 <= rri && rri <= 10*100){
    return rri/100;
  }

  if(rri == 50) {
    return 11;
  }

  if(rri == 20) {
    return 12;
  }

  return 0;
}

void srslte_repo_sci_encode(srslte_repo_t *q, uint8_t *sci_packed, srslte_ra_sl_sci_t *sci) {

  srslte_bit_unpack(sci->priority, &sci_packed, 3);
  srslte_bit_unpack(srslte_repo_encode_rr(sci->resource_reservation), &sci_packed, 4);
  //Frequency resource location of initial transmission and retransmission
  srslte_bit_unpack(sci->frl, &sci_packed, srslte_repo_frl_len(q));
  // Time gap between initial transmission and retransmission
  srslte_bit_unpack(sci->time_gap, &sci_packed, 4);
  // Modulation and coding scheme
  srslte_bit_unpack(sci->mcs.idx, &sci_packed, 5);
  // Retransmission index
  srslte_bit_unpack(sci->rti, &sci_packed, 1);

  // printf("ENCODED SCI-1: prio: %d rr: 0x%x frl(%d): 0x%x gap: %d mcs: %x rti: %d\n",
  //         sci->priority,
  //         sci->resource_reservation,
  //         srslte_repo_frl_len(q),
  //         sci->frl,
  //         sci->time_gap,
  //         sci->mcs.idx,
  //         sci->rti);
}


int srslte_repo_sci_decode(srslte_repo_t *q, uint8_t *sci_packed, srslte_ra_sl_sci_t *sci) {

  uint8_t *sci_end = sci_packed + 32;//SRSLTE_SCI1_MAX_BITS;

  sci->priority = srslte_bit_pack(&sci_packed, 3);
  // Resource reservation interval
  sci->resource_reservation = srslte_repo_decode_rr(srslte_bit_pack(&sci_packed, 4));
  //Frequency resource location of initial transmission and retransmission
  sci->frl = srslte_bit_pack(&sci_packed, srslte_repo_frl_len(q));
  // Time gap between initial transmission and retransmission
  sci->time_gap = srslte_bit_pack(&sci_packed, 4);
  // Modulation and coding scheme
  sci->mcs.idx = srslte_bit_pack(&sci_packed, 5);
  // Retransmission index
  sci->rti = srslte_bit_pack(&sci_packed, 1);

  // check if reserved bits are 0
  while(sci_packed < sci_end) {
    if(0 != *sci_packed) {
      printf("False decoded PSCCH\n");
      srslte_vec_fprint_hex(stdout, sci_end-32, 32+16);
      return SRSLTE_ERROR;
    }
    sci_packed++;
  }

  srslte_repo_decode_frl(q, sci);

  // printf("SCI-1 decoded: prio: %d rr: 0x%x frl: 0x%x(n: %d L: %d) gap: %d mcs: %x rti: %d\n",
  //         sci->priority,
  //         sci->resource_reservation,
  //         sci->frl,
  //         sci->frl_n_subCH,
  //         sci->frl_L_subCH,
  //         sci->time_gap,
  //         sci->mcs.idx,
  //         sci->rti);

  // @todo: check if prb size fulfills 2^a2 * 3^a3 * 5^a5 equation
  if(!srslte_dft_precoding_valid_prb(q->rp.sizeSubchannel_r14 * sci->frl_L_subCH - 2)) {
    printf("False decoded PSCCH, invalid PRB %d\n", q->rp.sizeSubchannel_r14 * sci->frl_L_subCH - 2);
    srslte_vec_fprint_hex(stdout, sci_end-32, 32+16);
    return SRSLTE_ERROR;
  }

  if(-1 == srslte_sl_fill_ra_mcs(&sci->mcs, q->rp.sizeSubchannel_r14 * sci->frl_L_subCH - 2)) {
    printf("False decoded PSCCH, invalid MCS %d\n",sci->mcs.idx);
    srslte_vec_fprint_hex(stdout, sci_end-32, 32+16);
    return SRSLTE_ERROR;
  }

  return SRSLTE_SUCCESS;
}


/**
 * @brief Build subframe pool
 * 
 * See 36.213 14.1.5
 * 
 * @param q 
 */
void srslte_repo_build_resource_pool(srslte_repo_t *q) {
  int i;

  // mark all subframes as valid t_i^SL
  for(i=0; i<10240; i++) {
    q->subframe_rp[i] = 0;
  }

  if(q->syncOffsetIndicator_r12 >= q->syncPeriod) {
    printf("Invalid combination of syncOffsetIndicator_r12:%d and syncPeriod:%d\n", q->syncOffsetIndicator_r12, q->syncPeriod);

    // change offset into a valid value
    q->syncOffsetIndicator_r12 = q->syncOffsetIndicator_r12 % q->syncPeriod;
  }

  // remove each sync symbol from pool -> l
  int valid_count = 0;
  int invalid_count = 0;
  for(i=0; i<10240; i++) {
    if(q->subframe_rp[i] != -1) {
      if((valid_count % q->syncPeriod) == q->syncOffsetIndicator_r12) {
        // mark as invalid
        q->subframe_rp[i] = -1;
        invalid_count++;
      }
      valid_count++;
    }
  }

  // step 2
  invalid_count = 0;
  valid_count = 0;
  int N_res = (10240 - 64) % q->rp.sl_Subframe_r14_len;
  int r = 0;
  for(i=0; i<10240; i++) {
    if(q->subframe_rp[i] != -1) {
      if(valid_count == r) {
        // mark as invalid
        q->subframe_rp[i] = -1;
        invalid_count++;

        // get next invalid pos
        r = floor(invalid_count*(10240 - 64)/N_res);
      }
      valid_count++;
    }
  }

  // apply bitmap
  invalid_count = 0;
  valid_count = 0;
  for(i=0; i<10240; i++) {
    if(q->subframe_rp[i] != -1) {
      if(q->rp.sl_Subframe_r14[valid_count % q->rp.sl_Subframe_r14_len] == 0) {
        // mark as invalid
        q->subframe_rp[i] = -1;
        invalid_count++;

      }
      valid_count++;
    }
  }

  // re-enumerate with sidelink subframe number
  valid_count = 0;
  for(i=0; i<10240; i++) {
    if(q->subframe_rp[i] != -1) {
      q->subframe_rp[i] = valid_count++;
    }
  }
}


/**
 * @brief Converts system subframe number to sidelink subframe number
 * 
 * See 36.211 9.2.4
 * 
 * @param q 
 * @param subframe 
 * @return uint32_t 
 */
uint32_t srslte_repo_get_n_PSSCH_ssf(srslte_repo_t *q, uint32_t subframe) {

  int k = q->subframe_rp[subframe%10240];

  // if(k == -1) {
  //   printf("ERROR: srslte_repo_get_n_PSSCH_ssf: sf %d is not in resoure pool\n", subframe);
  // }

  return k % 10;
}

/**
 * @brief Returns index k if subframe belings to resource pool
 *        See 14.1.5 36.213
 * 
 * @param q 
 * @param subframe 
 * @return int32_t 
 */
int32_t srslte_repo_get_t_SL_k(srslte_repo_t *q, uint32_t subframe) {

  return q->subframe_rp[subframe%10240];
}
