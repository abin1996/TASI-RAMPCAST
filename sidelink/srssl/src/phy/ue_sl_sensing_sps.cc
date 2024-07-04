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

/*
 * Note that these functions may be called from multiple cc_worker threads
 *      so we may need to add mutexes to make this multi-thread safe.
 *
 *
 *
 */

#include "srssl/hdr/phy/ue_sl_sensing_sps.h"
#include "srslte/common/common.h"
#include <assert.h>
#include <srslte/srslte.h>
#include <algorithm>

using namespace srslte;

SensingSPS::SensingSPS(SL_CommResourcePoolV2X_r14*     _resourcePool,
                       SL_CommTxPoolSensingConfig_r14* _sensingConfig,
                       uint32_t                        _sensingWindowSize)
{
  resourcePool      = _resourcePool;
  sensingConfig     = _sensingConfig;
  sensingWindowSize = _sensingWindowSize;

  currentPrsvpTx = 0;

  sensingWindowFilled = 0;

  reservation.active = false;

#ifdef USE_SENSING_SPS
  printf("SensingSPS numSubChannels=%d sensingWindowSize=%d\n",
         resourcePool->numSubchannel_r14,
         sensingWindowSize);
#endif
  assert(sensingWindowSize <= MAX_SENSING_WINDOW);

  memset(numScis, 0, sizeof(numScis));
  memset(transmit, 0, sizeof(transmit));

  for (int i = 0; i < MAX_SENSING_WINDOW; ++i) {
    avgSRssi[i] = -INFINITY;
    for (int j = 0; j < MAX_SUBCHANNELS; ++j) {
      sRssi[i][j] = -INFINITY;
    }
  }
}

// 36.213 v15.2.0 14.1.1.6 (3)
float SensingSPS::getThreshold(uint32_t prioTx, uint32_t prioRx) const
{
  uint32_t i = prioTx * 8 + prioRx;
  assert(i < size_thresPSSCH_RSRP_List_r14);
  return sensingConfig->thresPSSCH_RSRP_List_r14[i];
}

void SensingSPS::tick(uint32_t tti)
{
  // should be called before any other TTI processing
  uint32_t idx = getIdx(tti);

  numScis[idx] = 0;

  avgSRssi[idx] = -INFINITY;
  for (int i = 0; i < MAX_SUBCHANNELS; ++i) {
    sRssi[idx][i] = -INFINITY;
  }

  latestTti = tti;

  if (sensingWindowFilled<1000) {
    ++sensingWindowFilled;
  }
}

void SensingSPS::addSCI(
    uint32_t tti, uint32_t subChannelStart, uint32_t numSubChannels, uint32_t rsvp, uint32_t priority, float rsrp)
{
  uint32_t idx = getIdx(tti);

  assert(numScis[idx] < MAX_SCIS_IN_TTI);

  scis[idx][numScis[idx]].subChannelStart = subChannelStart;
  scis[idx][numScis[idx]].numSubChannels  = numSubChannels;
  scis[idx][numScis[idx]].rsvp            = rsvp;
  scis[idx][numScis[idx]].priority        = priority;
  scis[idx][numScis[idx]].rsrp            = rsrp;

  ++numScis[idx];
}

uint32_t SensingSPS::calc_reselection_counter(uint32_t rsvp) const
{
  uint32_t cresel = 0;
  if (rsvp>=100) {
    cresel = (rand() % 10) + 5;
  } else if (rsvp==50) {
    cresel = (rand() % 20) + 10;
  } else if (rsvp==20) {
    cresel = (rand() % 50) + 25;
  }
  return cresel;
}

