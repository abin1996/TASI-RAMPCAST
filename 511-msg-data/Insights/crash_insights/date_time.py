import mysql.connector

# Connect to the MySQL database
db = mysql.connector.connect(
    host='localhost',
    user='root',
    password='root1234',
    database='TrafficInfoDB'
)

# Create a cursor object to interact with the database
cursor = db.cursor()

def convert_to_epoch(date_str):
    try:
        if date_str is not None:
            return int(date_str)
        return None
    except ValueError:
        return None

# Execute the query to get dateStart and dateEnd
query = "SELECT dateStart, dateEnd FROM TrafficMessages WHERE dateStart IS NOT NULL AND dateEnd IS NOT NULL;"
cursor.execute(query)
result = cursor.fetchall()

# Find the minimum and maximum dateStart and dateEnd (as epoch values)
min_date_start = None
max_date_start = None
min_date_end = None
max_date_end = None

for row in result:
    date_start, date_end = row
    date_start_epoch = convert_to_epoch(date_start)
    date_end_epoch = convert_to_epoch(date_end)

    if date_start_epoch is not None:
        if min_date_start is None or date_start_epoch < min_date_start:
            min_date_start = date_start_epoch

        if max_date_start is None or date_start_epoch > max_date_start:
            max_date_start = date_start_epoch

    if date_end_epoch is not None:
        if min_date_end is None or date_end_epoch < min_date_end:
            min_date_end = date_end_epoch

        if max_date_end is None or date_end_epoch > max_date_end:
            max_date_end = date_end_epoch

print("Minimum dateStart (epoch):", min_date_start)
print("Maximum dateStart (epoch):", max_date_start)
print("Minimum dateEnd (epoch):", min_date_end)
print("Maximum dateEnd (epoch):", max_date_end)

# Close the database connection
db.close()
