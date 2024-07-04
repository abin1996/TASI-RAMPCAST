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
 *  File:         ue_sl_sensing_sps.h
 *
 *  Description:  Handles Sensing-SPS for Sidelink Transmission Mode 4
 *
 *  Reference:
 *****************************************************************************/

#include "srslte/srslte.h"

#ifndef SRSLTE_SL_SENSING_SPS_H
#define SRSLTE_SL_SENSING_SPS_H

/*
SL-CommTxPoolSensingConfig-r14 ::= SEQUENCE {
   pssch-TxConfigList-r14 SL-PSSCH-TxConfigList-r14,
   thresPSSCH-RSRP-List-r14 SL-ThresPSSCH-RSRP-List-r14,
   restrictResourceReservationPeriod-r14 SL-RestrictResourceReservationPeriodList-r14 OPTIONAL, -- Need OR
   probResourceKeep-r14 ENUMERATED {v0, v0dot2, v0dot4, v0dot6, v0dot8,
      spare3,spare2, spare1},
   p2x-SensingConfig-r14 SEQUENCE {
      minNumCandidateSF-r14 INTEGER (1..13),
      gapCandidateSensing-r14 BIT STRING (SIZE (10))
      } OPTIONAL, -- Need OR
   sl-ReselectAfter-r14 ENUMERATED {n1, n2, n3, n4, n5, n6, n7, n8, n9,
      spare7, spare6, spare5, spare4, spare3, spare2,
      spare1} OPTIONAL -- Need OR
}

SL-ThresPSSCH-RSRP Value 0 corresponds to minus infinity dBm,
                    value 1 corresponds to -128dBm, 
                    value 2 corresponds to -126dBm, 
                    value n corresponds to (-128 + (n-1)*2) dBm and so on, 
                    value 66 corresponds to infinity dBm.
*/

const uint32_t size_thresPSSCH_RSRP_List_r14 = 64;
const uint32_t maxReservationPeriod_r14 = 16;

typedef struct {
  // pssch-TxConfigList-r14
  float thresPSSCH_RSRP_List_r14[size_thresPSSCH_RSRP_List_r14]; // In dBm
  float restrictResourceReservationPeriod_r14[maxReservationPeriod_r14];
  float probResourceKeep_r14;
  // p2x-SensingConfig-r14
  uint8_t sl_ReselectAfter_r14;
} SL_CommTxPoolSensingConfig_r14;

// The standards support a sensing window of up to 1000ms
// We allow up to 1024 to allow us to index into the
// arrays using mod TTI
#define MAX_SENSING_WINDOW 1024
#define MAX_SCIS_IN_TTI 50

#define MAX_SELECTION_WINDOW 100
#define MAX_SUBCHANNELS 100     // @todo

// The maximum number of candidate resources in a subframe
#define MAX_CANDIDATE_RESOURCES (MAX_SUBCHANNELS)

namespace srslte {

class SensingSPS;

typedef struct {
  uint32_t subChannelStart;
  uint32_t numSubChannels;
  uint32_t rsvp;
  uint32_t priority;
  float    rsrp;
} SensingSCI;

struct ReservationResource {
  uint32_t rsvpOffset;
  uint32_t subchannelStart;
  uint32_t numSubchannels;
  uint32_t rsvp;
  uint32_t sl_gap;
  bool is_retx;
};

struct Reservation {
  bool active;

  uint32_t rsvp;
  uint32_t Cresel;

  uint32_t startRsvpTti;

  // we use these variables to determine, which tti belongs to the current reservation
  uint32_t _startRsvpTti;
  uint32_t _Cresel;

  uint32_t numResources;
  ReservationResource resources[2]; // @todo initially just two resources per reservation period
};

typedef enum {
  CANDIDATE_INVALID = 0,
  CANDIDATE_VALID,
  CANDIDATE_UNMONITORED,
  CANDIDATE_SCI_RSRP_THRESH,
  CANDIDATE_RSSI,
  CANDIDATE_CHOSEN,
} CandidateResourceStatus;

typedef struct {
  CandidateResourceStatus     valid;
  uint32_t subChannelStart;
} CandidateResource;

class CandidateResources {
  public:
    CandidateResources() { }

    CandidateResources(uint32_t _windowSize,uint32_t _LSubCh,uint32_t _NSubCh);
 
    CandidateResources(const CandidateResources &other);

    uint32_t remove(uint32_t idx, CandidateResourceStatus reason);

    uint32_t remove(uint32_t idx, uint32_t channel, CandidateResourceStatus reason);

    uint32_t remove(uint32_t idx, uint32_t channel, uint32_t len, CandidateResourceStatus reason);

    uint32_t size() { return setSize; }

    void print(bool withRssi = false);

