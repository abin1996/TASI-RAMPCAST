import mysql.connector
import datetime
import statistics
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
query = f"SELECT distinct(foreign_id), eventType, priorityLevel, dateStart, dateEnd FROM {TRAFFIC_INFO_TABLE} WHERE dateEnd != 'None'"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
event_durations = {}
event_counts = {}

for row in results:
    foreign_id, event_type, priority_level, date_start, date_end = row

    date_start = datetime.datetime.fromtimestamp(int(date_start) / 1000)
    date_end = datetime.datetime.fromtimestamp(int(date_end) / 1000)
    duration = date_end - date_start

    # Store event duration and count based on event type and priority level
    event_key = f"{event_type}_{priority_level}"
    if event_key in event_durations:
        event_durations[event_key].append(duration)
    else:
        event_durations[event_key] = [duration]
        event_counts[event_key] = 0

    event_counts[event_key] += 1

# Generate insights
print("Event Statistics by Event Type and Priority Level:")
for event_key, durations in event_durations.items():
    event_type, priority_level = event_key.split("_")
    avg_duration = sum(duration.total_seconds() / 3600 for duration in durations) / len(durations)
    min_duration = min(duration.total_seconds() / 3600 for duration in durations)
    max_duration = max(duration.total_seconds() / 3600 for duration in durations)
    median_duration = statistics.median(duration.total_seconds() / 3600 for duration in durations)
    mode_duration = statistics.mode(duration.total_seconds() / 3600 for duration in durations)
    event_count = event_counts[event_key]
    print(f"Event Type: {event_type}, Priority Level: {priority_level}")
    print(f"Average Duration (hours): {avg_duration}")
    print(f"Lowest Duration (hours): {min_duration}")
    print(f"Highest Duration (hours): {max_duration}")
    print(f"Median Duration (hours): {median_duration}")
    print(f"Mode Duration (hours): {mode_duration}")
    print(f"Number of Events: {event_count}")
    print()

# Plotting event durations by event type and priority level
x = [event_key for event_key in event_durations.keys()]
y = [sum(duration.total_seconds() / 3600 for duration in durations) / len(durations) for durations in event_durations.values()]

fig, ax = plt.subplots(figsize=(10, 6))
ax.bar(x, y)

# Add the average value as text above each bar
for i, j in enumerate(y):
    ax.text(i, j, f"{j:.2f}", ha='center', va='bottom')

# Calculate the total number of events
total_events = sum(event_counts.values())

# Add total number of events in the top right corner
plt.text(len(x)-0.5, max(y), f"Total Events: {total_events}", ha='right', va='top')

plt.xlabel("Event Type and Priority Level")
plt.ylabel("Average Duration (hours)")
plt.title("Average Event Durations by Event Type and Priority Level")
plt.xticks(rotation=90)
plt.tight_layout()
plt.show()