ReservationResource* SensingSPS::schedule(uint32_t tti, uint32_t bufferOccupancy)
{
  // Check if we have an existing reservation that has expired
  if (reservation.active) {
    //printf("schedule %d %d %d\n",tti,reservation.startRsvpTti,reservation.rsvp);
    if (tti == tti_add(reservation.startRsvpTti,reservation.rsvp)) {
      if (reservation.Cresel == 0) {
        // reservation has expired
        reservation.active = false;

        if (bufferOccupancy!=0) {
          // We have data to send, check probability that we can just
          // select same reservation again
          int32_t probResourceKeep = sensingConfig->probResourceKeep_r14 * 100; // convert to a percentage

          if (rand() % 100 <= probResourceKeep) {
            reservation.active = true;
            reservation.Cresel = calc_reselection_counter(reservation.rsvp);
            reservation.startRsvpTti = tti;

            reservation._Cresel = reservation.Cresel;
            reservation._startRsvpTti = reservation.startRsvpTti;

            printf("Reservation has been renewed\n");
          }
        }
      
      } else {
        reservation.Cresel -= 1;
        reservation.startRsvpTti = tti;
      }
    }
  }

  // If we still do not have an active reservation and we have data to send
  // we need to select one
  if (!reservation.active && bufferOccupancy) {
    // Try and get an SPS reservation
    uint32_t dl_tti = tti_add(tti,-TX_DELAY);

    uint32_t PrsvpTx = 100; // @todo hardcoded, selection is up to UE implementation
                            //       but must obey the restrictResourceReservationPeriod from higher layers.
    uint32_t cresel = calc_reselection_counter(PrsvpTx);
    bool retransmit = true; // @todo hardcoded derive from allowedRetxNumberPSSCH in pssch-TxConfigList
    uint32_t numChannels = 10; // @todo hardcoded number of channels at the moment
    uint32_t prio = 0; // @todo hardcoded

    srslte::CandidateResources candidates = resourceSelection(dl_tti, TX_DELAY, 50, numChannels, prio, cresel,PrsvpTx);

    if (candidates.size()==0) return NULL; // Something went wrong, or our sensing window is empty

    uint32_t selectionWindowOffset = 0;
    uint32_t subchannelStart = 0;
    uint32_t numSubchannels = 0;

    uint32_t sl_gap = 0;

    if (retransmit) {
      // randomly select a candidate, and try and find a retransmission opportunity
      // returns sl_gap=0 if no retransmission is available
      candidates.random_with_retx(selectionWindowOffset,subchannelStart,numSubchannels,sl_gap);
    } else {
      candidates.random(selectionWindowOffset,subchannelStart,numSubchannels);
    }

    candidates.print(false);

    if (numSubchannels != 0) {
      // We have chosen a new reservation:
      printf("Chosen Resource: %d %d %d tti %d rsvp %d cresel %d sl_gap %d\n",selectionWindowOffset,subchannelStart,numSubchannels,tti,PrsvpTx,cresel,sl_gap);

      reservation.active = true;
      reservation.rsvp = PrsvpTx;
      reservation.Cresel = cresel;
      reservation.startRsvpTti = tti;
      reservation.numResources = 0;

      reservation._Cresel = reservation.Cresel;
      reservation._startRsvpTti = reservation.startRsvpTti;

      reservation.resources[reservation.numResources].rsvpOffset = selectionWindowOffset;
      reservation.resources[reservation.numResources].subchannelStart = subchannelStart;
      reservation.resources[reservation.numResources].numSubchannels = numSubchannels;
      reservation.resources[reservation.numResources].sl_gap = sl_gap;
      reservation.resources[reservation.numResources].is_retx = false;
      reservation.resources[reservation.numResources].rsvp = PrsvpTx;
      ++reservation.numResources;

      if (sl_gap) {
        reservation.resources[reservation.numResources].rsvpOffset = selectionWindowOffset + sl_gap;
        reservation.resources[reservation.numResources].subchannelStart = subchannelStart;
        reservation.resources[reservation.numResources].numSubchannels = numSubchannels;
        reservation.resources[reservation.numResources].sl_gap = 0;
        reservation.resources[reservation.numResources].is_retx = true;
        reservation.resources[reservation.numResources].rsvp = PrsvpTx;
        ++reservation.numResources;
      }
    }
  }

  // Check if we now have a reservation and have something to send this TTI
  if (reservation.active) {
    for (uint32_t i = 0; i < reservation.numResources; ++i) {
      // if (tti == tti_add(reservation.startRsvpTti,reservation.resources[i].rsvpOffset)) {
      //   printf("TX resource tti %d start %d length %d retx %d\n",tti,reservation.resources[i].subchannelStart,reservation.resources[i].numSubchannels,reservation.resources[i].is_retx);
      //   return &reservation.resources[i]; // Only one reservation per TTI
      // }

      // if we miss the first reservation all transmission opportunies of this interval will get
      // skipped, so we check all opportunities
      for(uint32_t j=0; j<=reservation._Cresel; j++) {
        if (tti == tti_add(reservation._startRsvpTti, reservation.resources[i].rsvpOffset + j*reservation.rsvp)) {
          printf("TX resource tti %d start %d length %d retx %d iter: %d\n",tti,reservation.resources[i].subchannelStart,reservation.resources[i].numSubchannels,reservation.resources[i].is_retx, j);
          return &reservation.resources[i]; // Only one reservation per TTI
        }
      }
    }
  }

  return NULL;
}

