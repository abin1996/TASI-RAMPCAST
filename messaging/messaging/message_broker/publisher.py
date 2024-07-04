import sys
import paho.mqtt.client as mqtt
import time
import json

class MQTTPublisher:
    def __init__(self, message_data, ip_address="127.0.0.1", port=1883, timeout=60, msg_frequency=1,total_messages=500,client_username="mqtt-test1", client_pwd="tasi"):
        self.ip_address = ip_address
        self.port = port
        self.timeout = timeout
        self.client = None
        self.username = client_username
        self.pwd = client_pwd
        self.sleep_duration = float(1/msg_frequency)
        self.message_data = message_data.copy()
        self.total_msg = total_messages
        self.topic = "V2X-Traveller-Information"

    def connect(self):
        self.client = mqtt.Client()
        self.client.username_pw_set(self.username,self.pwd)
        self.client.on_connect = self.on_connect
        self.client.connect(self.ip_address, self.port, self.timeout)
    
    def disconnect(self):
        self.client.disconnect()

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))
        self.client.subscribe(self.topic)

    def on_publish(self, client, userdata, mid):
        print("Published")
    
    def publish(self):
        counter = 1
        while counter <= self.total_msg:
            cur_time = time.time()
            message_data = self.message_data
            message_data["time"] = cur_time
            message_data["counter"] = counter

            message_json = json.dumps(message_data)
            message_bytes = message_json.encode()
            self.client.publish(self.topic, message_bytes)
            print("Sent the {}th message".format(counter))
            counter += 1
            time.sleep(self.sleep_duration)  
        self.disconnect()


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Please provide the broker host and port as command line arguments.")
        sys.exit(1)

    broker_host = sys.argv[1]
    broker_port = int(sys.argv[2])
    timeout = 60

    mqtt_client = MQTTPublisher(broker_host, broker_port, timeout)
    mqtt_client.connect()
