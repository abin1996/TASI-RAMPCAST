import sys
import time
import pika


queue_names = ['pq-sub1', 'pq-sub2']
max_priority = 10


def get_channel():
    # connect and get channel
    parameters = pika.ConnectionParameters('localhost')
    connection = pika.BlockingConnection(parameters)
    channel = connection.channel()
    channel.exchange_declare(exchange='v2x-exchange', exchange_type='fanout')

    # declare queue with max priority
    channel.queue_declare(
        queue=queue_names[0], arguments={"x-max-priority": max_priority}
    )
    channel.queue_bind(exchange='v2x-exchange', queue=queue_names[0])
    
    channel.queue_declare(
        queue=queue_names[1], arguments={"x-max-priority": max_priority}
    )
    channel.queue_bind(exchange='v2x-exchange', queue=queue_names[1])

    return channel

if __name__ == '__main__':

    # default priority 0
    priority = int(sys.argv[1]) if len(sys.argv) > 1 else 0

    channel = get_channel()
    
    j=0
    # publish with priority
    while True:
        j += 1
        for queue in queue_names:
            
            message = 'message#{index}-with-priority{priority}'.format(
                index=j, priority=priority
            )
            channel.basic_publish(
                properties=pika.BasicProperties(priority=priority),
                exchange='v2x-exchange',
                routing_key=queue,
                body=message
            )
            print("Published:", message)
            time.sleep(0.5)