import mysql.connector
import datetime
import matplotlib.pyplot as plt
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

# Retrieve event data from the database
query = f"SELECT  distinct(foreign_id), dateStart FROM {TRAFFIC_INFO_TABLE} WHERE eventType LIKE 'CRASH%'"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
event_counts = {}

for row in results:
    date_start = datetime.datetime.fromtimestamp(int(row[1]) / 1000)
    est = timezone('US/Eastern')
    date_start_est = date_start.astimezone(est)
    hour = date_start_est.hour

    # Store event counts based on hour
    if hour in event_counts:
        event_counts[hour] += 1
    else:
        event_counts[hour] = 1

# Sort the event counts by hour
sorted_event_counts = sorted(event_counts.items())

# Extract the sorted hours and counts
hours = [hour for hour, _ in sorted_event_counts]
counts = [count for _, count in sorted_event_counts]

# Plotting the distribution of events over time
fig, ax = plt.subplots(figsize=(10, 6))

# Plot the line chart
ax.plot(hours, counts, marker='o', linestyle='-', color='blue')

# Add labels and title
plt.xlabel("Hour of the Day (EST)")
plt.ylabel("Event Count")
plt.title("Distribution of Events Over Time (EST)")
plt.xticks(range(24))
plt.tight_layout()
plt.show()
