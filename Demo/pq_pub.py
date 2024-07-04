import pika
import paho.mqtt.client as mqtt
import json
import sys
# Define the MQTT broker settings
mqtt_broker = 'localhost'
# mqtt_broker = '10.0.2.11'
mqtt_port = 1883
mqtt_topic = 'V2X-Traveller-Information'
publisher_queue_name = 'traffic_event_priority_queue'
# Function to publish events to the MQTT topic
def publish_event_to_subscribers(event):
    message = json.dumps(event)
    client.publish(mqtt_topic, message)
    print(f'Published event to MQTT topic: {message}')

# Define the callback function when a message is received from RabbitMQ
def callback(ch, method, properties, body):
    event = json.loads(body)
    print(f'Received event: {event}')
    publish_event_to_subscribers(event)

def get_traffic_events_channel():
    # Connect to RabbitMQ
    connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
    channel = connection.channel()
    # Declare the priority queue with 'x-max-priority' argument
    channel.queue_declare(queue=publisher_queue_name, arguments={'x-max-priority': 10})
    # Set up the consumer to consume from the queue
    channel.basic_consume(queue=publisher_queue_name, on_message_callback=callback, auto_ack=True)
    return channel, connection

try:
# Initialize MQTT client
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.username_pw_set('mqtt-test1', 'tasi')
    client.connect(mqtt_broker, mqtt_port, 60)
    # Start the MQTT loop (required for handling MQTT communication)
    client.loop_start()

    channel,connection = get_traffic_events_channel()
    print('Waiting for messages. To exit press CTRL+C')

    channel.start_consuming()
#When user presses CTRL+C, print the number of events published and exit
except KeyboardInterrupt:
    # Close the connection to RabbitMQ
    connection.close()
    sys.exit(0)