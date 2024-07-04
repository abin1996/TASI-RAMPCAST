import pandas as pd
import matplotlib.pyplot as plt

# Load data from 5.8 GHz CSV file
csv_file_5_8GHz = "D:\TASI-RAMPCAST\messaging\messaging\metrics-plot\output_5.8Ghz.csv"
data_5_8GHz = pd.read_csv(csv_file_5_8GHz)

# Load data from 5.9 GHz CSV file
csv_file_5_9GHz = "D:\TASI-RAMPCAST\messaging\messaging\metrics-plot\output_5.9Ghz.csv"
data_5_9GHz = pd.read_csv(csv_file_5_9GHz)

# Filter data for ue_id 1
data_5_8GHz = data_5_8GHz[data_5_8GHz['UE_ID'] == 1]
data_5_9GHz = data_5_9GHz[data_5_9GHz['UE_ID'] == 3]

# Extract relevant columns for both datasets
snr_5_8GHz = data_5_8GHz['SNR']
rsrp_5_8GHz = data_5_8GHz['RSRP']
noise_power_5_8GHz = data_5_8GHz['Noise_Power']
rssi_5_8GHz = data_5_8GHz['RSSI']

snr_5_9GHz = data_5_9GHz['SNR']
rsrp_5_9GHz = data_5_9GHz['RSRP']
noise_power_5_9GHz = data_5_9GHz['Noise_Power']
rssi_5_9GHz = data_5_9GHz['RSSI']

# Create subplots
fig, axs = plt.subplots(2, 2, figsize=(12, 8))
fig.suptitle('Data Analysis Comparison')

# Plot SNR for both frequencies
axs[0, 0].plot(snr_5_8GHz, label='5.8 GHz')
axs[0, 0].plot(snr_5_9GHz, label='5.9 GHz')
axs[0, 0].set_title('SNR Over Time')
axs[0, 0].set_xlabel('Time')
axs[0, 0].set_ylabel('SNR (dB)')
axs[0, 0].legend()

# Plot RSRP for both frequencies
axs[0, 1].plot(rsrp_5_8GHz, label='5.8 GHz')
axs[0, 1].plot(rsrp_5_9GHz, label='5.9 GHz')
axs[0, 1].set_title('RSRP Over Time')
axs[0, 1].set_xlabel('Time')
axs[0, 1].set_ylabel('RSRP (dBm)')
axs[0, 1].legend()

# Plot Noise Power for both frequencies
axs[1, 0].plot(noise_power_5_8GHz, label='5.8 GHz')
axs[1, 0].plot(noise_power_5_9GHz, label='5.9 GHz')
axs[1, 0].set_title('Noise Power Over Time')
axs[1, 0].set_xlabel('Time')
axs[1, 0].set_ylabel('Noise Power (dB)')
axs[1, 0].legend()

# Plot RSSI for both frequencies
axs[1, 1].plot(rssi_5_8GHz, label='5.8 GHz')
axs[1, 1].plot(rssi_5_9GHz, label='5.9 GHz')
axs[1, 1].set_title('RSSI Over Time')
axs[1, 1].set_xlabel('Time')
axs[1, 1].set_ylabel('RSSI')
axs[1, 1].legend()

# Adjust layout and display plots
plt.tight_layout()
plt.subplots_adjust(top=0.9)
plt.show()
