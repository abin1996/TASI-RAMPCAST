import os
import requests
import json
from math import radians, sin, cos, sqrt, atan2
import time

# Get the directory of the current script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Set this flag to True to use a local file for cars-event-feed.json
USE_LOCAL_CARS_EVENT_FEED = True

RSU_COORDINATES = [39.782159, -86.167573]
def haversine(lat1, lon1, lat2, lon2):
    R = 3958.8  # Radius of the Earth in miles

    lat1_rad = radians(lat1)
    lon1_rad = radians(lon1)
    lat2_rad = radians(lat2)
    lon2_rad = radians(lon2)

    dlon = lon2_rad - lon1_rad
    dlat = lat2_rad - lat1_rad

    a = sin(dlat / 2)**2 + cos(lat1_rad) * cos(lat2_rad) * sin(dlon / 2)**2
    c = 2 * atan2(sqrt(a), sqrt(1 - a))

    distance = R * c
    return distance

def extract_interstate(device_label):
    """
    Extract the direction from the 'device_label' field.

    Parameters:
    device_label (str): The value of the 'device_label' field containing the direction.

    Returns:
    The direction as a string (e.g., 'NB', 'SB', 'EB', 'WB').
    """
    parts = device_label.split('-')
    return int(parts[1])
def extract_direction(device_label):
    """
    Extract the direction from the 'device_label' field.

    Parameters:
    device_label (str): The value of the 'device_label' field containing the direction.

    Returns:
    The direction as a string (e.g., 'NB', 'SB', 'EB', 'WB').
    """
    parts = device_label.split('-')
    return parts[4]

def extract_mile_marker(device_label):
    """
    Extract the mile marker from the 'device_label' field.

    Parameters:
    device_label (str): The value of the 'device_label' field containing the mile marker.

    Returns:
    The mile marker as a float.
    """
    parts = device_label.split('-')
    integer_part = int(parts[2])
    decimal_part = int(parts[3]) / 10.0
    mile_marker = integer_part + decimal_part
    return mile_marker

def filter_messages_new_format(data, message_type, max_entries=5):
    new_data = []
    with open("../Demo/rsu_config.json", "r") as f:
        rsu_config = json.load(f)
    device_ids_to_filter = [int(sign['id']) for sign in rsu_config['signs']]
    print(device_ids_to_filter)
    entry_count = 0  # Keep track of the number of selected entries
    for feature in data['features']:
        if entry_count >= max_entries:
            break  # Stop processing after reaching the maximum number of entries
        device_id = int(feature['properties']['device_nbr'])
        if device_id in device_ids_to_filter and feature["geometry"]['type'] == 'Point':
            signs_list = rsu_config['signs']
            distance_to_rsu = 0
            time_to_rsu = 0
            lsign_distance = 0
            rsign_distance = 0
            for sign in signs_list:
                if(sign['id'] == str(feature['properties']['device_nbr'])):
                    distance_to_rsu = sign['distance_to_rsu']
                    time_to_rsu = sign['time_to_rsu']
                    lsign_distance = sign.get('lsign_distance',0)
                    rsign_distance = sign.get('rsign_distance',0)
            print(feature['properties']['device_label'])
            new_feature = {
                "interstate": extract_interstate(feature['properties']['device_label']),
                "direction": extract_direction(feature['properties']['device_label']),
                "milemarker": extract_mile_marker(feature['properties']['device_label']),
                "geometry": feature['geometry'],
                "distance_to_rsu": distance_to_rsu,
                "time_to_rsu": time_to_rsu,
                "lsign_distance": lsign_distance,
                "rsign_distance": rsign_distance,
                "title": feature['properties']['title'],
                "ontime": feature['properties']['ontime'],
                "offtime": feature['properties']['offtime'],
                "message1": feature['properties']['message1'],
                "message2": feature['properties']['message2'],
                "message3": feature['properties']['message3'],
                "priority": feature['properties']['priority'],
                "eventType": message_type,
                "message_type": message_type,
                "device_nbr": feature['properties']['device_nbr'],
                "device_label": feature['properties']['device_label'],
                "repeat": 6 - int(feature['properties']['priority']) if int(feature['properties']['priority']) <= 3 else 1,
                "rsu_coordinates": RSU_COORDINATES
            }
            new_data.append(new_feature)
            entry_count += 1  # Increment the entry count
    return new_data



