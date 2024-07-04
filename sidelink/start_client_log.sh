#!/bin/bash
TIMESTAMP=`date +"%Y%m%d%H%M%Sh"`
MEASUREFILE="robin_${TIMESTAMP}_sidelink_log.txt"
PCAPFILE="robin_${TIMESTAMP}_sidelink_log.pcap"
DATADIR="/home/vw/working/measurements"
OUTFILE="${DATADIR}/${MEASUREFILE}"
PCAPFILE="${DATADIR}/${PCAPFILE}"
sudo build/srssl/src/srssl srssl/ue.conf.example --expert.phy.sidelink_id 2 --expert.phy.sidelink_master 0 --log.filename $OUTFILE --log.phy_level "warning" --log.phy_lib_level "warning" --pcap.enable 1 --pcap.filename $PCAPFILE
