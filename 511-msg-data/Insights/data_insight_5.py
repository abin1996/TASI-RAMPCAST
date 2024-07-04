import mysql.connector
import datetime
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
query = f"SELECT distinct(foreign_id), dateStart FROM {TRAFFIC_INFO_TABLE} WHERE dateEnd != 'None' AND eventType LIKE 'CRASH%'"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
event_dates = [datetime.datetime.fromtimestamp(int(row[1]) / 1000) for row in results]
weekdays = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']
event_counts = [0] * 7

# Count the events by day of the week
for event_date in event_dates:
    weekday_index = event_date.weekday()
    event_counts[weekday_index] += 1

# Plotting the event distribution by day of the week
plt.bar(weekdays, event_counts)
plt.xlabel("Day of the Week")
plt.ylabel("Event Count")
plt.title("Event Distribution by Day of the Week")
plt.show()
