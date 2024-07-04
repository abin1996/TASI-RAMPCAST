import os
import json
import mysql.connector
import re

# Connect to the MySQL database
db = mysql.connector.connect(
    host='localhost',
    user='root',
    password='root1234',
    database='TrafficInfoDB'
)

TRAFFIC_INFO_TABLE = "TrafficMessages"
LOCATION_DETAILS_TABLE = "LocationDetails"
required_columns_from_json = ['id', 'eventType', 'priorityLevel', 'eventStatus', 'dateStart', 'dateUpdated', 'dateEnd',
                              'startMileMarker', 'endMileMarker', 'route', 'geometry', 'locationDetails']
required_columns_from_json_for_locationTable = ['district', 'county', 'subdistrict', 'city', 'region']
# Create a cursor object to interact with the database
cursor = db.cursor()

def extract_coordinates(geometry):
    coordinates = re.findall(r"-?\d+\.\d+", geometry)
    return [round(float(coord), 6) for coord in coordinates]

def process_geometry(geometry):
    if geometry.startswith("POINT"):
        # Point Geometry
        coordinates = extract_coordinates(geometry)
        start_long, start_lat = coordinates
        end_long, end_lat = None, None
    elif geometry.startswith("MULTILINESTRING"):
        # Multiline String Geometry
        coordinates = extract_coordinates(geometry)
        start_long, start_lat = coordinates[0], coordinates[1]
        end_long, end_lat = coordinates[-2], coordinates[-1]
    else:
        # Invalid geometry type
        start_long, start_lat, end_long, end_lat = None, None, None, None

    return start_long, start_lat, end_long, end_lat

def create_table():
    # Create the table if it doesn't exist
    query1 = f"CREATE TABLE IF NOT EXISTS {TRAFFIC_INFO_TABLE} (id INT AUTO_INCREMENT PRIMARY KEY, foreign_id INT, " \
             "priorityLevel INT, eventType VARCHAR(255), " \
             "eventStatus VARCHAR(255), dateStart VARCHAR(255), dateUpdated VARCHAR(255), dateEnd VARCHAR(255), " \
             "startMileMarker VARCHAR(255), endMileMarker VARCHAR(255), route VARCHAR(255), startLongitude DECIMAL(9, 6), startLatitude DECIMAL(9, 6), endLongitude DECIMAL(9, 6) NULL,  endLatitude DECIMAL(9, 6) NULL, geometry VARCHAR(2048)) "

    cursor.execute(query1)

    # Create the table if it doesn't exist
    query2 = f"CREATE TABLE IF NOT EXISTS {LOCATION_DETAILS_TABLE} (id INT AUTO_INCREMENT PRIMARY KEY, traffic_info_id INT," \
             "district VARCHAR(255), county VARCHAR(255), subdistrict VARCHAR(2048), " \
             "city VARCHAR(255), region VARCHAR(255), " \
             "unit VARCHAR(255), FOREIGN KEY (traffic_info_id) REFERENCES TrafficMessages(id))"

    cursor.execute(query2)

    db.commit()

def truncate_value(value, max_length):
    if len(value) > max_length:
        return value[:max_length]
    return value

def convert_value(value, max_length=None):
    if isinstance(value, dict) or isinstance(value, list):
        return json.dumps(value)
    if max_length is not None:
        return truncate_value(str(value), max_length)
    return value


def insert_entry(entry, table_name):
    # Prepare the SQL query
    columns = ", ".join(entry.keys())
    placeholders = ", ".join(["%s"] * len(entry))
    query = f"INSERT INTO {table_name} ({columns}) VALUES ({placeholders})"
    values = tuple(convert_value(value, 255) for value in entry.values())

    # Execute the query
    cursor.execute(query, values)
    db.commit()
    return cursor.lastrowid


# File path of the JSON file
#file_path = "/home/abhijeet/indot_traffic_msgs/old_data_merge/old_car_events.json"
file_path = "/home/abhijeet/indot_traffic_msgs/511_New_Datasource/Current_cars-event-feed.json"

# Create the table
create_table()

try:
    with open(file_path) as file:
        data = json.load(file)
        total_entries = len(data)
        entry_count = 0
        for entry in data:
            entry_count += 1
            print(f"Processing entry {entry_count}/{total_entries}")
            
            traffic_info_extracted_columns = {column: entry[column] for column in required_columns_from_json if column in entry}
            location_data_columns = {column: entry['locationDetails'][column] for column in required_columns_from_json_for_locationTable if column in entry['locationDetails']}
            
            for key, value in location_data_columns.items():
                if isinstance(value, list) and len(value) > 0:
                    location_data_columns[key] = value[0]
            
            traffic_info_extracted_columns.pop('locationDetails', None)
            traffic_info_extracted_columns['foreign_id'] = entry['id']
            traffic_info_extracted_columns.pop('id', None)
            
            if entry['geometry'] is not None:
                start_long, start_lat, end_long, end_lat = process_geometry(traffic_info_extracted_columns['geometry'])
                traffic_info_extracted_columns['startLongitude'] = start_long
                traffic_info_extracted_columns['startLatitude'] = start_lat
                if end_long is not None:
                    traffic_info_extracted_columns['endLongitude'] = end_long
                    traffic_info_extracted_columns['endLatitude'] = end_lat
            
            row_id = insert_entry(traffic_info_extracted_columns, TRAFFIC_INFO_TABLE)
            location_data_columns['traffic_info_id'] = row_id
            insert_entry(location_data_columns, LOCATION_DETAILS_TABLE)
            
            print(f"Completed processing entry {entry_count}/{total_entries}")

except (json.JSONDecodeError, FileNotFoundError):
    # Handle JSON decoding or file not found error
    pass

# Close the database connection
db.close()
