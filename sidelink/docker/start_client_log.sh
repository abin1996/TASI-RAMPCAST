#!/bin/bash
TIMESTAMP=`date +"%Y%m%d%H%M%Sh"`
MEASUREFILE="${TIMESTAMP}_sidelink_log.txt"
PCAPFILE="${TIMESTAMP}_sidelink_log.pcap"
DATADIR="/sidelink/logs"
OUTFILE="${DATADIR}/${MEASUREFILE}"
PCAPFILE="${DATADIR}/${PCAPFILE}"

/sidelink/build/srssl/src/srssl /sidelink/srssl/ue.conf.example --expert.phy.sidelink_id 2 --expert.phy.sidelink_master 0 --log.filename $OUTFILE --log.phy_level "warning" --log.phy_lib_level "warning" --pcap.enable 1 --pcap.filename $PCAPFILE --rf.device_args "type=b200,serial=320F331" --gw.tun_dev_name "tun_srssl_c"
