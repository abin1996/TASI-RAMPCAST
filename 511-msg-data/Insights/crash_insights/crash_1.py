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
query = f"SELECT foreign_id, eventType FROM TrafficMessages WHERE eventType IN ('CRASH PD', 'CRASH PI', 'CRASH FATAL') GROUP BY foreign_id, eventType"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Count the frequency of each eventType for distinct foreign_id
event_counts = {
    'CRASH PD': 0,
    'CRASH PI': 0,
    'CRASH FATAL': 0
}

for row in results:
    _, event_type = row
    event_counts[event_type] += 1

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
plt.title("Frequency of CRASH PD, CRASH PI, and CRASH FATAL")
plt.show()