RADIUS_THRESHOLD = 5.0
def convert_to_new_format(data, message_type, max_entries=5):
    new_data = []
    entry_count = 0  # Keep track of the number of selected entries

    for feature in data['features']:
        if entry_count >= max_entries:
            break  # Stop processing after reaching the maximum number of entries

        if feature["geometry"]['type'] == 'Point':
            lon, lat = feature["geometry"]["coordinates"][0],feature["geometry"]["coordinates"][1]
            distance_to_rsu = haversine(RSU_COORDINATES[0], RSU_COORDINATES[1], lat, lon)
            print(feature['properties']['device_label'])

        if distance_to_rsu <= RADIUS_THRESHOLD:
            new_feature = {
                "interstate": extract_interstate(feature['properties']['device_label']),
                "direction": extract_direction(feature['properties']['device_label']),
                "milemarker": extract_mile_marker(feature['properties']['device_label']),
                "geometry": feature['geometry'],
                "distance_from_rsu": distance_to_rsu,
                "title": feature['properties']['title'],
                "ontime": feature['properties']['ontime'],
                "offtime": feature['properties']['offtime'],
                "message1": feature['properties']['message1'],
                "message2": feature['properties']['message2'],
                "message3": feature['properties']['message3'],
                "priority": feature['properties']['priority'],
                "eventType": message_type,
                "message_type": message_type,
                "device_nbr": feature['properties']['device_nbr'],
                "device_label": feature['properties']['device_label'],
                "timestamp": int(time.time()),  # Current epoch timestamp
                "repeat": 6 - int(feature['properties']['priority']) if int(feature['properties']['priority']) <= 3 else 1,
                "rsu_coordinates": RSU_COORDINATES
            }
            entry_count += 1  # Increment the entry count
            new_data.append(new_feature)
    return new_data

MAX_MESSAGE_LENGTH = 20
def break_message(message):
    broken_message = []
    words = message.split()
    current_line = ""
    for word in words:
        if len(current_line) + len(word) + 1 <= MAX_MESSAGE_LENGTH:
            current_line += word + " "
        else:
            broken_message.append(current_line.strip())
            current_line = word + " "
    broken_message.append(current_line.strip())
    return broken_message

RADIUS_CAREVENT_THRESHOLD = 2.0