CandidateResources SensingSPS::resourceSelection(uint32_t tti, uint32_t t1, uint32_t t2, uint32_t LSubCh, uint32_t prioTx, uint32_t Cresel, uint32_t PrsvpTx)
{
  // If we do not have enough sensing data, return an empty set of candidate resources
  if (sensingWindowFilled<sensingWindowSize) {
    return CandidateResources();
  }

  printf("resourceSelection tti=%d t1=%d t2=%d LSubCh=%d\n", tti, t1, t2, LSubCh);
  assert(t1 <= 4 && t2 >= 20 && t2 <= 100); // @todo 20 can be overridden by higher layer prioTx parameter
  uint32_t selectionWindowLength = t2 - t1;

  assert(LSubCh > 0 && LSubCh <= resourcePool->numSubchannel_r14);

  CandidateResources setA(selectionWindowLength, LSubCh, resourcePool->numSubchannel_r14);
  uint32_t           MTotal = setA.size();

  // tti is the current tti
  // selection window is from tti + t1 to tti + t2
  // sensing window is from tti - (sensingWindowSize+1) to tti - 1
  //
  // z is the offset if the sensing window subframe relative to the current tti
  //
  //

  // To simplify the code, so we do not have to deal with a negative TTI
  // we add 1024 to it. This ensures the modulo index into the array is
  // unchanged.
  // For debug purposes, use remove_tti_wrap() when printing out the ttis
  tti = add_tti_wrap(tti);

  // Now 
  uint32_t sensingWindowStartTti = tti - sensingWindowSize - 1;
  uint32_t sensingWindowEndTti = tti - 1;

  // remove any subframes which we did not monitor
  // 36.213 v15.2.0 14.1.1.6 (6)
  // iterate backwards through the sensing window
  //   idx is the index into the sensing data, starting from the previous TTI
  //   z is the subframe of idx relative to the current TTI

  uint32_t count;
  uint32_t  PrsvpPrime = PrsvpTx * PStep / 100;

  for (uint32_t sensingWindowTti = sensingWindowStartTti; sensingWindowTti <= sensingWindowEndTti; ++sensingWindowTti) {
    uint32_t idx = getIdx(sensingWindowTti);
    int32_t z = sensingWindowTti - sensingWindowEndTti - 1;

    //printf("tti %d sensingWindowTti %d z %d idx %d avgRssi %f\n",remove_tti_wrap(tti),remove_tti_wrap(sensingWindowTti),z,idx,avgSRssi[idx]);

    if (avgSRssi[idx] == -INFINITY) {
      // Found a subframe we didn't monitor.
      //printf("tti %d did not monitor tti=%d (idx=%d,z=%d)\n", remove_tti_wrap(tti),remove_tti_wrap(sensingWindowTti), idx, z);

      // For each restrictResourceReservationPeriod
      for (uint32_t nrri = 0;
           nrri < maxReservationPeriod_r14 && sensingConfig->restrictResourceReservationPeriod_r14[nrri] != 0.0;
           ++nrri) {
        float   k = sensingConfig->restrictResourceReservationPeriod_r14[nrri];
        int32_t Q;
        if (k < 1) {
          // @todo should also check nprime - z <= Pstep * k
          printf("nPrime (?) <= Pstep * k + z (%f)\n", PStep * k + z);
          Q = 1 / k;
        } else {
          Q = 1;
        }
        // printf("z %d PStep %d k %f Q %d Cresel %d tti %d\n", z, PStep, k, Q, Cresel,z+tti);
        for (int32_t q = 1; q <= Q; ++q) {
          for (int32_t j = 0; j < (int32_t)Cresel; ++j) {
            int32_t y = z + (int32_t)PStep * k * q - j * PrsvpPrime;
            // Removing any values which lie in the selection window
            // @todo what about potential retransmissions?
            if (y >= (int32_t)t1 && y <= (int32_t)t2) {
              uint32_t removed = setA.remove(y - t1,CANDIDATE_UNMONITORED);
              if (removed)
                printf("tti %d removed %d unmonitored resources for tti %d\n", remove_tti_wrap(tti), removed, remove_tti_wrap(y + tti));
            }
          }
        }
      }
    }
  }

  // SCIs
  CandidateResources copyOfSetA;
  float              threshDelta = 0.0;
  int32_t            m;

  do {
    copyOfSetA = setA;

    for (uint32_t sensingWindowTti = sensingWindowStartTti; sensingWindowTti <= sensingWindowEndTti; ++sensingWindowTti) {
      uint32_t idx = getIdx(sensingWindowTti);
      int32_t m = sensingWindowTti - sensingWindowEndTti - 1;

      for (uint32_t sci = 0; sci < numScis[idx]; ++sci) {
        uint32_t PrioRx  = scis[idx][sci].priority;
        uint32_t PrsvpRx = scis[idx][sci].rsvp; // @todo should rsvp be a float?
        float    thresh  = getThreshold(prioTx, PrioRx) + threshDelta;

        printf("tti %d Handling SCI %d PrioRx %d PrsvpRx %d RSRP %f Threshold %f\n",remove_tti_wrap(tti),remove_tti_wrap(sensingWindowTti),PrioRx,PrsvpRx,scis[idx][sci].rsrp, thresh);

        // Only check SCIs which are above the RSRP threshold, and
        // have a resource_reservation field.
        if (scis[idx][sci].rsrp > thresh && PrsvpRx > 0) {
          for (int32_t j = 0; j < (int32_t)Cresel; ++j) {
            int32_t y = m + j * PStep * PrsvpRx;
            if (y >= (int32_t)t1 && y <= (int32_t)t2) {
              uint32_t removed = copyOfSetA.remove(y - t1, scis[idx][sci].subChannelStart, scis[idx][sci].numSubChannels,CANDIDATE_SCI_RSRP_THRESH);
              if (removed)
                printf("tti %d removed %d candidates sci %d:%d tti %d\n",
                       remove_tti_wrap(tti),
                       removed,
                       scis[idx][sci].subChannelStart,
                       scis[idx][sci].numSubChannels,
                       remove_tti_wrap(y + tti));
            }
          }
        }
      }
    }
    threshDelta += 3.0; // increase by 3dBm
  } while ((float)copyOfSetA.size() < 0.2 * MTotal);
  
  // Linear Average of S-RSSI
  copyOfSetA.handleSrssi(this,sensingWindowStartTti,sensingWindowEndTti,t1,t2,MTotal);

  if (copyOfSetA.size() != MTotal) {
    copyOfSetA.print(true);
  }

  return copyOfSetA;
}

