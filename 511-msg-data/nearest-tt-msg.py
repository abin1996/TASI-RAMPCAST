import os
import requests
from math import radians, sin, cos, sqrt, atan2
from PIL import Image
from io import BytesIO


# Function to calculate the distance between two coordinates using the Haversine formula
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

# Target ramp location coordinates
target_lat = 39.782161
target_lon = -86.167588

# Load JSON data from the provided URL
url = "https://content.trafficwise.org/json/tts.json"
response = requests.get(url)
data = response.json()

# List to store the records with their distances from the target ramp
records_with_distances = []

# Iterate through each feature in the JSON data
for feature in data["features"]:
    coords = feature["geometry"]["coordinates"]
    lon, lat = coords[0], coords[1]

    # Calculate the distance between the target ramp and the current record
    distance = haversine(target_lat, target_lon, lat, lon)

    # Add the record, its distance, and the image link to the list
    device_nbr = feature["properties"]["device_nbr"]
    image_link = f"https://511in.org/images/overlay-signs/{device_nbr}.png"
    records_with_distances.append((feature, distance, image_link))

# Sort the records based on their distances in ascending order
records_with_distances.sort(key=lambda x: x[1])

# Number of nearest records to display
num_nearest_records = 2000

# Create the folder to save the images if it doesn't exist
if not os.path.exists("Road-signs-template"):
    os.makedirs("Road-signs-template")

# Print the nearest records with distances in miles and download the images
print(f"Nearest {num_nearest_records} records from the target ramp (in miles):")

for i, (feature, distance, image_link) in enumerate(records_with_distances[:num_nearest_records], start=1):
    title = feature["properties"]["title"]
    lat, lon = feature["geometry"]["coordinates"]
    print(f"{i}. Title: {title}, Latitude: {lat}, Longitude: {lon}, Distance: {distance:.2f} miles, ")

    # Download the image for the current device number
    image_response = requests.get(image_link)
    image = Image.open(BytesIO(image_response.content))
    device_nbr = feature["properties"]["device_nbr"]
    image_path = os.path.join("Road-signs-template", f"{device_nbr}.png")
    image.save(image_path)  # Save the image in the folder with the device number as the filename