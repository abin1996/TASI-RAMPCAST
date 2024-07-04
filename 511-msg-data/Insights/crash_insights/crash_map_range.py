import mysql.connector
import folium

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

# Retrieve crash event data from the database
query = f"SELECT startLatitude, startLongitude, endLatitude, endLongitude, eventType, priorityLevel FROM {TRAFFIC_INFO_TABLE} WHERE eventType LIKE 'CRASH%' AND startLatitude IS NOT NULL AND startLongitude IS NOT NULL AND endLatitude IS NOT NULL AND endLongitude IS NOT NULL"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Create a map object
crash_map = folium.Map(location=[39.782161, -86.167588], zoom_start=12)

# Define colors for different severity levels
severity_colors = {
    "CRASH PD": "blue",
    "CRASH PI": "green",
    "CRASH FATAL": "red"
}

# Plot the crash locations and line range on the map
for row in results:
    start_lat, start_lon, end_lat, end_lon, event_type, priority_level = row
    color = severity_colors.get(event_type, "gray")  # Use gray color for unknown severity levels

    try:
        start_lat = float(start_lat)
        start_lon = float(start_lon)
        end_lat = float(end_lat)
        end_lon = float(end_lon)

        # Create a marker for each crash event at the start location
        folium.Marker(
            location=[start_lat, start_lon],
            popup=f"Event Type: {event_type}\nPriority Level: {priority_level}",
            icon=folium.Icon(color=color)
        ).add_to(crash_map)

        # Create a line connecting the start and end locations
        folium.PolyLine(
            locations=[(start_lat, start_lon), (end_lat, end_lon)],
            color=color,
            popup=f"Event Type: {event_type}\nPriority Level: {priority_level}"
        ).add_to(crash_map)
    except ValueError:
        # Skip invalid latitude and longitude values
        continue

# Save the map to an HTML file
crash_map.save("crash_location_map_with_range.html")