void SensingSPS::addAverageSRSSI(uint32_t tti, float _sRssi)
{
  avgSRssi[getIdx(tti)] = _sRssi;
}

void SensingSPS::addChannelSRSSI(uint32_t tti, uint32_t channel, float _sRssi)
{
  assert(channel < MAX_SUBCHANNELS);
  sRssi[getIdx(tti)][channel] = _sRssi;
}

class DummyUe {
  public:
    DummyUe(uint32_t _rsvp, uint32_t _Cresel, uint32_t _priority) {
      rsvp = _rsvp;
      Cresel = _Cresel;
      priority = _priority;

      currentCResel = 0;
      nextTx = rand() % 500;
    }

    SensingSCI genSci(uint32_t tti)
    {
      SensingSCI sci = {0, 0, 0, 0, 0.0};

      if (nextTx == tti)
      {
        // generate SCI using existing reservation

        // schedule next reservation
        if (currentCResel) {
          --currentCResel;
          sci.numSubChannels = NSubCh;
          sci.subChannelStart = LSubCh;
          sci.rsvp = (currentCResel) ? rsvp : 0;
          sci.priority = priority;
          sci.rsrp = 12.0; // @todo
          nextTx = SensingSPS::tti_add(nextTx,rsvp);
          //printf("dummyUe genSci tti %d NSubCh %d LSubCh %d rsvp %d prio %d rsrp %f nextTx %d currentCResel %d \n", tti, sci.subChannelStart, sci.numSubChannels, sci.rsvp, sci.priority, sci.rsrp, nextTx, currentCResel);

        } else {
          // need to schedule a new reservation
          nextTx         = SensingSPS::tti_add(tti,1 + (rand() % 300)); // @todo should we ensure this does not clash with sync signal?
          NSubCh         = (rand() % 10);                // 0..9 Assuming 10 subchannels
          LSubCh         = (rand() % (10 - NSubCh)) + 1; // 1..10 Assuming 10 subchannels
          currentCResel  = Cresel;
          //printf("dummyUe genSci tti %d new reservation %d\n",tti,nextTx);
        }
      }
      return sci;
    }

