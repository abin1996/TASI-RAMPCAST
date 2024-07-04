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
query = f"SELECT dateStart FROM {TRAFFIC_INFO_TABLE} WHERE eventType LIKE 'CRASH%'"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
start_times = []

for row in results:
    date_start = datetime.datetime.fromtimestamp(int(row[0]) / 1000)
    start_times.append(date_start)

# Convert start times to Eastern Standard Time (EST)
est = timezone('US/Eastern')
start_times_est = [dt.astimezone(est) for dt in start_times]

# Plotting the temporal distribution of events
fig, ax = plt.subplots(figsize=(10, 6))

# Define the time intervals for the histogram
bin_width = datetime.timedelta(hours=1)
start_time = min(start_times_est)
end_time = max(start_times_est) + bin_width
bins = [start_time]
while bins[-1] < end_time:
    bins.append(bins[-1] + bin_width)

# Plot the histogram
ax.hist(start_times_est, bins=bins, edgecolor='black')

# Set the x-axis labels to show the hour and minute in EST
ax.xaxis.set_major_formatter(plt.FixedFormatter([dt.strftime('%H:%M') for dt in bins]))

# Set the title and labels
plt.title("Temporal Distribution of Events (EST)")
plt.xlabel("Time of Day (EST)")
plt.ylabel("Number of Events")

plt.tight_layout()
plt.show()
