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

/**********************************************************************************************
 *  File:         ch_awgn.h
 *
 *  Description:  Additive white gaussian noise channel object
 *
 *  Reference:
 *********************************************************************************************/

#include <complex.h>
#include <stdint.h>

#include "srslte/config.h"

#ifndef SRSLTE_CH_AWGN_H
#define SRSLTE_CH_AWGN_H

SRSLTE_API void srslte_ch_awgn_c(const cf_t* input, 
                                 cf_t* output, 
                                 float variance, 
                                 uint32_t len);

SRSLTE_API void srslte_ch_awgn_f(const float* x, 
                                 float* y, 
                                 float variance, 
                                 uint32_t len);

SRSLTE_API float srslte_ch_awgn_get_variance(float ebno_db, 
                                             float rate);


#endif // SRSLTE_CH_AWGN_H
