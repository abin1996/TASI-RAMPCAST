import sys
import time
import pika
import json

max_priority = 10
publisher_queue_name = 'traffic_event_priority_queue'
# Read events from the JSON file
with open('../Demo/RSU_Messages.json', 'r') as file:
    events = json.load(file)['RSU_Messages']

# Connect to RabbitMQ
connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()

# Declare the priority queue with 'x-max-priority' argument
channel.queue_declare(queue=publisher_queue_name, arguments={'x-max-priority': max_priority})
counter = 0
# Publish events to the RabbitMQ priority queue
try:
    while True:
        for event in events:
            priority = event.get('priority', 0)  # Default priority is 0 if not specified
            event['publish_time'] = time.time()
            event['counter'] = counter
            message = json.dumps(event)
            channel.basic_publish(exchange='', routing_key=publisher_queue_name, body=message, properties=pika.BasicProperties(priority=priority))
            print(f'Published event: {message} with priority: {priority}')
            counter += 1
        time.sleep(2)
#When user presses CTRL+C, print the number of events published and exit
except KeyboardInterrupt:
    print(f'Published {counter} events to the priority queue')
    # Close the connection to RabbitMQ
    connection.close()
    sys.exit(0)
