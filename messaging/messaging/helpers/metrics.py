import pandas as pd
import matplotlib.pyplot as plt

class MetricsHelper():
        
    def calculate_latency(self, received_time, sent_time):
        return received_time - sent_time

    def calculate_packet_delivery_ratio(self, received_packets, sent_packets):
        return (received_packets/sent_packets) * 100

    def calculate_packet_inter_reception(self,prev_packet_reception_time, current_packet_reception_time):
        return  current_packet_reception_time - prev_packet_reception_time
    
    def plot_message_stats(self, received_message_stats, filename, timestamp=False):
        if timestamp:
            metrics_df = pd.DataFrame(received_message_stats,columns=["Timestamp","Packet Number","PRR (%)","Latency (s)","PIR (s)","S_Lat","S_Lon","S_Timestamp","S_Alt","S_Speed","R_Lat","R_Lon","R_Timestamp","R_Alt","R_Speed"])
            # Create three separate plots
            fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(8, 10))

            # Plot the data on each axis using df.plot
            metrics_df.plot(x=metrics_df.columns[1], y=metrics_df.columns[2], ax=ax1, color='blue')
            metrics_df.plot(x=metrics_df.columns[1], y=metrics_df.columns[3], ax=ax2, color='green')
            metrics_df.plot(x=metrics_df.columns[1], y=metrics_df.columns[4], ax=ax3, color='red')

        else:
            metrics_df = pd.DataFrame(received_message_stats,columns=["Num of pub packets","PRR (%)","Latency (s)","PIR (s)"])
            # Create three separate plots
            fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(8, 10))

            # Plot the data on each axis using df.plot
            metrics_df.plot(x=metrics_df.columns[0], y=metrics_df.columns[1], ax=ax1, color='blue')
            metrics_df.plot(x=metrics_df.columns[0], y=metrics_df.columns[2], ax=ax2, color='green')
            metrics_df.plot(x=metrics_df.columns[0], y=metrics_df.columns[3], ax=ax3, color='red')

        # Customize the plots (e.g., labels, titles, etc.)
        ax1.set_ylabel('Packet Reception Ratio(%)')
        ax2.set_ylabel('Latency(s)')
        ax3.set_ylabel('Packet Inter reception Ratio(s)')
        ax3.set_xlabel('X')

        # Show the plots
        plt.tight_layout()
        plt.savefig(filename)
        # plt.show()
