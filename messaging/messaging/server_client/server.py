import socket
import json
import time

class Server:

    def __init__(self, host='10.0.2.11', port=12345, target_ip='10.0.2.12',target_port=12346,filename="server.csv"):
        self.host = host
        self.port = port
        self.target_ip = target_ip
        self.target_port = int(target_port)
        self.socket = None
        self.filename = filename

    def start(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind((self.host, self.port))
        print(f"UDP Server listening on {self.host}:{self.port}")

        while True:
            data, addr = self.socket.recvfrom(2048)
            current_time = time.time()
            try:
                received_message = json.loads(data.decode())
            except json.JSONDecodeError:
                print("Invalid JSON message received.")

            received_message['received_at'] = current_time
            # response = {"received_at": current_time, "message": received_message}
            print(received_message)
            response_json = json.dumps(received_message).encode()
            # time.sleep(1)
            rc = self.socket.sendto(response_json, (self.target_ip, self.target_port))
            print("sent "+ str(rc))
        self.stop()

    def stop(self):
        if self.socket:
            self.socket.close()

if __name__ == "__main__":
    server = Server()
    server.start()
    server.stop()
    print("Server stopped")
