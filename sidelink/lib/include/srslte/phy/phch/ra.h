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

/*
 * Copyright 2013-2019 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
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
 *  File:         ra.h
 *
 *  Description:  Implements Resource allocation Procedures common in for DL and UL
 *
 *  Reference:    3GPP TS 36.213 version 10.0.1 Release 10
 *****************************************************************************/

#ifndef SRSLTE_RA_H
#define SRSLTE_RA_H

#include <stdint.h>
#include <stdbool.h>

#include "srslte/config.h"
#include "srslte/phy/common/phy_common.h"

/**************************************************
 * Common structures used for Resource Allocation
 **************************************************/

typedef struct SRSLTE_API {
  srslte_mod_t mod;
  int          tbs;
  int          rv;
  uint32_t     nof_bits;
  uint32_t     cw_idx;
  bool         enabled;

  // this is for debugging and metrics purposes
  uint32_t mcs_idx;
} srslte_ra_tb_t;

typedef enum SRSLTE_API {
  SRSLTE_RA_ALLOC_TYPE0 = 0,
  SRSLTE_RA_ALLOC_TYPE1 = 1,
  SRSLTE_RA_ALLOC_TYPE2 = 2
} srslte_ra_type_t;

typedef struct SRSLTE_API {
  uint32_t rbg_bitmask;
} srslte_ra_type0_t;

typedef struct SRSLTE_API {
  uint32_t vrb_bitmask;
  uint32_t rbg_subset;
  bool     shift;
} srslte_ra_type1_t;

typedef struct SRSLTE_API {
  uint32_t riv; // if L_crb==0, DCI message packer will take this value directly
  enum {
    SRSLTE_RA_TYPE2_NPRB1A_2 = 0, SRSLTE_RA_TYPE2_NPRB1A_3 = 1
  } n_prb1a;
  enum {
    SRSLTE_RA_TYPE2_NG1 = 0, SRSLTE_RA_TYPE2_NG2 = 1
  } n_gap;
  enum {
    SRSLTE_RA_TYPE2_LOC = 0, SRSLTE_RA_TYPE2_DIST = 1
  } mode;
} srslte_ra_type2_t;


/**************************************************
 * Structures used for Sidelink Resource Allocation 
 **************************************************/

// todo: (rl) this struct is now found under srslte_ra_tb_t
typedef struct SRSLTE_API {
  srslte_mod_t mod;
  uint32_t tbs;
  uint32_t idx; 
} srslte_ra_mcs_t;


/** Unpacked SCI-1 message */
typedef struct SRSLTE_API {
  uint8_t priority;
  uint8_t resource_reservation;
  //Frequency resource location of initial transmission and retransmission
  uint8_t frl;
  // Time gap between initial transmission and retransmission
  uint8_t time_gap;
  
  // Modulation and coding scheme
  // @todo: maybe this should go into a new grant struct?
  srslte_ra_mcs_t mcs;

  // Retransmission index
  uint8_t rti;

  // from frl decoded values
  uint8_t frl_n_subCH;
  uint8_t frl_L_subCH;


} srslte_ra_sl_sci_t;


SRSLTE_API uint32_t srslte_ra_type0_P(uint32_t nof_prb);

SRSLTE_API uint32_t srslte_ra_type2_n_vrb_dl(uint32_t nof_prb, bool ngap_is_1);

SRSLTE_API uint32_t srslte_ra_type2_n_rb_step(uint32_t nof_prb);

SRSLTE_API uint32_t srslte_ra_type2_ngap(uint32_t nof_prb, bool ngap_is_1);

SRSLTE_API uint32_t srslte_ra_type1_N_rb(uint32_t nof_prb);

SRSLTE_API uint32_t srslte_ra_type2_to_riv(uint32_t L_crb, uint32_t RB_start, uint32_t nof_prb);

SRSLTE_API void srslte_ra_type2_from_riv(uint32_t riv,
                                         uint32_t* L_crb,
                                         uint32_t* RB_start,
                                         uint32_t nof_prb,
                                         uint32_t nof_vrb);

SRSLTE_API int srslte_ra_tbs_idx_from_mcs(uint32_t mcs, bool is_ul);

SRSLTE_API srslte_mod_t srslte_ra_dl_mod_from_mcs(uint32_t mcs);

SRSLTE_API srslte_mod_t srslte_ra_dl_mod_from_mcs2(uint32_t mcs);

SRSLTE_API srslte_mod_t srslte_ra_ul_mod_from_mcs(uint32_t mcs);

SRSLTE_API int srslte_ra_mcs_from_tbs_idx(uint32_t tbs_idx, bool is_ul);

SRSLTE_API int srslte_ra_tbs_from_idx(uint32_t tbs_idx, uint32_t n_prb);

SRSLTE_API int srslte_ra_tbs_to_table_idx(uint32_t tbs, uint32_t n_prb);

SRSLTE_API int srslte_sl_fill_ra_mcs(srslte_ra_mcs_t *mcs, uint32_t nprb);

#endif // SRSLTE_RA_H
