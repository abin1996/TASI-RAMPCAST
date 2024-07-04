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

#ifndef SRSUE_PHY_METRICS_H
#define SRSUE_PHY_METRICS_H

#include "srslte/phy/common/phy_common.h"

namespace srsue {

struct sync_metrics_t
{
  float ta_us;
  float cfo;
  float sfo;
};

struct dl_metrics_t
{
  float n;
  float sinr;
  float rsrp;
  float rsrq;
  float rssi;
  float ri;
  float turbo_iters;
  float mcs;
  float pathloss;
  float sync_err;
};

struct ul_metrics_t
{
  float mcs;
  float power;
};

struct phy_metrics_t
{
  sync_metrics_t sync[SRSLTE_MAX_CARRIERS];
  dl_metrics_t   dl[SRSLTE_MAX_CARRIERS];
  ul_metrics_t   ul[SRSLTE_MAX_CARRIERS];
  uint32_t       nof_active_cc;
};

} // namespace srsue

#endif // SRSUE_PHY_METRICS_H