    void handleSrssi(SensingSPS* sps, uint32_t sensingWindowStart, uint32_t sensingWindowEnd, uint32_t t1, uint32_t t2, uint32_t MTotal);

    void random(uint32_t& selectionWindowOffset, uint32_t& subchannelStart, uint32_t& numSubchannels);
    void random_with_retx(uint32_t& selectionWindowOffset, uint32_t& subchannelStart, uint32_t& numSubchannels, uint32_t& sl_gap);

  private:
    uint32_t windowSize;
    uint32_t LSubCh;
    uint32_t NSubCh;
    uint32_t setSize;

    uint32_t          numResources[MAX_SELECTION_WINDOW];
    CandidateResource resources[MAX_SELECTION_WINDOW][MAX_CANDIDATE_RESOURCES];

    // for print() purposes only:
    float avgRssi[MAX_SELECTION_WINDOW][MAX_CANDIDATE_RESOURCES];
};

// @todo at the moment this only supports adjacent PSCCH transmissions

class SensingSPS
{
public:
  static const uint32_t PStep = 100;

  SensingSPS(SL_CommResourcePoolV2X_r14*     _resourcePool,
             SL_CommTxPoolSensingConfig_r14* _sensingConfig,
             uint32_t                        _sensingWindowSize);

  float getThreshold(uint32_t prioTx, uint32_t prioRx) const;

  void tick(uint32_t tti);

  static uint32_t tti_add(uint32_t tti, int32_t add) {
    int32_t sum = (int32_t)tti + add;
    while (sum>=10240) sum-=10240;
    while (sum<0) sum+=10240;
    return (uint32_t)sum;
  }

  void addSCI(uint32_t tti,
              uint32_t subChannelStart,
              uint32_t numSubChannels,
              uint32_t rsvp,
              uint32_t priority,
              float    rsrp);

  void addAverageSRSSI(uint32_t tti,float sRssi);
  void addChannelSRSSI(uint32_t tti,uint32_t channel,float sRssi);

  CandidateResources resourceSelection(uint32_t tti, uint32_t t1, uint32_t t2, uint32_t LSubCh, uint32_t prioTx, uint32_t Cresel, uint32_t PrsvpTx);
  ReservationResource* schedule(uint32_t tti, uint32_t bufferOccupancy);

  void setTransmit(uint32_t tti, bool _transmit) { transmit[getIdx(tti)] = _transmit; }
  bool getTransmit(uint32_t tti) { return transmit[getIdx(tti)]; }

  uint32_t getLatestTti() { return latestTti; }
  float getAverageSRSSI(uint32_t tti) {
    return avgSRssi[getIdx(tti)];
  }

  float getAverageSRSRP(uint32_t tti) {
    uint32_t idx = getIdx(tti);

    for (uint32_t i = 0; i < numScis[idx];++i)
    {
      return scis[idx][i].rsrp; // @todo currently just return the 1st rsrp for the 1st SCI we find
    }
    return -INFINITY;
  }

  float getSRSSI(uint32_t tti, uint32_t channel) {
    uint32_t idx = getIdx(tti);

    return sRssi[idx][channel];
  }

  void dummyRx(uint32_t tti);

private:
  SL_CommResourcePoolV2X_r14* resourcePool;
  SL_CommTxPoolSensingConfig_r14* sensingConfig;

  uint32_t sensingWindowSize;
  uint32_t currentPrsvpTx;
  
  uint32_t latestTti; // only for REST interface

  uint32_t sensingWindowFilled;

  // SCIs received this TTI
  uint32_t   numScis[MAX_SENSING_WINDOW];
  SensingSCI scis[MAX_SENSING_WINDOW][MAX_SCIS_IN_TTI];

  uint32_t calc_reselection_counter(uint32_t rsvp) const;

  // S-RSSI measurements
  // Set to -INFINITY if it we did not monitor the subframe
  float sRssi[MAX_SENSING_WINDOW][MAX_SUBCHANNELS];
  float avgSRssi[MAX_SENSING_WINDOW];

  // Used to keep track of whether work_sl_tx() transmitted in a tti
  bool transmit[MAX_SENSING_WINDOW];

  Reservation reservation;

  uint32_t           getIdx(uint32_t tti) { return tti % MAX_SENSING_WINDOW; }
  uint32_t           add_tti_wrap(uint32_t tti) { return tti + MAX_SENSING_WINDOW; }
  uint32_t           remove_tti_wrap(uint32_t tti) { return (tti<MAX_SENSING_WINDOW) ? tti + 10240 - MAX_SENSING_WINDOW : tti - MAX_SENSING_WINDOW; } 

};
}
#endif // SRSLTE_SL_SENSING_SPS_H
