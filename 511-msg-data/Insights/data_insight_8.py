import mysql.connector
import matplotlib.pyplot as plt
import numpy as np

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
query = f"SELECT eventType, priorityLevel, COUNT(DISTINCT foreign_id) FROM {TRAFFIC_INFO_TABLE} GROUP BY eventType, priorityLevel"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data
priority_level_event_type_counts = {}

for row in results:
    event_type, priority_level, count = row
    priority_level_event_type_counts[priority_level] = priority_level_event_type_counts.get(priority_level, {})
    priority_level_event_type_counts[priority_level][event_type] = count

# Get the total count of each event type across all priority levels
event_type_counts_total = {}
for priority_level_counts in priority_level_event_type_counts.values():
    for event_type, count in priority_level_counts.items():
        event_type_counts_total[event_type] = event_type_counts_total.get(event_type, 0) + count

# Sort event types by their counts in descending order
sorted_event_types = sorted(event_type_counts_total, key=lambda x: event_type_counts_total[x], reverse=True)

# Select the top 5 event types
top_5_event_types = sorted_event_types[:8]

# Separate priority levels and event types
priority_levels = list(priority_level_event_type_counts.keys())
priority_levels.sort()
event_types = list(set([event_type for event_types in priority_level_event_type_counts.values() for event_type in event_types.keys()]))

# Filter only the top 5 event types from the list of all event types
event_types = [event_type for event_type in event_types if event_type in top_5_event_types]

# Create a stacked bar plot for the top 5 event types
fig, ax = plt.subplots(figsize=(6, 6))

bottom = np.zeros(len(event_types))

for i, priority_level in enumerate(priority_levels):
    event_type_counts = [priority_level_event_type_counts[priority_level].get(event_type, 0) for event_type in event_types]
    ax.bar(event_types, event_type_counts, label=priority_level, bottom=bottom)
    bottom += np.array(event_type_counts)

ax.set_xlabel("Event Type")
ax.set_ylabel("No of Events")
ax.set_title("Top 8 Event Types Distribution by Priority Level")
ax.legend()
plt.xticks(rotation=90)
plt.tight_layout()
plt.show()
