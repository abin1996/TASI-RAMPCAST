# HHI-Sidelink

This repository is a fork of "sidelink" project by Heinrich-Hertz-Institut (HHI). For more information check original README-content below.

## How to use

When starting HHI-Sidelink, it will create one TUN network interface for each SDR (find examples below). HHI-Sidelink can be executed on the host itself but also in Containers, e.g. Docker-Containers.

### Host

One SDR needs to be started as master, the others as client. The master will start a cell and the clients connect to it. This is necessary because of the underlying srsLTE framework.

1. Connect the SDRs to the computer
2. Optional: find the SDRs by `uhd_find_devices`
3. Run the start scripts (e.g. `start.sh` and `start_client_log.sh`)

### Docker

Build: `docker build -t sidelink_hhi .`

Replace with correct device paths for the next commands:

Execute master: `docker run --rm -d -it --name cv2x_m --device /dev/bus/usb/002/006 --privileged sidelink_hhi /sidelink/start_master.sh`

Execute client: `docker run --rm -d -it --name cv2x_c --device /dev/bus/usb/002/007 --privileged sidelink_hhi /sidelink/start_client_log.sh`

## Resulting interfaces

```
10: tun_srssl_m: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 500
    link/none 
    inet 10.0.2.11/24 scope global tun_srssl_m
       valid_lft forever preferred_lft forever
11: tun_srssl_c: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 500
    link/none 
    inet 10.0.2.12/24 scope global tun_srssl_c
       valid_lft forever preferred_lft forever
```

-----------------------------------------

sidelink
========

Copyright
--------

 Copyright 2013-2019
 Fraunhofer Institute for Telecommunications, Heinrich-Hertz-Institut (HHI)

 This file is part of the HHI Sidelink.
 
 HHI Sidelink is under the terms of the GNU Affero General Public License
 as published by the Free Software Foundation version 3.

 HHI Sidelink is distributed WITHOUT ANY WARRANTY, 
 without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 A copy of the GNU Affero General Public License can be found in
 the LICENSE file in the top-level directory of this distribution
 and at http://www.gnu.org/licenses/.

 The HHI Sidelink is based on srsLTE. 
 All necessary files and sources from srsLTE are part of HHI Sidelink.
 srsLTE is under Copyright 2013-2017 by Software Radio Systems Limited.
 srsLTE can be found under: 
 https://github.com/srsLTE/srsLTE

Hardware
--------

We have tested the following hardware: 
 * USRP B210
 * USRP X300
 * USRP N310

Build Instructions
------------------
Based on Ubuntu the following packages are needed for building.

```
sudo apt-get install build-essential cmake libfftw3-dev libmbedtls-dev libboost-program-options-dev libconfig++-dev libsctp-dev libboost-all-dev libusb-1.0-0-dev python-mako doxygen python-docutils libudev-dev python-numpy python-requests python-setuptools libulfius-dev libjansson-dev libmicrohttpd-dev cpufrequtils

```

Note that depending on your flavor and version of Linux, the actual package names may be different.

* RF front-end driver:
  * UHD:                 https://github.com/EttusResearch/uhd

Please have a working UHD driver installed!


actual build

```
mkdir build
cd build
cmake ../
make
```

Execution Instructions
----------------------

After building, the srssl/ue.conf.example can be modified to your needs.
Setting frequency, gain values and other options. The default configuration is for a B210 locally connected via USB.

Easy start of the "master" node just use the start.sh script in the main folder.
Additionally you can use the start_client.sh script in the main folder for client mode operation with sidelink_id=2.

Runntime information
--------------------

Restapi is attached to port 1300 + given expert.phy.sidelink_id i.e. 13001
The stack generates a tunnel interface called tun_srssl with IP: 10.0.2.10+sidelink_id i.e. 10.0.2.11
Any IP package is routed to air and can be decoded by any other node.


Semi Persistent Scheduling(SPS)
-------------------------------

To enable SPS instead of the default one-shot scheduler we need set the _USE_SENSING_SPS_ flag in [CMakeLists.txt](CMakeLists.txt) and issue a recompilation. When run, the node always has an active 10 MHz Scheduler running, even when there is no data transmitted(in this case we do not transmit but keep the SPS subscription active).


Automatic Gain Control(AGC)
---------------------------

The AGC is enabled by not setting a tx_gain value during startup or in the configuration file(this is the default). When we run in AGC mode we always try to set the receive gain to the largest possible value, where no received signal runs into clipping. The measurement interval for this is larger then 100 ms to make sure we catch transmission of at least 10 Hz SPS.


REST API
--------------------------
During runtime the REST API can be used to get status information and change settings.

* Gain settings
Readout:
```
curl -X GET localhost:13001/phy/gain
```
Example:
```
{"tx_gain":57.0,"rx_gain":41.0}
```

Set new values:
```
curl -X PUT -H "Content-Type: application/json" -d '{"rx_gain":60, "tx_gain":35}' localhost:13001/phy/gain
```

* Measurement data
To get SNR values in db of differnt sidelink_id UEs and PSBCH use:
```
curl -X GET localhost:13001/phy/metrics
```
Example:
```
{"snr_psbch":8.4800424575805664,"snr_ue_0":0.0,"snr_ue_1":8.6331901550292969,"snr_ue_2":21.439271926879883,"snr_ue_3":0.0,"snr_ue_4":0.0}
```
This can be used for estimating the link quality and find optimal gain values. If the SNR is near 30db the RX may saturated and lower values make more sense.

* Resource pool configuration
We can change the physical layer resource pool configuration via REST API.
Readout:
```
curl -X GET localhost:13001/phy/repo
```

Example:
```
{"numSubchannel_r14":10,"sizeSubchannel_r14":5,"sl_OffsetIndicator_r14":0,"sl_Subframe_r14_len":20,"startRB_PSCCH_Pool_r14":0,"startRB_Subchannel_r14":0}
```

The resource pool configuration should be the same on all nodes in the network, otherwise there will be an error information on stdout.

Any field can be set independent or all at once. To set the resource pool configuration:
```
curl -X PUT -H "Content-Type: application/json" -d '{"numSubchannel_r14":10,"sizeSubchannel_r14":5,"sl_OffsetIndicator_r14":0,"sl_Subframe_r14_len":20,"startRB_PSCCH_Pool_r14":0,"startRB_Subchannel_r14":0}' localhost:13001/phy/repo
```
