import paho.mqtt.client as mqtt
import time
import re
import json
from math import radians, sin, cos, sqrt, atan2

RSU_ORIGIN_MILE_MARKER = 113
RSU_INTERSTATES = [65]
def on_connect(client, userdata, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("V2X-Traveller-Information")

def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))

pub_messages = 0

def on_publish(client, userdata, mid):
    global pub_messages
    pub_messages += 1
    print("Published: " + str(mid))


def filter_signs_on_rsu_loc(rsu_data,max_dist_miles, msg_type):
    
    """
    Remove objects from the list whose distance is greater than the given limit.

    Parameters:
    objects_list (list): List of objects, each containing a "distance_from_rsu" field.
    distance_limit (float): The maximum distance allowed. Objects with distance greater than this limit will be removed.

    Returns:
    A new list with objects that satisfy the distance limit.
    """
    filtered_list = [obj for obj in rsu_data if float(obj["distance_from_rsu"]) <= max_dist_miles and obj["message_type"] == msg_type]
    return filtered_list
def adjust_tts_messages(data, origin_mile_marker, origin_interstate, origin_direction):
    """
    Adjust the values of message fields (message1, message2, etc.) when message_type is "tts".

    Parameters:
    data (list): List of objects containing the data.
    origin_mile_marker (float): Origin mile marker to be used for adjustment.
    origin_interstate (int): Origin interstate to be used for adjustment.
    origin_direction (str): Origin direction to be used for adjustment.

    Returns:
    A new list with adjusted message fields for the objects.
    """
    adjusted_data = []
    for obj in data:
        if obj["message_type"] == "tts":
            interstate = obj.get("interstate")
            direction = obj.get("direction")
            mile_marker = obj.get("milemarker")

            if interstate == origin_interstate and direction == origin_direction:
                adjustment = abs(origin_mile_marker - mile_marker)
                for key in obj:
                    if key.startswith("message") and obj[key] != "":
                        obj[key] = str(int(obj[key]) + int(adjustment))

        adjusted_data.append(obj)

    return adjusted_data
def adjust_tts_messages_radial(data):
    adjusted_data = []
    for obj in data:
        if obj["message_type"] == "tts":
            adjustment = round(obj.get("distance_from_rsu"))
            for key in obj:
                if re.match(r'^message\d+$', key) and obj[key] != "":
                    obj[key] = str(int(obj[key]) + int(adjustment))

        adjusted_data.append(obj)
    return adjusted_data

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_publish = on_publish
    client.username_pw_set('mqtt-test1', 'tasi')
    client.connect("localhost", 1883, 60)

    with open("./Demo/RSU_Messages.json", "r") as f:
        rsu_data = json.load(f)

    signs_list = rsu_data["RSU_Messages"]
    # filtered_signs_list = filter_signs_on_rsu_loc(signs_list,3,"tts")
    # print(filtered_signs_list)

    # print("****************************************")
    # filtered_signs_list.extend(filter_signs_on_rsu_loc(signs_list,8,"dms"))
    # adjusted_signs_list = adjust_tts_messages_radial(filtered_signs_list)
    while True:
        for sign in signs_list:
            # Serialize and encode the message
            MESSAGE = json.dumps(sign).encode()
            client.publish("V2X-Traveller-Information", MESSAGE)
            time.sleep(1)

    client.disconnect()
    print("done")

if __name__ == "__main__":
    main()
