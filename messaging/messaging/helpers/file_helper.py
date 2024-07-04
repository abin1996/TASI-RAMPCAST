import json
import pandas as pd
import os

class FileHelper():
    def load_message_to_send(filename):
        with open(filename, "r") as file:
            data = json.load(file)
            rsu_messages = data.get('sign', [])
        return rsu_messages

    def save_message_stats(self, received_message_stats, filename, timestamp=False):
        if timestamp:
            metrics_df = pd.DataFrame(received_message_stats, columns=["Timestamp","Packet Number", "PRR (%)", "Latency (ms)", "PIR (ms)","S_Lat","S_Lon","S_Timestamp","S_Alt","S_Speed","R_Lat","R_Lon","R_Timestamp","R_Alt","R_Speed"])
        else:
            metrics_df = pd.DataFrame(received_message_stats, columns=["Packet Number", "PRR (%)", "Latency (s)", "PIR (s)"])
        
            avg_stats = self.display_avg_stats(metrics_df)
            
            avg_stats_txt_path = os.path.join(directory, 'avg_stats.txt')
            self.save_avg_stats_to_txt(avg_stats, avg_stats_txt_path)
        directory = os.path.dirname(filename)
        filename += '.csv'
        stat_csv_file_name = os.path.basename(filename)
        metrics_df.to_csv(os.path.join(directory, stat_csv_file_name))
    
    def display_avg_stats(self, metrics_df):
        avg_stats = metrics_df.mean()
        print("Stats", avg_stats)
        return avg_stats

    def save_avg_stats_to_txt(self, avg_stats, filename):
        with open(filename, 'w') as txt_file:
            txt_file.write(str(avg_stats))
        print(f"Avg stats saved to: {filename}")