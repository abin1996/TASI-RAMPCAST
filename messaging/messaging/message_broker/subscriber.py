import sys
import time
import json
import paho.mqtt.client as mqtt
from helpers.metrics import MetricsHelper
from helpers.file_helper import FileHelper
import os
import shutil

class MQTTListener:
    def __init__(self, broker_host="127.0.0.1", broker_port=1883, timeout=60, filename="test", total_sent_msges=500, client_username="mqtt-test1", client_pwd="tasi"):
        self.sub_messages = 0
        self.client = None
        self.username = client_username
        self.pwd = client_pwd
        self.ts_map = {}
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.timeout = timeout
        self.filename = filename
        self.total_msgs = total_sent_msges
        self.metrics_helper = MetricsHelper()
        self.file_helper = FileHelper()

        self.num_of_received_messages = 0
        self.message_received_times = []
        self.received_message_stats=[]
    
    def move_metric_file(self, metric_file_path, output_directory):
        try:
            # Check if the file exists before proceeding
            if not os.path.exists(metric_file_path):
                raise FileNotFoundError(f"The file {metric_file_path} does not exist.")
            
            # Extract the filename from the metric_file_path
            metric_file_name, extension = os.path.splitext(os.path.basename(metric_file_path))
            
            # Create the new filename by appending the current date and timestamp
            new_metric_file_name = '{}-{}.csv'.format(metric_file_name, time.strftime('%Y%m%d-%H%M%S'))
            
            # Create the full path for the new metric file in the output directory
            new_metric_file_path = os.path.join(output_directory, new_metric_file_name)
            
            # Move the metric file to the output directory with the new name
            shutil.move(metric_file_path, new_metric_file_path)
            
            print(f"Metric file moved and renamed to: {new_metric_file_path}")
    
        except FileNotFoundError as e:
            print(f"Error: {e}")
        
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
        

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))
        client.subscribe("V2X-Traveller-Information")

    def on_message(self, client, userdata, message_data):
        print("Message received:", message_data.payload)
        self.num_of_received_messages += 1
        message_received_time = time.time()
        message_json = json.loads(message_data.payload)
        num_sent_packet = message_json['counter']
        message_sent_time = message_json['time']
        msg_latency = self.metrics_helper.calculate_latency(message_received_time, message_sent_time)
        msg_pdr = self.metrics_helper.calculate_packet_delivery_ratio(self.num_of_received_messages, num_sent_packet)
        msg_pir = None
        if len(self.message_received_times) > 2 :
            msg_pir = self.metrics_helper.calculate_packet_inter_reception(self.message_received_times[-1], message_received_time)
        self.message_received_times.append(message_received_time)
        msg_stat = [num_sent_packet,msg_pdr,msg_latency,msg_pir]
        self.received_message_stats.append(msg_stat)
        print("Message Stats(Nos, PDR, Latency, PIR): ", msg_stat)

        if(message_json['counter'] >= self.total_msgs):
            self.file_helper.save_message_stats(self.received_message_stats, self.filename)
            self.metrics_helper.plot_message_stats(self.received_message_stats, self.filename)
            self.client.disconnect()

    def connect(self):
        self.client = mqtt.Client()
        self.client.username_pw_set(self.username,self.pwd)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(self.broker_host, self.broker_port, self.timeout)
        print("Listening for messages")
        self.client.loop_forever()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Please provide the broker host and port as command line arguments.")
        sys.exit(1)

    broker_host = sys.argv[1]
    broker_port = int(sys.argv[2])

    mqtt_client = MQTTListener(broker_host, broker_port)
    mqtt_client.connect()
