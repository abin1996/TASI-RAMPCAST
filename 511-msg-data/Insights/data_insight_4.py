import mysql.connector
import folium
import time
from datetime import datetime
from pytz import timezone

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

# Retrieve event data with latitude, longitude, and dateStart from the database
query = f"SELECT eventType, startLatitude, startLongitude, dateStart, foreign_id FROM {TRAFFIC_INFO_TABLE} WHERE startLatitude IS NOT NULL AND startLongitude IS NOT NULL AND eventType LIKE 'CRASH%'"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Create a map object
m = folium.Map(location=[39.782078, -86.167522], zoom_start=10)

# Add markers for each event
for row in results:
    event_type, latitude, longitude, date_start_epoch, foreign_id = row

    # Extract the valid portion of the timestamp based on its length
    if len(date_start_epoch) == 13:
        date_start_epoch = int(date_start_epoch) // 1000  # Remove milliseconds
    elif len(date_start_epoch) != 10:
        print(f"Ignoring invalid date_start_epoch: {date_start_epoch}")
        continue

    # Convert epoch time to readable format in EST
    est = timezone('US/Eastern')
    try:
        date_start = datetime.fromtimestamp(int(date_start_epoch), est).strftime('%Y-%m-%d %H:%M:%S')
    except ValueError as e:
        print("ValueError:", e)
        continue

    # Add a marker to the map with eventType, dateStart, and foreign_id
    popup_text = f"Event Type: {event_type}<br>Date Start (EST): {date_start}<br>Foreign ID: {foreign_id}"
    folium.Marker([latitude, longitude], popup=popup_text).add_to(m)

# Display the map
m.save("event_map.html")
