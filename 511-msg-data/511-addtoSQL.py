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
required_columns_from_json = ['id','eventType','priorityLevel','eventStatus','dateStart','dateUpdated','dateEnd','startMileMarker','endMileMarker','route','geometry','locationDetails']
required_columns_from_json_for_locationTable = ['district','county','subdistrict','city','region']
# Create a cursor object to interact with the database
cursor = db.cursor()

def extract_coordinates(geometry):
    coordinates = re.findall(r"-?\d+\.\d+", geometry)
    return [round(float(coord),6) for coord in coordinates]

def process_geometry(geometry):
    if geometry.startswith("POINT"):
        # Point Geometry
        coordinates = extract_coordinates(geometry)
        start_long, start_lat = coordinates
        end_long, start_lat = None, None
    elif geometry.startswith("MULTILINESTRING"):
        # Multiline String Geometry
        coordinates = extract_coordinates(geometry)
        start_long, start_lat = coordinates[0], coordinates[1]
        end_long, start_lat = coordinates[-2], coordinates[-1]
    else:
        # Invalid geometry type
        start_lat, start_long, end_lat, end_long = None, None, None, None
    
    return start_lat, start_long, end_lat, end_long

def create_table():
    
    # Create the table if it doesn't exist
    query1 = f"CREATE TABLE IF NOT EXISTS {TRAFFIC_INFO_TABLE} (id INT AUTO_INCREMENT PRIMARY KEY, foreign_id INT, " \
            "priorityLevel INT, eventType VARCHAR(255), " \
            "eventStatus VARCHAR(255), dateStart VARCHAR(255), dateUpdated VARCHAR(255), dateEnd VARCHAR(255), " \
            "startMileMarker VARCHAR(255), endMileMarker VARCHAR(255), route VARCHAR(255), startLatitude DECIMAL(9, 6), startLongitude DECIMAL(9, 6), endLatitude DECIMAL(9, 6) NULL,  endLongitude DECIMAL(9, 6) NULL, geometry VARCHAR(2048)) " 
    
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
    values = tuple(convert_value(value,255) for value in entry.values())

    # Execute the query
    cursor.execute(query, values)
    db.commit()
    return cursor.lastrowid

# Folder path containing JSON files
folder_path = "/home/abhijeet/indot_traffic_msgs/511_Data"  # Replace with your folder path

# Create the table
create_table()

file_count = 0
total_files = 0

# Iterate over files in the folder
for filename in os.listdir(folder_path):
    if filename.endswith("cars-event-feed.json"):
        total_files += 1
ids = []
# Iterate over files in the folder
for filename in os.listdir(folder_path):
    if filename.endswith("cars-event-feed.json"):
        file_count += 1
        file_path = os.path.join(folder_path, filename)
        print(f"Processing file {file_count}/{total_files}: {filename}")

        try:
            with open(file_path) as file:
                data = json.load(file)
                for entry in data:
                    # insert_entry(entry)
                    #Call the function to extract the required columns from the JSON file and insert into the database table traffic_info_table_name and then location_table_name
                    traffic_info_extracted_columns = {column: entry[column] for column in required_columns_from_json if column in entry}
                    #insert all columns into the traffic_info_table_name except the locationDetails column
                    location_data_columns = {column: entry['locationDetails'][column] for column in required_columns_from_json_for_locationTable if column in entry['locationDetails']}
                    #if the columns in locationDetails consists of a list, then take the first element of the list as the value for the column
                    for key, value in location_data_columns.items():
                        if isinstance(value, list) and len(value) > 0:
                            location_data_columns[key] = value[0]
                    traffic_info_extracted_columns.pop('locationDetails', None)
                    traffic_info_extracted_columns['foreign_id'] = entry['id']
                    traffic_info_extracted_columns.pop('id', None)
                    #extract the coordinates from the geometry column if it not empty and insert into the traffic_info_table_name
                    if entry['geometry'] != None:
                        start_lat, start_long, end_lat, end_long = process_geometry(traffic_info_extracted_columns['geometry'])
                        traffic_info_extracted_columns['startLatitude'] = start_lat
                        traffic_info_extracted_columns['startLongitude'] = start_long
                        if end_lat != None:
                            traffic_info_extracted_columns['endLatitude'] = end_lat
                            traffic_info_extracted_columns['endLongitude'] = end_long
                    #insert traffic_info_extracted_columns into the traffic_info_table_name
                    row_id = insert_entry(traffic_info_extracted_columns, TRAFFIC_INFO_TABLE)
                    #insert the locationDetails column into the location_table_name after inserting the row_id from the traffic_info_table_name
                    location_data_columns['traffic_info_id'] = row_id
                    insert_entry(location_data_columns, LOCATION_DETAILS_TABLE)
                    ids.append(entry['id'])
        except (json.JSONDecodeError, FileNotFoundError):
            # Handle JSON decoding or file not found error
            pass

        print(f"Completed processing file {file_count}/{total_files}: {filename}\n")
unique_ids = set(ids)
print(len(unique_ids))
print(len(ids))

# Close the database connection
db.close()
