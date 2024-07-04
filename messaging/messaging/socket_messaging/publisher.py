import socket
import json
import time

import socket
import json
import time
import random
import string

class UDPPublisher:
    def __init__(self, message_data, ip_address="127.0.0.1", port=12345, msg_frequency=1,total_messages=500, message_size=1024):
        self.ip_address = ip_address
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sleep_duration = float(1/msg_frequency)
        self.message_data = message_data.copy()
        self.total_msg = total_messages
        self.message_size = message_size

    def publish(self):
        counter = 1
        current_message_index = 0  # Initialize the index for message_data
        while counter <= self.total_msg:
            cur_time = time.time()
            message_data = self.message_data[current_message_index]
            message_data["time"] = cur_time
            message_data["counter"] = counter

            # Check if the JSON message is less than self.message_size
            message_json = json.dumps(message_data)
            if len(message_json) < self.message_size:
                random_data_length = self.message_size - len(message_json)
                random_data = ''.join(random.choice(string.ascii_letters) for _ in range(random_data_length))
                message_data["random_data"] = random_data

            message_bytes = json.dumps(message_data).encode()
            self.sock.sendto(message_bytes, (self.ip_address, self.port))

            print("Sent the {}th message".format(counter))
            counter += 1
            current_message_index += 1  # Move to the next message

            # If we reach the end of message_data, reset the index to 0
            if current_message_index >= len(self.message_data):
                current_message_index = 0

            time.sleep(self.sleep_duration)

        self.sock.close()
        
if __name__ == "__main__":
    # Message data as a dictionary
    message_data = {
        "sign": {
            "__typename": "Sign",
        }
    }

    udp_client = UDPPublisher(message_data)
    udp_client.publish()
