import argparse
import paho.mqtt.client as mqtt
import json
import time
import math
import tkinter as tk
from PIL import ImageTk, Image, ImageDraw, ImageFont
import os
import queue
import threading
from gpsdclient import GPSDClient
from geographiclib.geodesic import Geodesic
from copy import copy

SPEEDOMETER_IMAGE_PATH = '../Demo/assets/speedometer.png'
V2X_SUB_TOPIC = "V2X-Traveller-Information"
UI_WINDOW_TITLE = "Road messages"
FREEMONO_FONT_PATH = '../Demo/assets/font/FreeMono.ttf'
FREEMONOBOLD_FONT_PATH = '../Demo/assets/font/FreeMonoBold.ttf'


class MsgPriorityQueue(queue.PriorityQueue):
    def __init__(self):
        self.pq = queue.PriorityQueue()
        self.counter = 0
    def put(self, item, priority):
        self.pq.put((priority, self.counter, item))
        print(f"put->priority :{priority}, counter: {self.counter}, item: {item}")
        self.counter += 1
    def get(self):
        p1, p2, item = self.pq.get()
        # print(f"get->p1 :{p1}, p2 :{p2},counter: {self.counter}, item: {item}")
        elements = []
        for item in self.pq.queue:
            elements.append(item)

        # print(elements)
        print("LENGTH: ", str(self.pq.qsize()))
        return item
    def empty(self) -> bool:
        return self.pq.empty()
