
from gpsdclient import GPSDClient
import csv
import time
import threading

class GPSData:
    def __init__(self):
        self.lat = None
        self.lon = None
        self.time = None
        self.alt = None
        self.speed = None
        self.prev_lat = None
        self.prev_lon = None
        self.prev_time = None
        self.prev_alt = None
        self.prev_speed = None
        self.polling_active = False
        self.lock = threading.Lock()

    def get_cached_gps_data(self):
        return self.lat, self.lon, self.time, self.alt, self.speed
    
    def get_current_gps_data(self):
        try:
            with self.lock:
                with GPSDClient(timeout=5) as client:
                    for result in client.dict_stream(convert_datetime=True, filter=["TPV"]):
                        self.prev_lat = self.lat
                        self.prev_lon = self.lon
                        self.prev_time = self.time
                        self.prev_alt = self.alt
                        self.prev_speed = self.speed

                        self.lat = result.get("lat", "n/a")
                        self.lon = result.get("lon", "n/a")
                        self.time = result.get("time", "n/a")
                        self.alt = result.get("alt", "n/a")
                        self.speed = result.get("speed", "n/a")

                        if self.lat == "n/a":
                            self.lat = self.prev_lat
                        if self.lon == "n/a":
                            self.lon = self.prev_lon

                        return self.lat, self.lon, self.time, self.alt, self.speed

        except ConnectionRefusedError:
            print("Connection to GPSD refused.")
            return self.prev_lat, self.prev_lon, self.prev_time, self.prev_alt, self.prev_speed

    def start_polling(self, output_file):
        try:
            with GPSDClient() as client, open(output_file, 'w+', newline='') as csvfile:
                fieldnames = ['Local_Time','Time', 'Latitude', 'Longitude', 'Altitude', 'Speed']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                writer.writeheader()

                while self.polling_active:
                    lat, lon, timestamp, alt, speed = self.get_current_gps_data()

                    if lat is not None and lon is not None:
                        print("Latitude: %s" % lat)
                        print("Longitude: %s" % lon)
                        print("Time: %s" % timestamp)
                        print("Altitude: %s" % alt)
                        print("Speed: %s" % speed)

                        writer.writerow({
                            'Local_Time': str(time.time()),
                            'Time': timestamp,
                            'Latitude': lat,
                            'Longitude': lon,
                            'Altitude': alt,
                            'Speed': speed
                        })

                    time.sleep(0.5)  # Poll every second

        except ConnectionRefusedError:
            print("Connection to GPSD refused.")

    def start_gps_polling(self, output_file="gps_data.csv"):
        self.polling_active = True
        polling_thread = threading.Thread(target=self.start_polling, args=(output_file,))
        polling_thread.start()

    def stop_gps_polling(self):
        self.polling_active = False
        print("Stopping gps collection..")

   

# def main():
#     gps_data = GPSData()
    
#     # Start polling in a separate thread and dump data to "gps_data.csv"
#     gps_data.start_gps_polling(output_file="gps_data.csv")
    
#     time.sleep(20)
    
#     # Stop GPS polling
#     gps_data.stop_gps_polling()

# if __name__ == "__main__":
#     main()
