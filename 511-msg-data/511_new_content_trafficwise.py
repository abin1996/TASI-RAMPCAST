import os
import json
import urllib.request
from datetime import datetime
import time

def dump_json_links_to_files(links):
    current_datetime = datetime.now().strftime("%Y%m%d_%H%M%S")
    directory = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(directory, "511_New_Datasource")

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for link in links:
        filename = f"{current_datetime}_{link.split('/')[-1]}"
        filepath = os.path.join(output_dir, filename)

        try:
            with urllib.request.urlopen(link) as url:
                data = url.read().decode()
                json_data = json.loads(data)
                with open(filepath, "w") as file:
                    json.dump(json_data, file, indent=4)
                    print(f"JSON data from {link} dumped to {filename}")
        except Exception as e:
            print(f"Error occurred while processing {link}: {e}")

# List of JSON links
json_links = [
    "https://content.trafficwise.org/json/cars-event-feed.json",
    "https://content.trafficwise.org/json/dms_tt.json",
    "https://content.trafficwise.org/json/hh.json",
    "https://content.trafficwise.org/json/tts.json",
    "https://content.trafficwise.org/json/tpims.json"
]

# Run the JSON dumping process every hour
while True:
    dump_json_links_to_files(json_links)
    time.sleep(1200)  # Sleep for 1 hour (3600 seconds)