def convert_car_events_to_new_format(car_events, max_entries=5):
    new_data = []
    entry_count = 0  # Keep track of the number of selected entries
    for event in car_events:
        if entry_count >= max_entries:
            break  # Stop processing after reaching the maximum number of entries
        geometry = event["geometry"]
        if geometry:
            if geometry.startswith("POINT"):
                lon, lat = map(float, geometry.split("(")[1].split(")")[0].split())

                geometry_data = {
                    "type": "POINT",
                    "coordinates": [lon, lat]
                }
            else:  # For MULTILINESTRING
                coordinates = geometry.split("((")[1].split(")")[0].split(",")
                lat_lon_pairs = [pair.split() for pair in coordinates]
                lat, lon = map(float, lat_lon_pairs[0])  # Use the first point

                geometry_data = {
                    "type": "MULTILINESTRING",
                    "coordinates": [lon, lat]
                }
            distance_to_rsu = haversine(RSU_COORDINATES[0], RSU_COORDINATES[1], lat, lon)
            if distance_to_rsu <= RADIUS_CAREVENT_THRESHOLD:
                event_type = event["eventType"]
                start_mile = event["startMileMarker"]
                route = event["route"]
                cardinal_dir = event['locationDetails']['lanes']['positive']['cardinalDir']
                if cardinal_dir == 'N':
                    cardinal_dir = 'NB'
                if cardinal_dir == 'S':
                    cardinal_dir = 'SB'
                if cardinal_dir == 'E':
                    cardinal_dir = 'EB'
                if cardinal_dir == 'W':
                    cardinal_dir = 'WB'

                if 'CRASH' in event_type:
                    # CRASH on I-65 NB, 3 miles ahead. Drive with caution.
                    message = f"CRASH ON {route}{cardinal_dir}, {start_mile} MILES AHEAD DRIVE WITH CAUTION"
                else:
                    message = f"{event_type} ON {route} {cardinal_dir}, {start_mile} MILES AHEAD"

                broken_messages = break_message(message)
                message1 = broken_messages[0] if len(broken_messages) >= 1 else ""
                message2 = broken_messages[1] if len(broken_messages) >= 2 else ""
                message3 = broken_messages[2] if len(broken_messages) >= 3 else ""

                new_feature = {
                    "interstate": extract_interstate(route),
                    "direction": cardinal_dir,
                    "milemarker": start_mile,
                    "geometry": geometry_data,
                    "distance_from_rsu": distance_to_rsu,
                    "message1": message1,
                    "message2": message2,
                    "message3": message3,
                    "priority": event['priorityLevel'],
                    "message_type": "car-event",
                    "eventType": event["eventType"],
                    "startMileMarker": event["startMileMarker"],
                    "cardinal_dir": event['locationDetails']['lanes']['positive']['cardinalDir'],
                    "device_nbr": event['id'],
                    "device_label": route,
                    "timestamp": int(time.time()),
                    "repeat": 6 - int(event['priorityLevel']) if int(event['priorityLevel']) <= 3 else 1,
                    "rsu_coordinates":RSU_COORDINATES
                }
                new_data.append(new_feature)
                entry_count += 1  # Increment the entry count
    return new_data


# def main():
#     if USE_LOCAL_CARS_EVENT_FEED:
#         with open(LOCAL_CARS_EVENT_FEED_FILE, "r") as local_cars_event_feed:
#             car_events = json.load(local_cars_event_feed)
#     else:
#         response = requests.get("https://content.trafficwise.org/json/cars-event-feed.json")
#         if response.status_code == 200:
#             car_events = json.loads(response.text)
#
#     new_data = convert_car_events_to_new_format(car_events)
#     combined_data["RSU_Messages"].extend(new_data)
#
#     for url in urls:
#         response = requests.get(url)
#         if response.status_code == 200:
#             data = json.loads(response.text)
#             message_type = "tts" if "tts" in url else "dms"
#             new_data = filter_messages_new_format(data, message_type)
#             combined_data["RSU_Messages"].extend(new_data)
#
#     json_file_path = os.path.join(SCRIPT_DIR, "RSU_Messages.json")
#
#     with open(json_file_path, "w") as f:
#         json.dump(combined_data, f, indent=2)
#
# if __name__ == "__main__":
#     main()

def main():
    urls = [
        "https://content.trafficwise.org/json/tts.json",
        "https://content.trafficwise.org/json/dms_tt.json",
    ]

    combined_data = {
        "RSU_Coordinates": RSU_COORDINATES,
        "RSU_Messages": []
    }

    if USE_LOCAL_CARS_EVENT_FEED:
        with open("cars-event-feed-local.json", "r") as local_cars_event_feed:
            car_events = json.load(local_cars_event_feed)
    else:
        response = requests.get("https://content.trafficwise.org/json/cars-event-feed.json")
        if response.status_code == 200:
            car_events = json.loads(response.text)

    new_data = convert_car_events_to_new_format(car_events)
    combined_data["RSU_Messages"].extend(new_data)

    for url in urls:
        response = requests.get(url)
        if response.status_code == 200:
            data = json.loads(response.text)
            message_type = "tts" if "tts" in url else "dms"
            new_data = filter_messages_new_format(data, message_type)
            combined_data["RSU_Messages"].extend(new_data)

    json_file_path = os.path.join(SCRIPT_DIR, "RSU_Messages.json")

    with open(json_file_path, "w") as f:
        json.dump(combined_data, f, indent=2)

if __name__ == "__main__":
    main()
