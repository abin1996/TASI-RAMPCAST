import requests
import json
import time
import tkinter as tk
from tkinter import messagebox

RSU_COORDINATES = [39.782159, -86.167573]
JSON_FILE = "./Demo/RSU_Messages.json"
MAX_MESSAGE_LENGTH = 20
WINDOW_WIDTH = 540
WINDOW_HEIGHT = 240

def break_message(message):
    broken_message = []
    words = message.split()
    current_line = ""
    for word in words:
        if len(current_line) + len(word) + 1 <= MAX_MESSAGE_LENGTH:
            current_line += word + " "
        else:
            broken_message.append(current_line.strip())
            current_line = word + " "
    broken_message.append(current_line.strip())
    return broken_message

def convert_to_new_format(message_text, message_type, priority, duration):
    timestamp = int(time.time())
    offtime = timestamp + duration

    broken_messages = break_message(message_text)
    message1 = broken_messages[0] if len(broken_messages) >= 1 else ""
    message2 = broken_messages[1] if len(broken_messages) >= 2 else ""
    message3 = broken_messages[2] if len(broken_messages) >= 3 else ""

    new_feature = {
        "geometry": {
            "type": "Point",
            "coordinates": RSU_COORDINATES
        },
        "title": "Custom Message",
        "ontime": time.strftime('%Y-%m-%dT%H:%M:%S-04:00', time.localtime(timestamp)),
        "offtime": time.strftime('%Y-%m-%dT%H:%M:%S-04:00', time.localtime(offtime)),
        "message1": message1,
        "message2": message2,
        "message3": message3,
        "priority": priority,
        "message_type": message_type,
        "device_nbr": 0,  # Assuming 0 for custom messages
        "device_label": "Custom",
        "timestamp": timestamp
    }
    return new_feature

def save_new_entry():
    message_text = entry_message.get()
    message_type = var_message_type.get()
    priority = var_priority.get()
    duration = int(entry_duration.get()) * 60  # Convert to seconds

    new_entry = convert_to_new_format(message_text, message_type, priority, duration)

    try:
        with open(JSON_FILE, "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        data = {"RSU_Coordinates": RSU_COORDINATES, "RSU_Messages": []}

    data["RSU_Messages"].append(new_entry)

    with open(JSON_FILE, "w") as f:
        json.dump(data, f, indent=2)

    messagebox.showinfo("Success", "New entry added to RSU_Messages.json")
    window.destroy()

def create_gui():
    global window, entry_message, var_message_type, var_priority, entry_duration

    window = tk.Tk()
    window.title("Add New RSU Message")
    
    # Set window dimensions and position
    window_width = WINDOW_WIDTH
    window_height = WINDOW_HEIGHT
    screen_width = window.winfo_screenwidth()
    screen_height = window.winfo_screenheight()
    x = (screen_width - window_width) // 2
    y = (screen_height - window_height) // 2
    window.geometry(f"{window_width}x{window_height}+{x}+{y}")

    label_message = tk.Label(window, text="Message Text (max 60 characters):")
    label_message.grid(row=0, column=0, padx=10, pady=5)

    entry_message = tk.Entry(window, width=30)
    entry_message.grid(row=0, column=1, padx=10, pady=5)

    label_message_type = tk.Label(window, text="Message Type:")
    label_message_type.grid(row=1, column=0, padx=10, pady=5)

    var_message_type = tk.StringVar(window)
    var_message_type.set("tts")  # Default value
    options_message_type = ["tts", "dms", "custom", "crash", "construction", "traffic hazard"]
    dropdown_message_type = tk.OptionMenu(window, var_message_type, *options_message_type)
    dropdown_message_type.grid(row=1, column=1, padx=10, pady=5)

    label_priority = tk.Label(window, text="Priority (1 to 10):")
    label_priority.grid(row=2, column=0, padx=10, pady=5)

    var_priority = tk.IntVar(window)
    var_priority.set(5)  # Default value
    dropdown_priority = tk.OptionMenu(window, var_priority, *range(1, 11))
    dropdown_priority.grid(row=2, column=1, padx=10, pady=5)

    label_duration = tk.Label(window, text="Duration (minutes):")
    label_duration.grid(row=3, column=0, padx=10, pady=5)

    entry_duration = tk.Entry(window, width=10)
    entry_duration.grid(row=3, column=1, padx=10, pady=5)

    btn_submit = tk.Button(window, text="Submit", command=save_new_entry)
    btn_submit.grid(row=4, column=0, columnspan=2, padx=10, pady=10)

    window.mainloop()

if __name__ == "__main__":
    create_gui()