  private:
    uint32_t rsvp;
    uint32_t Cresel;
    uint32_t priority;

    uint32_t currentCResel;
    uint32_t NSubCh;
    uint32_t LSubCh;
    uint32_t txCount;
    uint32_t nextTx;
};

void SensingSPS::dummyRx(uint32_t tti)
{
  static uint32_t PrsvpTx = 100;
  static DummyUe dummyUes[5] = {
    DummyUe(PrsvpTx,calc_reselection_counter(PrsvpTx),1),
    DummyUe(PrsvpTx,calc_reselection_counter(PrsvpTx),2),
    DummyUe(PrsvpTx,calc_reselection_counter(PrsvpTx),3),
    DummyUe(PrsvpTx,calc_reselection_counter(PrsvpTx),4),
    DummyUe(PrsvpTx,calc_reselection_counter(PrsvpTx),5)
  };
  
  // Create some dummy SCIs / RSSI measurements for development/testing
  for (int i=0; i<5; ++i) {
    SensingSCI sci = dummyUes[i].genSci(tti);
    if (!getTransmit(tti)) {
      if (sci.numSubChannels) {
        addSCI(tti,sci.subChannelStart,sci.numSubChannels,sci.rsvp,sci.priority,sci.rsrp);
      }
    }
  }
}

CandidateResources::CandidateResources(uint32_t _windowSize, uint32_t _LSubCh, uint32_t _NSubCh)
{
  windowSize = _windowSize;
  LSubCh     = _LSubCh;
  NSubCh     = _NSubCh;

  memset(numResources, 0, sizeof(numResources));
  memset(resources, 0, sizeof(resources));

  setSize = 0;

  for (uint32_t i = 0; i < windowSize; ++i) {
    numResources[i] = 0;
    for (uint32_t chan = 0; chan + LSubCh <= NSubCh; ++chan) {
      resources[i][numResources[i]].valid           = CANDIDATE_VALID;
      resources[i][numResources[i]].subChannelStart = chan;
      ++numResources[i];
      ++setSize;
    }
  }
}

CandidateResources::CandidateResources(const CandidateResources& other)
{
  windowSize = other.windowSize;
  LSubCh     = other.LSubCh;
  NSubCh     = other.NSubCh;
  setSize    = other.setSize;

  memcpy(numResources, other.numResources, sizeof(numResources));
  memcpy(resources, other.resources, sizeof(resources));
}

uint32_t CandidateResources::remove(uint32_t idx, CandidateResourceStatus reason)
{
  uint32_t removed = 0;

  if (numResources[idx] == 0)
    return removed;

  for (uint32_t i = 0; i < MAX_CANDIDATE_RESOURCES; ++i) {
    if (resources[idx][i].valid == CANDIDATE_VALID) {
      removed += remove(idx,i,reason);
    }
  }

  return removed;
}

