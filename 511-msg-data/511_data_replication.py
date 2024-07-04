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


def update_json_file(filepath, new_data, changes_file):
    with open(filepath, "r") as file:
        existing_data = json.load(file)

    modified_objects = []
    existing_objects = existing_data if isinstance(existing_data, list) else []

    for new_object in new_data:
        matching_objects = [obj for obj in existing_objects if obj.get("id") == new_object.get("id")]

        if not any(compare_json_objects(existing_object, new_object) for existing_object in matching_objects):
            modified_objects.append(new_object)
            changes_file.writerow([new_object.get("id"), datetime.now().strftime("%Y-%m-%d %H:%M:%S")])

    if modified_objects:
        updated_data = modified_objects + existing_objects
        with open(filepath, "w") as file:
            json.dump(updated_data, file, indent=4)
        print(f"Modifications in {filepath} applied.")
    else:
        print(f"No modifications found in {filepath}. Skipping update.")



def dump_json_links_to_files(links):
    directory = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(directory, "511_New_Datasource")
    changes_file_path = os.path.join(directory, "511_New_Datasource","modifications.csv")

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(changes_file_path, "a", newline="") as changes_file:
        changes_writer = csv.writer(changes_file)
        if os.path.getsize(changes_file_path) == 0:
            changes_writer.writerow(["Object ID", "Timestamp"])

        for link in links:
            filename = f"Current_{link.split('/')[-1]}"
            filepath = os.path.join(output_dir, filename)

            try:
                with urllib.request.urlopen(link) as url:
                    data = url.read().decode()
                    json_data = json.loads(data)

                    if os.path.exists(filepath):
                        update_json_file(filepath, json_data, changes_writer)
                    else:
                        with open(filepath, "w") as file:
                            json.dump(json_data, file, indent=4)
                        print(f"JSON data from {link} dumped to {filename}")
            except Exception as e:
                print(f"Error occurred while processing {link}: {e}")

    # Run the JSON dumping process every hour
    time.sleep(SLEEP_TIME)  # Sleep for half an hour
    dump_json_links_to_files(links)


# List of JSON links
json_links = [
    "https://content.trafficwise.org/json/cars-event-feed.json"
]

# Start the JSON dumping process
dump_json_links_to_files(json_links)
