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

# Retrieve event data from the database where dateEnd is not equal to None
query = f"SELECT distinct(foreign_id), eventType, priorityLevel, dateStart FROM {TRAFFIC_INFO_TABLE} WHERE dateEnd != 'None' AND eventType LIKE 'CRASH%'  "
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
event_start_times = {}

for row in results:
    foreign_id, event_type, priority_level, date_start = row

    date_start = datetime.datetime.fromtimestamp(int(date_start) / 1000)
    start_time = date_start.time()

    # Store start times based on event type and priority level
    event_key = f"{event_type}_{priority_level}"
    if event_key in event_start_times:
        event_start_times[event_key].append(start_time)
    else:
        event_start_times[event_key] = [start_time]

# Generate insights
print("Range of Start Times by Event Type and Priority Level:")
for event_key, start_times in event_start_times.items():
    event_type, priority_level = event_key.split("_")
    min_start_time = min(start_times)
    max_start_time = max(start_times)
    print(f"Event Type: {event_type}, Priority Level: {priority_level}")
    print(f"Range of Start Times: {min_start_time.strftime('%I:%M %p')} - {max_start_time.strftime('%I:%M %p')}")
    print()

# Plotting the range of start times by event type and priority level
x = [[min(start_time.hour for start_time in start_times), max(start_time.hour for start_time in start_times)] for start_times in event_start_times.values()]
y = [event_key for event_key in event_start_times.keys()]

fig, ax = plt.subplots(figsize=(10, 6))
for i, (start, end) in enumerate(x):
    ax.plot([start, end], [i, i], color='blue')
    ax.text(start, i, f"{datetime.datetime.strptime(str(start), '%H').strftime('%I:%M %p')}", ha='right', va='center')
    ax.text(end, i, f"{datetime.datetime.strptime(str(end), '%H').strftime('%I:%M %p')}", ha='left', va='center')
ax.set_yticks(range(len(y)))
ax.set_yticklabels(y)
plt.ylabel("Event Type and Priority Level")
plt.xlabel("Time of Day")
plt.title("Range of Start Times by Event Type and Priority Level")
plt.tight_layout()
plt.show()
