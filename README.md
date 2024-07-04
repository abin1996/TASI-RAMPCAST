# TASI-sidelink

Running PUB code using RabbitMQ broker
Argument in order: Msg framework type, pub or sub, host ip , host port, total number of messages, input file name(must be json), messaging frequency(msgs/s)

python3 ./messaging/messaging/main.py broker pub 127.0.0.1 1883 100 messaging/data/900kb.json 10

Running SUB code using RabbitMQ Broker

Argument in order: Msg framework type, pub or sub, host ip , host port, total number of messages, output csv name
python3 ./messaging/messaging/main.py broker sub 127.0.0.1 1883 100 SAMPLE_DT_1M_2700KB_10HZ_500MSG

Sample Command for running using server-client mode, publisher program(Client)

`python3 messaging/main.py -m sc -p pub -i 127.0.0.1 -port 12346 -tip 127.0.0.1 -tpo 12345 -t 100 -inf ./data/900kb.json -f 5 -mf ../sidelink/output_new_metrics.csv -d TEST-WIFI-100M`

Sample Command for running using server-client mode, subscriber program(Server)

`python3 messaging/main.py -m sc -p sub -i 127.0.0.1 -port 12345 -tip 127.0.0.1 -tpo 12346`

## GPSD Installation and Setup Guide

This guide walks you through the installation and setup of GPSD to retrieve GPS coordinates information from a smartphone. GPSD is a service daemon that collects GPS data from a GPS receiver, such as that found in a smartphone, and makes it available to other programs.

### Installation Steps:

1. **Install GPSD:**
   Open your terminal and run the following command:
    
    `sudo apt-get install gpsd`

2. **Stop GPSD Service:**
To customize GPSD parameters, stop the GPSD service by executing the following commands:

    `sudo systemctl stop gpsd`

    `sudo systemctl stop gpsd.socket`

3. **Run GPSD Daemon with Custom Parameters:**
Start the GPSD daemon with the desired parameters. For example, to run GPSD on UDP port 29998, use the following command:

    `gpsd -N udp://*:29998`


### Setting Up Smartphone:

1. **Install GPSD Client App:**
Install a GPSD client app on your Android smartphone. You can find one at [Google Play Store](https://play.google.com/store/apps/details?id=io.github.tiagoshibata.gpsdclient&pli=1).

2. **Network Configuration:**
Ensure your smartphone and laptop are connected to the same network. Find the IP address of your laptop using the `ipconfig` command and enter it along with the port (29998) in the server address and port fields of the GPSD client app.

3. **Start GPS Streaming:**
Once configured, initiate GPS streaming from the smartphone using the GPSD client app. This will start sending GPS information to your laptop.

### Verification:

To verify that GPS streaming is working correctly, you can use the `gpspipe` command. First, install `gpsd-clients` package by running:
sudo apt install gpsd-clients


Then, use the following commands:
- `gpspipe -w`: This command sends native GPSD sentences.
- `gpspipe -r`: This command streams raw NMEA sentences to the terminal.

By following these steps, you can successfully install and configure GPSD to retrieve GPS coordinates from your smartphone. For more detailed instructions, refer to [this tutorial](https://www.linux-magazine.com/Issues/2018/210/Tutorial-gpsd).
