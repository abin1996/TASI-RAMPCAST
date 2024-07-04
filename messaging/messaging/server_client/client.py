import socket
import json
import time
from helpers.metrics import MetricsHelper
from helpers.file_helper import FileHelper
import os
import shutil

class Client:

    def __init__(self, message, host='10.0.2.12', port=12346,target_ip='10.0.2.11',target_port=12345, frequency=5, total_messages_sent=500, saved_metrics_filename="sample",gps_thread=None):
        self.host = host
        self.port = port
        self.target_ip = target_ip
        self.target_port = int(target_port)
        self.message = message
        self.interval = float(1/frequency)
        self.timeout = 5
        self.socket = None
        self.msg_stats = []
        self.total_messages = total_messages_sent
        self.metrics_file_name = saved_metrics_filename
        self.metrics_helper = MetricsHelper()
        self.file_helper = FileHelper()
        self.last_received_at = None
        self.gps_thread = gps_thread

    def connect(self):
        print("*")
        # self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # self.socket.bind((self.host, self.port))
        # self.socket.settimeout(10)
        print(f"Connected to UDP server at {self.host}:{self.port}")

    def start_pinging(self):
        start_time = time.time()
        received_count = 0
        sent_count = 0
        sending_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        for i in range(self.total_messages):
            recv_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            recv_socket.bind((self.host, self.port))
            recv_socket.settimeout(self.timeout)
            # Track per-message index
            sent_count += 1
            # Send message
            s_lat, s_lon, s_timestamp, s_alt, s_speed = self.gps_thread.get_cached_gps_data()
            message_json = json.dumps({"sequence": sent_count, "message": self.message}).encode()
            sending_socket.sendto(message_json, (self.target_ip, self.target_port))
            # Start individual message timer
            send_time_individual = time.time()

            try:
                # Receive response with timeout
                data, addr = recv_socket.recvfrom(2048, self.timeout)
                received_time_individual = time.time()
                r_lat, r_lon, r_timestamp, r_alt, r_speed = self.gps_thread.get_cached_gps_data()

                
                # print(data)
                
                if not data:
                    raise TimeoutError("Response timed out")

                
                # Decode and parse response
                response = json.loads(data.decode())
                print("Received from Server: "+str(response["sequence"]))
                # Check sequence number
                if response["sequence"] != sent_count:
                    print(f"Warning: Received response with unexpected sequence number: {response['sequence']}.")
                received_count += 1

                # Calculate latency and packet delivery rate
                latency = 50*(self.metrics_helper.calculate_latency(received_time_individual, send_time_individual))
                msg_pdr = self.metrics_helper.calculate_packet_delivery_ratio(received_count, sent_count)
                # Calculate inter-reception rate (only for received packets)
                msg_pir = 100*(self.metrics_helper.calculate_packet_inter_reception(self.last_received_at, response["received_at"])) if self.last_received_at else 0
                
                
                self.last_received_at = response["received_at"]
                # Store per-message statistics
                self.msg_stats.append([send_time_individual, sent_count, msg_pdr, latency, msg_pir,s_lat, s_lon, s_timestamp, s_alt, s_speed, r_lat, r_lon, r_timestamp, r_alt, r_speed])

                # Print message received status
                print(f"  - PDR: {msg_pdr:.2f}")
                print(f"  - Latency: {latency:.3f} ms")
                print(f"  - PIR: {msg_pir:.3f} ms")
                print("---------------")
            except TimeoutError as e:
                # Mark as dropped and update statistics
                msg_pdr = self.metrics_helper.calculate_packet_delivery_ratio(received_count, sent_count)
                print(f"Error: Timeout waiting for response to message {sent_count}: {e}")
                print(f"  - PDR: {msg_pdr:.2f}")
                print(f"  - Latency: N/A (timeout)")
                print(f"  - PIR: N/A (timeout)")
                print("---------------")
            # Wait between messages
            time.sleep(self.interval)
            recv_socket.close()

        end_time = time.time()

        # Calculate overall statistics
        total_time = end_time - start_time

        print(f"\nSent {sent_count} messages, Received {received_count}")
        print(f"Packet delivery rate: {msg_pdr:.2f}%")
        print(f"Total time taken: {total_time:.3f} seconds")

        self.file_helper.save_message_stats(self.msg_stats, self.metrics_file_name, timestamp=True)
        self.metrics_helper.plot_message_stats(self.msg_stats, self.metrics_file_name, timestamp=True)
       

    def disconnect(self):
        # UDP doesn't have explicit disconnection
        self.socket.close()

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
if __name__ == "__main__":
    # Set your messages and other parameters
    messages = ""
    client = Client(messages=messages)
    client.connect()
    client.start_pinging()
    client.disconnect()
    print("Client stopped")
