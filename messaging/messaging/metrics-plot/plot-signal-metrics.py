import pandas as pd
import matplotlib.pyplot as plt

# Load data from CSV file
csv_file = "D:\TASI-RAMPCAST\messaging\messaging\metrics-plot\output_5.8Ghz.csv"  # Replace with your CSV file path
data = pd.read_csv(csv_file)

# Extract relevant columns
snr = data['SNR']
rsrp = data['RSRP']
noise_power = data['Noise_Power']
rx_full_secs = data['RX_Full_Secs']
rssi = data['RSSI']

# Create subplots
fig, axs = plt.subplots(2, 2, figsize=(12, 8))
fig.suptitle('Data Analysis')

# Plot SNR
axs[0, 0].plot(snr)
axs[0, 0].set_title('SNR Over Time')
axs[0, 0].set_xlabel('Time')
axs[0, 0].set_ylabel('SNR (dB)')

# Plot RSRP
axs[0, 1].plot(rsrp)
axs[0, 1].set_title('RSRP Over Time')
axs[0, 1].set_xlabel('Time')
axs[0, 1].set_ylabel('RSRP (dBm)')

# Plot Noise Power
axs[1, 0].plot(noise_power)
axs[1, 0].set_title('Noise Power Over Time')
axs[1, 0].set_xlabel('Time')
axs[1, 0].set_ylabel('Noise Power (dB)')

# Plot RX_Full_Secs
axs[1, 1].plot(rssi)
axs[1, 1].set_title('RSSI Over Time')
axs[1, 1].set_xlabel('Time')
axs[1, 1].set_ylabel('RSSI')

# Adjust layout and display plots
plt.tight_layout()
plt.subplots_adjust(top=0.9)
plt.show()
