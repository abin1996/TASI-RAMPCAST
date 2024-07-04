import mysql.connector
import datetime
from math import radians, sin, cos, sqrt, atan2
import folium
import pytz

# Connect to the MySQL database
db = mysql.connector.connect(
    host='localhost',
    user='root',
    password='root1234',
    database='TrafficInfoDB'
)

TRAFFIC_INFO_TABLE = "TrafficMessages"

# Create a cursor object to interact with the database
cursor = db.cursor()

# Specify the latitude and longitude of the location
lon = -86.167588
lat = 39.782161

# Get the radius distance from the user
radius_distance = float(input("Enter the radius distance in miles: "))

# Create a folium map centered on the specified location
map_center = [lat, lon]
crash_map = folium.Map(location=map_center, zoom_start=12)

# Mark the specified location on the map
folium.Marker(
    location=map_center,
    popup='TARGET RAMP',
    icon=folium.Icon(color='blue')
).add_to(crash_map)

# Function to get the distance between two coordinates in miles
def get_distance(coord1, coord2):
    lat1, lon1 = coord1
    lat2, lon2 = coord2
    dlon = radians(lon2 - lon1)
    dlat = radians(lat2 - lat1)
    a = sin(dlat / 2) ** 2 + cos(radians(lat1)) * cos(radians(lat2)) * sin(dlon / 2) ** 2
    c = 2 * atan2(sqrt(a), sqrt(1 - a))
    distance_km = 6371 * c
    distance_miles = distance_km / 1.60934
    return distance_miles

# Convert the radius from miles to kilometers
radius_km = radius_distance * 1.60934

# Calculate the bounding box coordinates
lat_min = lat - (radius_km / 111)
lat_max = lat + (radius_km / 111)
lon_min = lon - (radius_km / (111 * cos(radians(lat))))
lon_max = lon + (radius_km / (111 * cos(radians(lat))))

# Retrieve crash event data within the bounding box and with distinct foreign_id
query = f"SELECT DISTINCT foreign_id, eventType, priorityLevel, startLatitude, startLongitude, dateStart FROM {TRAFFIC_INFO_TABLE} WHERE startLatitude BETWEEN {lat_min} AND {lat_max} AND startLongitude BETWEEN {lon_min} AND {lon_max} AND eventType LIKE 'CRASH%'"
cursor.execute(query)
results = cursor.fetchall()

# List to store crash event records found
crash_events_list = []

# Loop through each crash event with distinct foreign_id
for row in results:
    foreign_id, event_type, priority_level, event_lat, event_lon, date_start = row

    # Convert the decimal values to float
    event_lon = float(event_lon)
    event_lat = float(event_lat)

    # Convert epoch timestamp to datetime object and adjust for EST time zone
    date_start = datetime.datetime.fromtimestamp(int(date_start) / 1000).astimezone(pytz.timezone('US/Eastern'))

    # Calculate the distance between the location and the crash event
    distance_miles = get_distance((lat, lon), (event_lat, event_lon))

    # Define marker colors based on eventType
    if event_type == 'CRASH PD':
        marker_color = 'green'
    elif event_type == 'CRASH PI':
        marker_color = 'orange'
    elif event_type == 'CRASH FATAL':
        marker_color = 'red'
    else:
        marker_color = 'blue'  # Default color for other eventTypes

    # Mark the crash event on the map with the corresponding color
    folium.Marker(
        location=[event_lat, event_lon],
        popup=f"Foreign ID: {foreign_id}\nEvent Type: {event_type}\nPriority Level: {priority_level}\nDate Start: {date_start.strftime('%Y-%m-%d %H:%M:%S')}\nDistance: {distance_miles:.2f} miles",
        icon=folium.Icon(color=marker_color)
    ).add_to(crash_map)

    # Add crash event record to the list
    crash_events_list.append((foreign_id, event_type, priority_level, event_lat, event_lon, date_start, distance_miles))


# Save the map to an HTML file
crash_map.save("crash_events_ramp_map.html")

# Close the database connection
db.close()

# Print the list of crash event records found
print("Crash Events Found:")
for crash_event in crash_events_list:
    foreign_id, event_type, priority_level, event_lat, event_lon, date_start, distance_miles = crash_event
    print(f"Foreign ID: {foreign_id}, Event Type: {event_type}, Priority Level: {priority_level}, Latitude: {event_lat}, Longitude: {event_lon}, Date Start: {date_start}, Distance: {distance_miles:.2f} miles")
