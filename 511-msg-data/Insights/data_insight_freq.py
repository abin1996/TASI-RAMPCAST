import mysql.connector
import matplotlib.pyplot as plt

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
query = "SELECT eventType, COUNT(DISTINCT foreign_id) AS event_count FROM TrafficMessages GROUP BY eventType ORDER BY event_count DESC LIMIT 5"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Extract event types and their respective frequencies from the results
event_counts = {}
for row in results:
    event_type, event_count = row
    event_counts[event_type] = event_count

# Plot the graph
event_types = list(event_counts.keys())
event_frequencies = list(event_counts.values())

fig, ax = plt.subplots()
bars = ax.bar(event_types, event_frequencies)

# Add the count as text above each bar
for bar in bars:
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width() / 2, height, height, ha='center', va='bottom')

plt.xlabel("Event Type")
plt.ylabel("No of Events")
plt.title("Top 5 Event Types by Frequency")
plt.show()
