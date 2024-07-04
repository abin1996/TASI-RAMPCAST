import mysql.connector
import matplotlib.pyplot as plt

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

# Retrieve event data from the database
query = f"SELECT startLatitude, startLongitude FROM {TRAFFIC_INFO_TABLE} WHERE dateEnd IS NOT NULL AND startLatitude IS NOT NULL AND startLongitude IS NOT NULL"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
latitudes = []
longitudes = []

for row in results:
    latitude = row[0]
    longitude = row[1]

    if latitude is not None and longitude is not None:
        latitudes.append(float(latitude))
        longitudes.append(float(longitude))

# Plotting the spatial distribution of events
plt.scatter(longitudes, latitudes, s=5, alpha=0.5)
plt.xlabel("Longitude")
plt.ylabel("Latitude")
plt.title("Spatial Distribution of Events")
plt.show()
