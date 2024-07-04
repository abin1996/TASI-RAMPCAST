import time
import pika

queue_name = 'pq-sub2'
max_priority = 10


def get_channel():
    # connect and get channel
    parameters = pika.ConnectionParameters('localhost')
    connection = pika.BlockingConnection(parameters)
    channel = connection.channel()
    channel.exchange_declare(exchange='v2x-exchange', exchange_type='fanout')

    # declare queue with max priority
    channel.queue_declare(
        queue=queue_name, arguments={"x-max-priority": max_priority}
    )
    channel.queue_bind(exchange='v2x-exchange', queue=queue_name)
    return channel
if __name__ == '__main__':

    channel = get_channel()
    queue_name = queue_name

    # get messages
    while True:
        method_frame, header_frame, body = channel.basic_get(queue_name)
        if method_frame:
            channel.basic_ack(method_frame.delivery_tag)
            print("Received:", str(body))
        time.sleep(0.5)