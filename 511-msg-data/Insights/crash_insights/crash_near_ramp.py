import mysql.connector
import datetime
from math import radians, sin, cos, sqrt, atan2
import pandas as pd

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

# Define the radius distances in miles
radius_distances = [5, 10, 20]

# Create a DataFrame to store the crash event details
columns = ["Event ID", "Foreign ID", "Priority Level", "Event Type", "Date Start", "Date End", "Latitude", "Longitude", "Distance (miles)"]
crash_events_data = []

for radius in radius_distances:
    # Convert the radius from miles to kilometers
    radius_km = radius * 1.60934

    # Calculate the bounding box coordinates
    lat_min = lat - (radius_km / 111)
    lat_max = lat + (radius_km / 111)
    lon_min = lon - (radius_km / (111 * cos(radians(lat))))
    lon_max = lon + (radius_km / (111 * cos(radians(lat))))

    # Retrieve crash event data within the bounding box
    query = f"SELECT id, foreign_id, priorityLevel, eventType, dateStart, dateEnd, startLatitude, startLongitude FROM {TRAFFIC_INFO_TABLE} WHERE startLatitude BETWEEN {lat_min} AND {lat_max} AND startLongitude BETWEEN {lon_min} AND {lon_max} AND eventType LIKE 'CRASH%'"
    cursor.execute(query)
    results = cursor.fetchall()

    # Calculate the distance between the location and each event
    for row in results:
        event_id, foreign_id, priority_level, event_type, date_start, date_end, event_lat, event_lon = row

        # Convert the decimal values to float
        event_lon = float(event_lon)
        event_lat = float(event_lat)

        # Check if date_end is not None
        if date_end != 'None':
            date_end = datetime.datetime.fromtimestamp(int(date_end) / 1000)
        else:
            date_end = None

        # Calculate the distance using the haversine formula
        dlon = radians(event_lon - lon)
        dlat = radians(event_lat - lat)
        a = sin(dlat / 2) ** 2 + cos(radians(lat)) * cos(radians(event_lat)) * sin(dlon / 2) ** 2
        c = 2 * atan2(sqrt(a), sqrt(1 - a))
        distance_km = 6371 * c
        distance_miles = distance_km / 1.60934

        crash_events_data.append([
            event_id,
            foreign_id,
            priority_level,
            event_type,
            datetime.datetime.fromtimestamp(int(date_start) / 1000),
            date_end,
            event_lat,
            event_lon,
            distance_miles
        ])

# Close the database connection
db.close()

crash_events_df = pd.DataFrame(crash_events_data, columns=columns)
print(crash_events_df)
