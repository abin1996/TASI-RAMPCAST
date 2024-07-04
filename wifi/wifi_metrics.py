import subprocess
import time
import argparse
from datetime import datetime

parser = argparse.ArgumentParser(description='Display WLAN signal strength.')
parser.add_argument(dest='interface', nargs='?', default='wlp0s20f3',
                    help='wlan interface (default: wlp0s20f3)')
args = parser.parse_args()

print ('\n---Press CTRL+Z or CTRL+C to stop.---\n')

output_file = None

try:
    while True:
        if output_file is None:
            timestamp = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
            output_file_name = f"wifi_metrics_{timestamp}.log"
            output_file = open(output_file_name, "w")
        
        cmd = subprocess.Popen(['iwconfig', args.interface], stdout=subprocess.PIPE)
        output_lines = []
        for line in cmd.stdout:
            line = line.decode('utf-8')  # Decode bytes-like object to string
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')  # Get current timestamp
            output_line = timestamp + ' ' + line.rstrip('\n')  # Add timestamp to the line
            output_lines.append(output_line + '\n')  # Append newline character
            if 'Link Quality' in line:
                print(output_line.lstrip(' '), end='')  # Print the line with timestamp
            elif 'Not-Associated' in line:
                print(timestamp + ' No signal')
        output_content = ''.join(output_lines)
        print(output_content)
        output_file.write(output_content)
        time.sleep(1)

except KeyboardInterrupt:
    print("Exiting...")
    if output_file:
        output_file.close()