uint32_t CandidateResources::remove(uint32_t idx, uint32_t channel, CandidateResourceStatus reason)
{
  if (numResources[idx] != 0 && resources[idx][channel].valid == CANDIDATE_VALID) {
    resources[idx][channel].valid = reason;
    --setSize;
    --numResources[idx];
    return 1;
  }
  return 0;
}

uint32_t CandidateResources::remove(uint32_t idx, uint32_t channel, uint32_t len, CandidateResourceStatus reason)
{
  uint32_t removed = 0;

  if (numResources[idx] == 0)
    return removed;

  for (uint32_t i = 0; i < MAX_CANDIDATE_RESOURCES; ++i) {
    if (resources[idx][i].valid == CANDIDATE_VALID) {
      if (resources[idx][i].subChannelStart >= channel + len || resources[idx][i].subChannelStart + LSubCh <= channel) {
        // okay
      } else {
        // overlap
        removed += remove(idx,i,reason);
      }
    }
  }
  return removed;
}

void CandidateResources::print(bool withRssi)
{
  for (uint32_t j = 0; j <= NSubCh-LSubCh; ++j) {
    printf("%02d+%d", j,LSubCh);
    for (uint32_t i = 0; i < windowSize; ++i) {
      switch (resources[i][j].valid) {
        case CANDIDATE_VALID:
          printf(" *");
          break;
        case CANDIDATE_UNMONITORED:
          printf(" U");
          break;
        case CANDIDATE_SCI_RSRP_THRESH:
          printf(" S");
          break;
        case CANDIDATE_RSSI:
          printf(" R");
          break;
        case CANDIDATE_CHOSEN:
          printf(" C");
          break;
        default:
          printf(" ?");
          break;
      }
    }
    printf("\n");
  }
  if (withRssi) {
    for (uint32_t j = 0; j <= NSubCh-LSubCh; ++j) {
      printf("%02d+%d", j,LSubCh);
      for (uint32_t i = 0; i < windowSize; ++i) {
        if (resources[i][j].valid!=CANDIDATE_UNMONITORED) {
          printf(" %.2f",avgRssi[i][j]);
        } else {
          printf(" -");
        }
      }
      printf("\n");
    }
  }
}

void CandidateResources::handleSrssi(
    SensingSPS* sps, uint32_t sensingWindowStartTti, uint32_t sensingWindowEndTti, uint32_t t1, uint32_t t2, uint32_t MTotal)
{
  uint32_t numRssiToSort = 0;
  float rssiToSort[MAX_SELECTION_WINDOW*MAX_CANDIDATE_RESOURCES] = { 0.0 };

  for (uint32_t selectionIdx = 0; selectionIdx < windowSize; ++selectionIdx) {
    for (uint32_t chan = 0; chan < MAX_CANDIDATE_RESOURCES; ++chan) {
      if (resources[selectionIdx][chan].valid == CANDIDATE_VALID) {

        float    sumRssi = 0.0;
        uint32_t numRssi = 0;

        for (uint32_t sensingWindowTti = sensingWindowStartTti; sensingWindowTti <= sensingWindowEndTti;
             ++sensingWindowTti) {
          int32_t z = sensingWindowTti - sensingWindowEndTti - 1;
          int32_t y = selectionIdx + t1 - z;

/*
  j is non-negative integer
  if (PrsvpTx >= 100) {
      y - Pstep * j
  } else {
      y - PrsvpTxPrime * j
  }
*/

          if ( y % 100 /* @todo */ == 0) {
            for (uint32_t l = resources[selectionIdx][chan].subChannelStart; l <= LSubCh; ++l) {
              float rssi = sps->getSRSSI(sensingWindowTti, l);
              if (rssi!=-INFINITY) {
                sumRssi += rssi;
                ++numRssi;
              }
            }
          }
        }

        if (numRssi!=0) {
          avgRssi[selectionIdx][chan] = sumRssi / numRssi;
        } else {
          avgRssi[selectionIdx][chan] = INFINITY; // @todo
        }

        rssiToSort[numRssiToSort++] = avgRssi[selectionIdx][chan];
      }
    }
  }

  //
  // We want to 'sort' the candidate resources and only keep the 0.2*MTotal candidates
  // with the lowest avgRssi.
  // To do this we will sort the rssiToSort array, and use this to find the threshold,
  // and then remove the candidates from the set.
  //
  std::sort(&rssiToSort[0], &rssiToSort[numRssiToSort - 1]);

  uint32_t threshold_index = (uint32_t)(0.2 * MTotal);
  if (threshold_index > 0)
    threshold_index -= 1;
  float threshold = rssiToSort[threshold_index];

  for (uint32_t selectionIdx = 0; selectionIdx < windowSize; ++selectionIdx) {
    for (uint32_t chan = 0; chan < MAX_CANDIDATE_RESOURCES; ++chan) {
      if (resources[selectionIdx][chan].valid == CANDIDATE_VALID) {
        if (avgRssi[selectionIdx][chan] > threshold) {
          remove(selectionIdx,chan,CANDIDATE_RSSI);
        }
      }
    }
  }
}

