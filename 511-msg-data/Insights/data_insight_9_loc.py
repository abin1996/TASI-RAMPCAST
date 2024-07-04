import mysql.connector
from math import radians, sin, cos, sqrt, atan2

# Connect to the MySQL database
db = mysql.connector.connect(
    host='localhost',
    user='root',
    password='root1234',
    database='TrafficInfoDB'
)

# Create a cursor object to interact with the database
cursor = db.cursor()

# Specify the latitude and longitude of the location
lat = 39.782161
lon = -86.167588

# Define the radius distances in miles
radius_distances = [5, 10, 20]

# Retrieve event data within the specified radius distances
nearest_events = []

for radius in radius_distances:
    # Convert the radius from miles to kilometers
    radius_km = radius * 1.60934

    # Calculate the bounding box coordinates
    lat_min = lat - (radius_km / 111)
    lat_max = lat + (radius_km / 111)
    lon_min = lon - (radius_km / (111 * cos(radians(lat))))
    lon_max = lon + (radius_km / (111 * cos(radians(lat))))

    # Retrieve event data within the bounding box
    query = f"SELECT id, foreign_id, priorityLevel, eventType, dateStart, dateEnd, startLatitude, startLongitude FROM TrafficMessages WHERE startLatitude BETWEEN {lat_min} AND {lat_max} AND startLongitude BETWEEN {lon_min} AND {lon_max}"
    cursor.execute(query)
    results = cursor.fetchall()

    # Calculate the distance between the location and each event
    for row in results:
        event_id, foreign_id, priority_level, event_type, date_start, date_end, event_lat, event_lon = row

        # Convert the decimal values to float
        event_lon = float(event_lon)
        event_lat = float(event_lat)

        # Calculate the distance using the haversine formula
        dlon = radians(event_lon - lon)
        dlat = radians(event_lat - lat)
        a = sin(dlat / 2) ** 2 + cos(radians(lat)) * cos(radians(event_lat)) * sin(dlon / 2) ** 2
        c = 2 * atan2(sqrt(a), sqrt(1 - a))
        distance_km = 6371 * c
        distance_miles = distance_km / 1.60934

        if distance_miles <= radius:
            nearest_events.append((event_id, foreign_id, priority_level, event_type, date_start, date_end, event_lat, event_lon, distance_miles))

# Retrieve location information for the nearest events
for event in nearest_events:
    event_id, foreign_id, priority_level, event_type, date_start, date_end, event_lat, event_lon, distance = event
    
    # Retrieve location info
    location_query = f"SELECT * FROM LocationDetails WHERE traffic_info_id={foreign_id}"
    cursor.execute(location_query)
    location_info = cursor.fetchone()

    print(f"Event ID: {event_id}")
    print(f"Foreign ID: {foreign_id}")
    print(f"Priority Level: {priority_level}")
    print(f"Event Type: {event_type}")
    print(f"Date Start: {date_start}")
    print(f"Date End: {date_end}")
    print(f"Latitude: {event_lat}")
    print(f"Longitude: {event_lon}")
    print(f"Distance: {distance:.2f} miles")

    if location_info is not None:
        location_id, traffic_info_id, district, county, subdistrict, city, region, unit = location_info
        print(f"Location ID: {location_id}")
        print(f"District: {district}")
        print(f"County: {county}")
        print(f"Subdistrict: {subdistrict}")
        print(f"City: {city}")
        print(f"Region: {region}")
        print(f"Unit: {unit}")
    else:
        print("Location info not found")

    print()
    
# Close the database connection
db.close()
