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
 * This file is part of the srsUE library.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <vector>

#include "srssl/hdr/phy/ue_sl_sensing_sps.h"


// using namespace srslte;

  SL_CommResourcePoolV2X_r14 rp = {
    0, //uint8_t sl_OffsetIndicator_r14;

    // subframe bitmap
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},//uint8_t sl_Subframe_r14[100];
    20, //uint8_t sl_Subframe_r14_len;

    true,//bool adjacencyPSCCH_PSSCH_r14;

    // indicated the number of PRBs of each subchannel in the corresponding resource pool
    5,//uint8_t sizeSubchannel_r14;

    // indicates the number of subchannels in the corresponding resource pool
    10,//uint8_t numSubchannel_r14;

    // indicates the lowest RB index of the subchannel with the lowest index
    0,//uint8_t startRB_Subchannel_r14;

    // indicates the lowest RB index of PSCCH pool, this field is absent whe a pool is (pre)configured
    // such tha a UE always transmits SC and data in adjacent RBs in the same subframe
    0,//uint8_t startRB_PSCCH_Pool_r14;
  };

  // if (srslte_repo_init(&ue_repo, cell, respool)) {
  //   fprintf(stderr, "Error creating Ressource Pool object\n");
  // }

  // // set sync period to 5, as this is what the current sync implementations expects
  // ue_repo.syncPeriod = 5;
  // srslte_repo_build_resource_pool(&ue_repo);


static uint32_t ttl = 1;
static uint32_t port = 9865;

static void usage(char *prog) {
  printf("Usage: %s [t]\n", prog);
  printf("\t-t Seconds the rest server is running [Default %d]\n", ttl);
  printf("\t-p Port to listen on [Default %d]\n", port);
}


void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "tp")) != -1) {
    switch (opt) {
      case 't':
        ttl = (uint32_t)(uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'p':
        port = (uint32_t)(uint32_t)strtol(argv[optind], NULL, 10);
        break;
      default:
        usage(argv[0]);
        exit(-1);
    }
  }
}




// @todo define the SL_CommTxPoolSensingConfig_r14 somewhere else...
static SL_CommTxPoolSensingConfig_r14 sL_CommTxPoolSensingConfig_r14 = {
    .thresPSSCH_RSRP_List_r14 =
        {
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=0, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=1, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=2, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=3, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=4, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=5, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=6, prioRx=0..7 in dBm
            -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, // prioTx=7, prioRx=0..7 in dBm
        },
    .restrictResourceReservationPeriod_r14 = {1.0},
    .probResourceKeep_r14                  = 0.5,
    .sl_ReselectAfter_r14                  = 1};


class SpsUe{
  public:
    SpsUe(SL_CommResourcePoolV2X_r14* rp, SL_CommTxPoolSensingConfig_r14* poolSC){
      sensing_sps = new srslte::SensingSPS(rp,
                                          poolSC,
                                          1000); // sensingWindowSize

      id = N++;
      n_tti_processed = 0;

      printf("Added new UE with id %d\n", id);
    }

    srslte::ReservationResource* schedule(uint32_t tx_tti) {
      return sensing_sps->schedule(tx_tti, 1);
      if(n_tti_processed > 1000) {
        return sensing_sps->schedule(tx_tti, 1);
      }
      return nullptr;
    }

    

    void tick(uint32_t tti) {
      sensing_sps->tick(tti);
      n_tti_processed++;
    }
    
    srslte::SensingSPS* sensing_sps;
    int id;
    static int N;
  private:
    int n_tti_processed;
    
};

int SpsUe::N = 0;

typedef struct {
  int tx_id;
  int frl_n_subCH;
  int frl_L_subCH;
  int resource_reservation;
  int priority;
} tx_sci_t;

class SpsBase {
  public:
    SpsBase() {
      n_collisions = 0;
    }


