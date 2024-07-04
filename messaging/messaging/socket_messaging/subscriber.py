import socket
import json
import time
import pandas as pd
import shutil
from helpers.metrics import MetricsHelper
from helpers.file_helper import FileHelper
import os


class UDPListener:
    def __init__(self, ip_address="127.0.0.1", port=12345, filename="sample", total_sent_msges=500):
        
        self.ip_address = ip_address
        self.port = port
        self.total_sent_msges = total_sent_msges
        self.filename = filename
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.ip_address, self.port))
        self.metrics_helper = MetricsHelper()
        self.file_helper = FileHelper()  
        self.message_sent_times = []
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

    def receive_messages(self):
        message_bytes, address = self.sock.recvfrom(4096)
        message_string = message_bytes.decode()
        message_data = json.loads(message_string)
        print("Received message from {}: {}".format(address, message_data['counter']))
        return message_data, time.time()
    
    def listen_for_messages(self):
        num_of_received_messages = 0
        num_sent_packet = 1 #Assume one packet would be sent
        print("Listening for messages")
        while num_sent_packet < self.total_sent_msges:
            message_data, received_time = self.receive_messages()
            num_of_received_messages += 1
            num_sent_packet = message_data['counter']
            message_sent_time = message_data['time']
            msg_latency = self.metrics_helper.calculate_latency(received_time, message_sent_time)
            msg_pdr = self.metrics_helper.calculate_packet_delivery_ratio(num_of_received_messages, num_sent_packet)
            msg_pir = None
            if len(self.message_sent_times) > 2 :
                msg_pir = self.metrics_helper.calculate_packet_inter_reception(self.message_sent_times[-1], message_sent_time)
            self.message_sent_times.append(message_sent_time)
            msg_stat = [num_sent_packet,msg_pdr,msg_latency,msg_pir]
            self.received_message_stats.append(msg_stat)
            print("Message Stats(Nos, PDR, Latency, PIR): ", msg_stat)

        self.file_helper.save_message_stats(self.received_message_stats, self.filename)
        self.metrics_helper.plot_message_stats(self.received_message_stats, self.filename)
       
    
if __name__ == "__main__":
    udp_listener = UDPListener()
    udp_listener.listen_for_messages()
