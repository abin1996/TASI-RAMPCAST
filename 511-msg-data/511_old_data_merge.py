import os
import json
import urllib.request
from datetime import datetime
import time
import csv

SLEEP_TIME = 1800

def compare_json_objects(json_obj1, json_obj2):
    if type(json_obj1) != type(json_obj2):
        return False

    if isinstance(json_obj1, dict):
        if set(json_obj1.keys()) != set(json_obj2.keys()):
            return False
        for key in json_obj1.keys():
            if not compare_json_objects(json_obj1[key], json_obj2[key]):
                return False
        return True

    if isinstance(json_obj1, list):
        if len(json_obj1) != len(json_obj2):
            return False
        for i in range(len(json_obj1)):
            if not compare_json_objects(json_obj1[i], json_obj2[i]):
                return False
        return True

    return json_obj1 == json_obj2


def update_json_file(filepath, new_data):
    with open(filepath, "r") as file:
        existing_data = json.load(file)

    modified_objects = []
    existing_objects = existing_data if isinstance(existing_data, list) else []

    for new_object in new_data:
        matching_objects = [obj for obj in existing_objects if obj.get("id") == new_object.get("id")]

        if not any(compare_json_objects(existing_object, new_object) for existing_object in matching_objects):
            modified_objects.append(new_object)

    if modified_objects:
        updated_data = modified_objects + existing_objects
        with open(filepath, "w") as file:
            json.dump(updated_data, file, indent=4)
        print(f"Modifications in {filepath} applied.")
    else:
        print(f"No modifications found in {filepath}. Skipping update.")



def merge_old_data():
    directory = os.path.dirname(os.path.abspath(__file__))
    input_dir = os.path.join(directory, "sample_car_events")
    output_filename = "old_car_events.json"
    output_filepath = os.path.join(input_dir, output_filename)
    for filename in os.listdir(input_dir):
        filepath = os.path.join(input_dir, filename)
        
        # Check if the file is a JSON file
        if os.path.isfile(filepath) and filename.endswith(".json"):
            with open(filepath, "r") as file:
                try:
                    json_data = json.load(file)
                    if os.path.exists(output_filepath):
                        update_json_file(output_filepath, json_data)
                    else:
                        with open(output_filepath, "w") as file:
                            json.dump(json_data, file, indent=4)
                        print(f"JSON data from dumped from {filename}")
                except Exception as e:
                    print(f"Error occurred while processing {e}")
    
merge_old_data()