    void checkforRx(uint32_t tx_tti) {
      
      float srssi[10];
      float rsrp_sps = 0.0004;

      // set initial value
      for(int i=0; i<rp.numSubchannel_r14; i++) {
        srssi[i] = 0.00000001;
      }


      for(int i = 0; i < future_transmit[tx_tti % 4].size(); i++){

        rsrp_sps = 0.0004 * (0.8 + (rand()%100)/1000.0) * 1e-0;

        // add energy to occupied channels
        for(int j = 0; j < (future_transmit[tx_tti % 4])[i].frl_L_subCH; j++) {
          srssi[(future_transmit[tx_tti % 4])[i].frl_n_subCH + j] += rsrp_sps;
        }

        // send decoded sci to all ues
        for(int ue_id = 0; ue_id < _ues.size(); ue_id++){
          if(_ues[ue_id]->id == (future_transmit[tx_tti % 4])[i].tx_id) continue;

          rsrp_sps = 0.0004 * (0.8 + (rand()%100)/1000.0 + ((_ues[ue_id]->id + 7)%10)/100.0) * 1e-0;

          
          _ues[ue_id]->sensing_sps->addSCI(tx_tti,
                                        (future_transmit[tx_tti % 4])[i].frl_n_subCH,
                                        (future_transmit[tx_tti % 4])[i].frl_L_subCH,
                                        (future_transmit[tx_tti % 4])[i].resource_reservation,
                                        (future_transmit[tx_tti % 4])[i].priority,
                                        10 * log10(rsrp_sps * 1000)
                                        );
        }
      }

      // calc average RSSI
      float average_rssi = 0.0;
      for(int i=0; i<rp.numSubchannel_r14; i++) {
        average_rssi += srssi[i];
      }
      average_rssi = average_rssi / rp.numSubchannel_r14;
      

      for(int i = 0; i < _ues.size(); i++){

        // check if this ue was transmitting
        if(_ues[i]->sensing_sps->getTransmit(tx_tti)) continue;
        
        for(int c=0; c<rp.numSubchannel_r14; c++){
          _ues[i]->sensing_sps->addChannelSRSSI(tx_tti, c, 10 * log10(srssi[c] * 1000));
        }

        // set average
        _ues[i]->sensing_sps->addAverageSRSSI(tx_tti,10 * log10(average_rssi * 1000));
      }

    }

    void checkUeForTx(uint32_t tx_tti) {
      future_transmit[tx_tti % 4].clear();


      for(int i = 0; i < _ues.size(); i++){
        srslte::ReservationResource* resource = _ues[i]->schedule(tx_tti);// sensing_sps->schedule(tx_tti, 1);
        if (resource) {
          // 
          tx_sci_t t;
          t.frl_L_subCH = resource->numSubchannels;
          t.frl_n_subCH = resource->subchannelStart;
          t.priority = 0; // fixed
          t.resource_reservation = resource->rsvp;
          t.tx_id = _ues[i]->id;

          future_transmit[tx_tti % 4].push_back(t);

          _ues[i]->sensing_sps->setTransmit(tx_tti,true);
          
          printf("UE %d transmits on tti %4d with L:%d  n:%d\n", t.tx_id, tx_tti, t.frl_L_subCH, t.frl_n_subCH);
        }

      }

      if(future_transmit[tx_tti % 4].size() > 1) {
        printf("Collision detected!!!!!\n");
        n_collisions++;
      }
    }

    void tick(uint32_t tti) {
      for(int i = 0; i < _ues.size(); i++){
        _ues[i]->tick(tti);
        _ues[i]->sensing_sps->setTransmit(tti + 4, false);
      }
    }


    void mainloop() {
      uint32_t tti = 0;

      // setup first ue
      // _ues.push_back(new SpsUe(&rp, &sL_CommTxPoolSensingConfig_r14));


      //for(int i=0; i< 10240; i++) {
      for(int i=0; i< 30820; i++) {

        if(i<7000 && i % 1000 == 0) {
          _ues.push_back(new SpsUe(&rp, &sL_CommTxPoolSensingConfig_r14));
        }

        tick(tti);

        checkforRx(tti);

        checkUeForTx(tti + 4);


        tti = (tti + 1) % 10240;

        // if(n_collisions >2) break;
      }
      printf("Collisons detected: %d\n", n_collisions);

    }



  private:
    std::vector<SpsUe*> _ues;
    // float rssi_per_channel[10] = {0.0};
    std::vector<tx_sci_t> future_transmit[4];
    int n_collisions;
};




int main() {
  SpsBase base;

  base.mainloop();

}