class UserInterface(threading.Thread):
    def __init__(self, thread_id, ui_queue, window, label):
        threading.Thread.__init__(self)
        self.thread_id = thread_id
        self.speedometer_image = Image.open(SPEEDOMETER_IMAGE_PATH)
        self.window = window
        self.label = label
        self.ui_queue = ui_queue
        self.ui_cache = {'tts': None, 'dms': None}
        self.ui_direction_cache = {
            'NB': {
                65: {
                    'tts': None,
                    'dms': None
                }
            },
            'SB': {
                65: {
                    'tts': None,
                    'dms': None
                },
            },
            'EB': {
                70: {
                    'tts': None,
                    'dms': None
                },
            },
            'WB': {
                70: {
                    'tts': None,
                    'dms': None
                },
            }
        }
        # self.direction_queues = [i65_north_messages, i65_south_messages, i70_east_messages, i70_west_messages]
        # self.i65_north_messages = i65_north_messages
        # self.i65_south_messages = i65_south_messages
        # self.i70_east_messages = i70_east_messages
        # self.i70_west_messages = i70_west_messages
        self.die = False

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))
        self.client.subscribe(V2X_SUB_TOPIC)

    def load_base_image(self):
        composite_image = Image.new('RGBA', (1200, 500), (255, 255, 255, 255))
        self.speedometer_image = self.speedometer_image.resize((1200, 500))
        composite_image.paste(self.speedometer_image, (0, 0))
        self.base_image = composite_image

    def display_image(self):
        tts_message = self.ui_cache['tts']
        if tts_message:
            self.load_image(tts_message.get('device_nbr'), tts_message.get('travel_times'), self.base_image,
                            tts_message.get('mile_values'), tts_message.get('interstate'), tts_message.get('direction'))
        dms_message = self.ui_cache['dms']
        if dms_message:
            # print(dms_message)
            self.create_text_image(dms_message.get('input_text'), self.base_image)
        photo = ImageTk.PhotoImage(self.base_image)
        self.label.config(image=photo)
        self.label.image = photo
        self.window.update()
        time.sleep(2.5)

        # def read_from_queue_and_display(self):

    #     # Define the categories based on 'direction' and 'interstate'
    #     categories = [
    #         {'direction': 'NB', 'interstate': 65},
    #         {'direction': 'SB', 'interstate': 65},
    #         {'direction': 'EB', 'interstate': 70},
    #         {'direction': 'WB', 'interstate': 70}
    #     ]

    #     # Iterate through each category
    #     for direction in self.direction_queues:
    #         category_messages = []

    #         # Read and process messages of type 'tts' for the current category
    #         while not self.direction.empty():
    #             ui_message = self.direction.get()
    #             if ui_message['message_type'] == 'tts' and ui_message['direction'] == category['direction'] and ui_message['interstate'] == category['interstate']:
    #                 category_messages.append(ui_message)

    #         # Display messages of type 'tts' for the current category
    #         for ui_message in category_messages:
    #             self.load_image(ui_message.get('device_nbr'), ui_message.get('travel_times'), self.base_image, ui_message.get('mile_values'), ui_message.get('interstate'), ui_message.get('direction'))
    #             self.display_image()
    #             time.sleep(0.5)

    #         # Process and display messages of other types for the current category
    #         while not self.ui_queue.empty():
    #             ui_message = self.ui_queue.get()
    #             if ui_message['direction'] == category['direction'] and ui_message['interstate'] == category['interstate']:
    #                 self.create_text_image(ui_message.get('input_text'), self.base_image)
    #                 self.display_image()
    #                 time.sleep(0.5)

    def read_from_queue_and_display(self):
        self.load_base_image()
        if not self.ui_queue.empty():
            _,_,ui_message = self.ui_queue.get()
            print(ui_message)
            priority = ui_message.get('priority', 0)
            message_content = ui_message.get('message_type', '') + ': ' + ui_message.get('input_text', '')
            print(f"Message Priority: {priority}, Content: {message_content}")

            if ui_message['message_type'] == 'tts':
                self.ui_cache['tts'] = ui_message
                self.ui_direction_cache[ui_message.get('direction')][ui_message.get('interstate')][
                    'tts'] = ui_message
                prev_dms = self.ui_direction_cache[ui_message.get('direction')][ui_message.get('interstate')]['dms']
                if prev_dms:
                    self.ui_cache['dms'] = prev_dms
            else:
                self.ui_cache['dms'] = ui_message
                self.ui_direction_cache[ui_message.get('direction')][ui_message.get('interstate')][
                    'dms'] = ui_message
                prev_tts = self.ui_direction_cache[ui_message.get('direction')][ui_message.get('interstate')]['tts']
                if prev_tts:
                    self.ui_cache['tts'] = prev_tts
            self.display_image()

        # Delay of 2.5 seconds

    def load_image(self, device_nbr, travel_times, composite_image, additional_mile_values, interstate_value,
                   interstate_direction):
        sign_image_path = f"../Demo/assets/Road-signs-template/{device_nbr}.png"
        if os.path.exists(sign_image_path):
            sign_image = Image.open(sign_image_path)
        else:
            print(f"Sign image for device {device_nbr} not found.")
            return

        sign_image = sign_image.resize((190, 190))
        sign_x = (composite_image.width - sign_image.width) // 2
        sign_y = ((composite_image.height - sign_image.height) // 2) - 100

        composite_image.paste(sign_image, (sign_x, sign_y), mask=sign_image)

        draw = ImageDraw.Draw(composite_image)
        font = ImageFont.truetype(FREEMONO_FONT_PATH, size=20)
        mile_font = ImageFont.truetype(FREEMONOBOLD_FONT_PATH, size=16)

        travel_time1, travel_time2 = travel_times
        mile_value1, mile_value2 = additional_mile_values

        text_width1, text_height1 = draw.textsize(travel_time1, font=font)
        text_x1 = 530
        text_y1 = ((composite_image.height - sign_image.height) // 2) + 50

        text_width2, text_height2 = draw.textsize(travel_time2, font=font)
        text_x2 = 620
        text_y2 = ((composite_image.height - sign_image.height) // 2) + 50

        mile_width1, mile_height1 = draw.textsize(mile_value1, font=mile_font)
        mile_x1 = 518
        mile_y1 = text_y1 - mile_height1 - 28

        mile_width2, mile_height2 = draw.textsize(mile_value2, font=mile_font)
        mile_x2 = 608
        mile_y2 = text_y2 - mile_height2 - 28

        draw.rectangle((mile_x1, mile_y1, mile_x1 + mile_width1 + 2, mile_y1 + mile_height1 + 5),
                       fill=(24, 69, 134, 255))
        draw.rectangle((mile_x2, mile_y2, mile_x2 + mile_width2 + 2, mile_y2 + mile_height2 + 5),
                       fill=(24, 69, 134, 255))

        draw.text((text_x1, text_y1), travel_time1, fill='white', font=font)
        draw.text((text_x2, text_y2), travel_time2, fill='white', font=font)
        draw.text((mile_x1, mile_y1), mile_value1, fill='white', font=mile_font)
        draw.text((mile_x2, mile_y2), mile_value2, fill='white', font=mile_font)

        draw = ImageDraw.Draw(composite_image)
        font = ImageFont.truetype(FREEMONO_FONT_PATH, size=23)

        interstate_text = f"I-{interstate_value} {interstate_direction}"
        text_width, text_height = draw.textsize(interstate_text, font=font)
        text_x = (self.speedometer_image.width - text_width) // 2
        text_y = 20
        draw.text((text_x, text_y), interstate_text, fill='white', font=font)

    def create_text_image(self, text, composite_image):
        draw = ImageDraw.Draw(composite_image)
        font = ImageFont.truetype(FREEMONO_FONT_PATH, size=22)

        text_width, text_height = draw.textsize(text, font=font)
        text_x = (composite_image.width - text_width) // 2
        text_y = ((composite_image.height - text_height) // 2) + 100

        draw.text((text_x, text_y), text, fill='yellow', font=font)

    def run(self):
        try:
            print("Started UI")
            while not self.die:
                self.read_from_queue_and_display()

        except KeyboardInterrupt:
            print("Exiting")

    def join(self):
        print("UI Thread Joining")
        self.die = True
        super().join()


class MqttHandler(threading.Thread):
    def __init__(self, thread_id, vehicle_name, gps_thread, ui_message_queue, is_local, save_message=False):
        threading.Thread.__init__(self)
        self.thread_id = thread_id
        self.client = mqtt.Client(vehicle_name, clean_session=False, reconnect_on_failure=True)
        self.client.username_pw_set('mqtt-test1', 'tasi')
        self.is_local = is_local
        self.save_message = save_message
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.ui_message_queue = ui_message_queue
        self.gps_thread = gps_thread

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))
        self.client.subscribe(V2X_SUB_TOPIC)

    def on_message(self, client, userdata, msg):
        # print("Message received:", msg.payload)
        msg_json = json.loads(msg.payload)
        msg_json['received_time'] = time.time()

        if self.save_message:
            self.save_message_json(msg_json)
        if self.check_message_in_heading_of_vehicle(msg_json):
            try:
                if msg_json['message_type'] == 'tts':
                    self.handle_travel_time_message(msg_json)
                else:
                    self.handle_text_message(msg_json)
            except KeyError:
                print("Error: Required keys not found in the message.")

    def save_message_json(self, msg_json):
        msg_json_counter = msg_json['counter']
        with open(f"./Demo/received_msgs/{msg_json_counter}.json", "w") as f:
            json.dump(msg_json, f, indent=2)

    def handle_travel_time_message(self, msg_json):
        travel_times = (
            str(int(msg_json['message1']) + msg_json['time_to_rsu']),
            str(int(msg_json['message2']) + msg_json['time_to_rsu'])
        )
        mile_values = (
            f"{msg_json['lsign_distance']} MILES",
            f"{msg_json['rsign_distance']} MILES"
        )
        ui_message = {
            "message_type": msg_json['message_type'],
            "device_nbr": msg_json['device_nbr'],
            "travel_times": travel_times,
            "mile_values": mile_values,
            "interstate": msg_json.get('interstate', ''),
            "direction": msg_json.get('direction', ''),
            "priority": msg_json.get('priority', 0)
        }
        priority = msg_json.get('priority', 0)
        #self.ui_message_queue.put((priority, ui_message))
        self.ui_message_queue.put(ui_message, int(priority))


    def handle_text_message(self, msg_json):
        text_lines = msg_json['message1'], msg_json['message2'], msg_json['message3']
        input_text = "\n".join(text_lines)

        ui_message = {
            "message_type": msg_json['message_type'],
            "input_text": input_text,
            "interstate": msg_json.get('interstate', ''),
            "direction": msg_json.get('direction', ''),
            "priority": msg_json.get('priority', 0)
        }
        priority = msg_json.get('priority', 0)

        self.ui_message_queue.put(ui_message,int(priority))
    def check_message_in_heading_of_vehicle(self, msg_json):
        current_loction = self.gps_thread.get_current_gps_data()
        if current_loction.lat and current_loction.lon and current_loction.heading:
            print(current_loction.lat, current_loction.lon, current_loction.heading)

            # Vehicle's heading (in degrees, with respect to north)
            vehicle_heading = current_loction.heading  # Example heading (adjust as needed)

            # Vehicle's latitude and longitude (in degrees) and given coordinate (adjust as needed)
            vehicle_latitude = current_loction.lat  # Example vehicle latitude
            vehicle_longitude = current_loction.lon  # Example vehicle longitude
            print(msg_json['geometry'])
            coord_latitude = msg_json['geometry']['coordinates'][1]  # Example given coordinate latitude
            coord_longitude = msg_json['geometry']['coordinates'][0]  # Example given coordinate longitude

            # Radius of the sector (in degrees)
            sector_radius_degrees = 30  # Half of the 30-degree sector

            # Radius around the vehicle (in kilometers)
            vehicle_radius_kilometers = 10  # 5 miles in kilometers

            # Convert latitude and longitude to radians
            vehicle_latitude_rad = math.radians(vehicle_latitude)
            vehicle_longitude_rad = math.radians(vehicle_longitude)
            coord_latitude_rad = math.radians(coord_latitude)
            coord_longitude_rad = math.radians(coord_longitude)

            # Calculate the distance between the vehicle and the given coordinate using Haversine formula
            delta_lat = coord_latitude_rad - vehicle_latitude_rad
            delta_lon = coord_longitude_rad - vehicle_longitude_rad

            a = math.sin(delta_lat / 2) ** 2 + math.cos(vehicle_latitude_rad) * math.cos(coord_latitude_rad) * math.sin(
                delta_lon / 2) ** 2
            c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
            distance_to_coordinate = 6371.0 * c  # Earth's radius in kilometers

            # Calculate the initial bearing from the vehicle to the given coordinate
            y = math.sin(delta_lon) * math.cos(coord_latitude_rad)
            x = math.cos(vehicle_latitude_rad) * math.sin(coord_latitude_rad) - math.sin(
                vehicle_latitude_rad) * math.cos(coord_latitude_rad) * math.cos(delta_lon)
            angle_to_coordinate = math.degrees(math.atan2(y, x))
            angle_to_coordinate = (angle_to_coordinate + 360) % 360  # Normalize angle to be between 0 and 360 degrees

            # Calculate the absolute angular difference between the vehicle's heading and angle to coordinate
            angular_difference = abs(vehicle_heading - angle_to_coordinate)

            # Check if the coordinate lies within the sector and within the specified radius
            if angular_difference <= sector_radius_degrees and distance_to_coordinate <= vehicle_radius_kilometers:
                print("The coordinate lies within the sector and within the specified radius.")
                return True
            else:
                print("The coordinate does not meet the criteria.")
                return False
        return True

    def run(self):
        try:
            if self.is_local.lower() == 'true':
                self.client.connect("localhost", 1883, 60)
            else:
                self.client.connect("10.0.2.11", 1883, 60)
            self.client.loop_forever()
        except KeyboardInterrupt:
            print("Exiting")
            self.client.disconnect()

    def join(self):
        print("MQTT Thread Joining")
        self.client.disconnect()
        super().join()


class GPSData():
    def __init__(self, lat=None, lon=None, time=None, alt=None, speed=None, heading=None) -> None:
        self.lat = lat
        self.lon = lon
        self.time = time
        self.alt = alt
        self.speed = speed
        self.heading = heading


class GPSHandler(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.gps_client = GPSDClient(port=12345)
        self.cached_gps_value = GPSData()
        self.active_gps_value = GPSData()

    def set_bearing(self):
        # print("cach", self.cached_gps_value.lat, self.cached_gps_value.lon)
        # print("act",self.active_gps_value.lat, self.active_gps_value.lon)
        if self.cached_gps_value.lat and self.cached_gps_value.lon:
            brng = \
            Geodesic.WGS84.Inverse(self.cached_gps_value.lat, self.cached_gps_value.lon, self.active_gps_value.lat,
                                   self.active_gps_value.lon)['azi1']
            self.active_gps_value.heading = brng

    def get_current_gps_data(self):
        return self.active_gps_value

    def run(self):
        while True:
            try:
                with self.gps_client as client:
                    for result in client.dict_stream(convert_datetime=True, filter=["TPV", "ATT"]):
                        # print("GPS: ",result)
                        self.cached_gps_value = copy(self.active_gps_value)
                        self.active_gps_value.lat = result.get("lat", self.cached_gps_value.lat)
                        self.active_gps_value.lon = result.get("lon", self.cached_gps_value.lon)
                        self.active_gps_value.time = result.get("time", "")
                        self.active_gps_value.alt = result.get("alt", "")
                        self.active_gps_value.speed = result.get("speed", "")
                        self.set_bearing()
                        time.sleep(2)

            except ConnectionRefusedError:
                print("Connection to GPSD refused.")
                time.sleep(2)

    def join(self):
        print("GPS Thread Joining")
        self.gps_client.close()
        super().join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="User Interface for Road Messages")
    parser.add_argument("-v", "--vehicle-name", required=True, help="Name of the vehicle")
    parser.add_argument("-l", "--is-local", required=True, help="Whether the connection is local (true/false)")
    parser.add_argument("-s", "--save-message", required=False, default=False, action="store_true",
                        help="Save messages")
    args = parser.parse_args()
    print(args)
    # Create a mqtt wrapper class for the client and connect to the broker
    # and handle all the callbacks and publish the messages to a queue which is shared with the UI thread.

    window = tk.Tk()
    window.title(UI_WINDOW_TITLE)
    label = tk.Label(window)
    label.pack()
    try:
       # ui_message_queue = queue.PriorityQueue()
        ui_message_queue = MsgPriorityQueue()
        gps_thread = GPSHandler()
        gps_thread.daemon = True
        gps_thread.start()
        ui = UserInterface("ui_thread", ui_message_queue, window, label)
        ui.daemon = True
        mqtt_handler = MqttHandler("mqtt_thread", args.vehicle_name, gps_thread, ui_message_queue, args.is_local,
                                   args.save_message)
        mqtt_handler.daemon = True
        mqtt_handler.start()
        ui.start()
        window.mainloop()
    except KeyboardInterrupt:
        mqtt_handler.join()
        ui.join()
        gps_thread.join()
        print("Exiting")

    # thd = threading.Thread(target=runtk)   # gui thread
    # thd.daemon = True  # background thread will exit if main thread exits
    # thd.start()

