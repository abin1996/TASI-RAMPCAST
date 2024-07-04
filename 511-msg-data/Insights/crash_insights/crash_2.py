import mysql.connector
import matplotlib.pyplot as plt
import datetime

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

# Retrieve event data from the database where dateEnd is not equal to None and eventType is "CRASH PD", "CRASH PI", or "CRASH FATAL"
query = f"SELECT dateStart, dateEnd FROM {TRAFFIC_INFO_TABLE} WHERE dateEnd != 'None' AND eventType IN ('CRASH PD', 'CRASH PI', 'CRASH FATAL')"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Calculate event durations in hours
event_durations = [(datetime.datetime.fromtimestamp(int(date_end) / 1000) - datetime.datetime.fromtimestamp(int(date_start) / 1000)).total_seconds() / 3600 for date_start, date_end in results]

# Plot event duration distribution using a histogram
plt.hist(event_durations, bins=20, edgecolor='black')
plt.xlabel("Event Duration (hours)")
plt.ylabel("Frequency")
plt.title("Distribution of Event Durations for CRASH Events")
plt.grid(True)
plt.show()
