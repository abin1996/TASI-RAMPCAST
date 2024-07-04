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

# Create a cursor object to interact with the database
cursor = db.cursor()

# Retrieve event data from the database
query = f"SELECT foreign_id, eventType, dateStart FROM TrafficMessages WHERE eventType IN ('CRASH PD', 'CRASH PI', 'CRASH FATAL')"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data and create a dictionary to store event counts for each event type at specific times of the day
event_counts = {
    'CRASH PD': [0] * 24,
    'CRASH PI': [0] * 24,
    'CRASH FATAL': [0] * 24
}

for row in results:
    foreign_id, event_type, date_start = row
    date_start = datetime.datetime.fromtimestamp(int(date_start) / 1000)
    hour = date_start.hour
    event_counts[event_type][hour] += 1

# Plot the graph
hours = list(range(24))

fig, ax = plt.subplots()
ax.bar(hours, event_counts['CRASH PD'], label='CRASH PD', alpha=0.7)
ax.bar(hours, event_counts['CRASH PI'], bottom=event_counts['CRASH PD'], label='CRASH PI', alpha=0.7)
ax.bar(hours, event_counts['CRASH FATAL'], bottom=[sum(x) for x in zip(event_counts['CRASH PD'], event_counts['CRASH PI'])], label='CRASH FATAL', alpha=0.7)

ax.set_xlabel("Hour of the Day")
ax.set_ylabel("Events Frequency")
ax.set_title("Frequency Distribution of CRASH PD, CRASH PI, and CRASH FATAL Events by Hour of the Day")
ax.legend()

# Display the values on the chart
for i in range(24):
    total_count = event_counts['CRASH PD'][i] + event_counts['CRASH PI'][i] + event_counts['CRASH FATAL'][i]
    ax.text(i, total_count + 5, str(total_count), ha='center', va='bottom', fontweight='bold', fontsize=8)

plt.xticks(hours)
plt.show()
