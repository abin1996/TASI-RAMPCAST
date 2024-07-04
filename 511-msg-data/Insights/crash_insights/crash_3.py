import mysql.connector
import datetime
import pytz
import seaborn as sns
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

# Retrieve crash event data from the database
query = f"SELECT distinct(foreign_id),dateStart, eventType, priorityLevel FROM {TRAFFIC_INFO_TABLE} WHERE eventType LIKE 'CRASH%'"
cursor.execute(query)
results = cursor.fetchall()

# Close the database connection
db.close()

# Process the retrieved data and create a dictionary to store frequency of crash events
crash_frequency = {}

# Define the EST timezone
est_tz = pytz.timezone('America/New_York')

for row in results:
    foreign_id, date_start, event_type, priority_level = row

    # Convert dateStart from epoch to a datetime object
    date_start = datetime.datetime.fromtimestamp(int(date_start) / 1000)

    # Convert the datetime to EST timezone
    date_start = date_start.astimezone(est_tz)

    # Group events by hour and day of the week
    hour_of_day = date_start.hour
    day_of_week = date_start.strftime("%A")

    # Create a unique key for each hour and day of the week combination
    key = f"{hour_of_day}_{day_of_week}"

    # Increment the crash frequency for the specific hour and day
    crash_frequency[key] = crash_frequency.get(key, 0) + 1

# Convert the crash frequency data into a matrix format for heatmap
hours_of_day = list(range(24))
days_of_week = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']

heatmap_data = []
for day in days_of_week:
    row_data = [crash_frequency.get(f"{hour}_{day}", 0) for hour in hours_of_day]
    heatmap_data.append(row_data)

# Create the heatmap using Seaborn
plt.figure(figsize=(12, 6))
sns.heatmap(heatmap_data, cmap='YlGnBu', xticklabels=hours_of_day, yticklabels=days_of_week, annot=True, fmt="d")
plt.xlabel('Hour of the Day')
plt.ylabel('Day of the Week')
plt.title('Crash Frequency Heatmap ')
plt.show()
