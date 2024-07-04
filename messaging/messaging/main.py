import sys
import argparse
import time
from socket_messaging.subscriber import UDPListener
from socket_messaging.publisher import UDPPublisher
from server_client.server import Server
from server_client.client import Client
from message_broker.publisher import MQTTPublisher
from message_broker.subscriber import MQTTListener
from helpers.file_helper import FileHelper
import os
from helpers.gps import GPSData

def parse_arguments():
    parser = argparse.ArgumentParser(description='Publish and subscribe messages over socket or broker.')
    
    parser.add_argument('-m', '--mode', choices=['socket', 'broker' , 'sc'], default='socket', help='Socket or broker mode')
    parser.add_argument('-p', '--pub', choices=['pub', 'sub'], default='pub', help='Publish or subscribe mode')
    parser.add_argument('-i', '--ip', default='localhost', help='Broker IP address')
    parser.add_argument('-port', '--port', type=int, default=1883, help='Broker port number')
    parser.add_argument('-t', '--total', type=int, default=100, help='Total number of messages')
    parser.add_argument('-inf', '--input_file', default='/Demo/RSU_Messages.json', help='Message data file name (publisher)')
    default_outf = 'output-KPI-{}'.format(time.strftime('%Y%m%d-%H%M%S'))
    parser.add_argument('-outf', '--output_file', default=default_outf, help='Output message data file name (subscriber)')
    parser.add_argument('-f', '--frequency', type=float, default=1.0, help='Message frequency')
    parser.add_argument('-s', '--message_size', type=int, default=512, help='Message size')
    parser.add_argument('-d', '--main_directory', default='Default-Test', help='Main directory for test cases')
    parser.add_argument('-mf', '--metric_file', default='../sidelink/output_new_metrics.csv', help='Metric file location')
    parser.add_argument('-tpo', '--target_port', default=12345, help='Target Port')
    parser.add_argument('-tip', '--target_ip', default='10.0.2.11', help='Target IP')

    return parser.parse_args()

def run_pub_sub(args):
    
    gps_data = GPSData()
    if args.mode == 'socket':
        if args.pub == 'pub':
            message_data_to_send = FileHelper.load_message_to_send(args.input_file)
            publisher = UDPPublisher(message_data_to_send, args.ip, args.port, args.frequency, args.total, args.message_size)
            publisher.publish()
        else:
            out_dir = os.path.join(args.main_directory, 'testcase-{}'.format(time.strftime('%Y%m%d-%H%M%S')))
            os.makedirs(out_dir, exist_ok=True)
            output_file_path = os.path.join(out_dir, args.output_file)
            subscriber = UDPListener(args.ip, args.port, output_file_path, args.total)
            gps_file = os.path.join(out_dir, "gps_data.csv")
            gps_data.start_gps_polling(output_file=gps_file)
            subscriber.listen_for_messages()
            subscriber.move_metric_file(args.metric_file, out_dir)
            gps_data.stop_gps_polling()
           

    elif args.mode == 'broker':
        if args.pub == 'pub':
            message_data_to_send = FileHelper.load_message_to_send(args.input_file)
            publisher = MQTTPublisher(message_data_to_send, args.ip, args.port, 60, args.frequency, args.total)
            publisher.connect()
            publisher.publish()
        else:
            out_dir = os.path.join(args.main_directory, 'testcase-{}'.format(time.strftime('%Y%m%d-%H%M%S')))
            os.makedirs(out_dir, exist_ok=True)
            output_file_path = os.path.join(out_dir, args.output_file)
            gps_data.start_gps_polling(output_file=gps_file)
            subscriber = MQTTListener(args.ip, args.port, 60, output_file_path, args.total)
            subscriber.connect()
            gps_data.stop_gps_polling()
            subscriber.move_metric_file(args.metric_file, out_dir)

    elif args.mode == 'sc':
        if args.pub == 'pub':
            out_dir = os.path.join(args.main_directory, 'testcase-{}'.format(time.strftime('%Y%m%d-%H%M%S')))
            gps_file = os.path.join(out_dir, "gps_data.csv")
            os.makedirs(out_dir, exist_ok=True)
            # gps_data.start_gps_polling(output_file=gps_file)
            output_file_path = os.path.join(out_dir, args.output_file)
            message_data_to_send = FileHelper.load_message_to_send(args.input_file)
            time.sleep(1)
            print(args.target_port)
            publisher = Client(message_data_to_send, args.ip, args.port,args.target_ip, args.target_port, args.frequency, args.total,output_file_path,gps_data)
            publisher.connect()
            publisher.start_pinging()
            publisher.move_metric_file(args.metric_file, out_dir)
            # gps_data.stop_gps_polling()
        else:
            subscriber = Server(args.ip, args.port, args.target_ip, args.target_port)
            subscriber.start()            

if __name__ == "__main__":
    args = parse_arguments()
    run_pub_sub(args)
