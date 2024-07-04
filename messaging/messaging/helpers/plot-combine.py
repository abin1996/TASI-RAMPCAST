import pandas as pd
import matplotlib.pyplot as plt

class MetricsHelper():
        
    def calculate_latency(self, received_time, sent_time):
        return received_time - sent_time

    def calculate_packet_delivery_ratio(self, received_packets, sent_packets):
        return (received_packets / sent_packets) * 100

    def calculate_packet_inter_reception(self, prev_packet_reception_time, current_packet_reception_time):
        return current_packet_reception_time - prev_packet_reception_time

    def plot_message_stats(self, xlsx_filenames, filename):
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(8, 10))

        colors = ['blue', 'green', 'red', 'yellow', 'purple', 'orange']
        
        for i, xlsx_filename in enumerate(xlsx_filenames):
            metrics_df = pd.read_csv(xlsx_filename)
            
            label_parts = xlsx_filename.split('/')[-1].split('.')[0].split('_')  # Extract relevant parts from file name
            label = f"{label_parts[1]}_{label_parts[3]}"  # Combine parts to form label
            metrics_df.plot(x=metrics_df.columns[1], y=metrics_df.columns[2], ax=ax1, color=colors[i], label=label)
            metrics_df.plot(x=metrics_df.columns[1], y=metrics_df.columns[3], ax=ax2, color=colors[i], label=label)
            metrics_df.plot(x=metrics_df.columns[1], y=metrics_df.columns[4], ax=ax3, color=colors[i], label=label)

        # Customize the plots (e.g., labels, titles, etc.)
        ax1.set_ylabel('Packet Reception Ratio(%)')
        ax2.set_ylabel('Latency(s)')
        ax3.set_ylabel('Packet Inter reception Ratio(s)')
        ax3.set_xlabel('X')

        # Show the plots
        plt.tight_layout()
        plt.savefig(filename)
        # plt.show()

# Create an instance of the MetricsHelper class
metrics_helper = MetricsHelper()

# Specify the XLSX filenames and the output filename
# xlsx_filenames_5m = [
#     '/home/abhijeet/TASI-RAMPCAST/messaging/data/test_results/DT_5M_900KB_5HZ_500MSG_T3.xlsx',
#     '/home/abhijeet/TASI-RAMPCAST/messaging/data/test_results/DT_5M_900KB_8HZ_500MSG.xlsx',
#     '/home/abhijeet/TASI-RAMPCAST/messaging/data/test_results/DT_5M_900KB_10HZ_500MSG.xlsx'
# ]
# output_filename_5m = '5M_900KB_500MSG_3_PLOT.png'

# # Call the plot_message_stats() method
# metrics_helper.plot_message_stats(xlsx_filenames_5m, output_filename_5m)


xlsx_filenames_1m = [
    '/Users/abinmath/Documents/TASI-RAMPCAST/code/TASI-RAMPCAST/messaging/data/test_results/DT_50M_2700KB_5HZ_500MSG_T1.csv',
    '/Users/abinmath/Documents/TASI-RAMPCAST/code/TASI-RAMPCAST/messaging/data/test_results/DT_50M_2700KB_5HZ_500MSG_T1.csv'
    ]
output_filename_1m = '50M_2700KB_5HZ_500MSG_PLOT.png'

# Call the plot_message_stats() method
metrics_helper.plot_message_stats(xlsx_filenames_1m, output_filename_1m)