void CandidateResources::random(uint32_t& selectionWindowOffset, uint32_t& subchannelStart, uint32_t& numSubchannels)
{

  // Randomly choose one of the candidate resources
  // Once it has been chosen, it is then removed from the set.
  //
  uint32_t num = 0;

  for (uint32_t selectionIdx = 0; selectionIdx < windowSize; ++selectionIdx) {
    num += numResources[selectionIdx];
  }

  uint32_t chosen = rand() % num;
  uint32_t count = 0;

  for (uint32_t selectionIdx = 0; selectionIdx < windowSize; ++selectionIdx) {
    if (numResources[selectionIdx] != 0) {
      for (uint32_t chan = 0; chan < MAX_CANDIDATE_RESOURCES; ++chan) {
        if (resources[selectionIdx][chan].valid == CANDIDATE_VALID) {
          if (count==chosen) {
            selectionWindowOffset = selectionIdx;
            subchannelStart = resources[selectionIdx][chan].subChannelStart;
            numSubchannels = LSubCh;
            remove(selectionIdx,chan,CANDIDATE_CHOSEN);
            return;
          }
          ++count;
        }
      }
    }
  }
}

void CandidateResources::random_with_retx(uint32_t& selectionWindowOffset, uint32_t& subchannelStart, uint32_t& numSubchannels, uint32_t& sl_gap)
{
  random(selectionWindowOffset, subchannelStart, numSubchannels);

  sl_gap = 0;

  // See if there is a retransmission opportunity within
  // 15 subframes of our selected resource (36.321 5.14.1 & 36.213 14.1.1.7)
  //
  // The specs suggest that the retransmission can be outside of the selection
  // window. But for now we will constrain it to be within this window.
  //
  // If no retransmission opportunity can be found we return sl_gap=0
  //
  for (uint32_t selectionIdx = (selectionWindowOffset >= 15) ? selectionWindowOffset - 15 : 0;
       selectionIdx < windowSize && selectionIdx <= selectionWindowOffset + 15;
       ++selectionIdx) {
    if (numResources[selectionIdx] != 0) {
      for (uint32_t chan = 0; chan < MAX_CANDIDATE_RESOURCES; ++chan) {
        if (resources[selectionIdx][chan].valid == CANDIDATE_VALID) {
          if (resources[selectionIdx][chan].subChannelStart == subchannelStart) {
            printf("Found retx candidate %d orig %d\n", selectionIdx, selectionWindowOffset);

            if (selectionIdx>selectionWindowOffset) {
              sl_gap = selectionIdx - selectionWindowOffset;
              remove(selectionIdx,chan,CANDIDATE_CHOSEN);
            } else {
              sl_gap = selectionWindowOffset - selectionIdx;
              selectionWindowOffset = selectionIdx;
              remove(selectionIdx,chan,CANDIDATE_CHOSEN);
            }
            return;
          }
        }
      }
    }
  }
}
