import paho.mqtt.client as mqtt
import json
import time
import sys
import tkinter as tk
from PIL import ImageTk, Image, ImageDraw, ImageFont
import requests
from io import BytesIO
import os


sub_messages = 0
ts_map = {}
prev_tt = dict()
prev_dms = dict()
speedometer_image_path = './Demo/assets/speedometer.png'

def load_base_image(composite_image):
    # Load the speedometer image
    speedometer_image = Image.open(speedometer_image_path)  # Replace "speedometer.png" with the path to your speedometer image
    speedometer_image = speedometer_image.resize((1200, 500))

    

    # Paste the speedometer image as the background
    composite_image.paste(speedometer_image, (0, 0))

def load_image(device_nbr, travel_times, composite_image, additional_mile_values,  interstate_value, interstate_direction):
    # Load the sign image from the offline file
    sign_image_path = f"./Demo/assets/Road-signs-template/{device_nbr}.png"
    if os.path.exists(sign_image_path):
        sign_image = Image.open(sign_image_path)
    else:
        print(f"Sign image for device {device_nbr} not found.")
        return

    sign_image = sign_image.resize((190, 190))  # Adjust the size of the sign image as per your requirement

    # Calculate the position to place the sign image in the middle
    sign_x = (composite_image.width - sign_image.width) // 2
    sign_y = ((composite_image.height - sign_image.height) // 2) - 100

    # Paste the sign image on top of the speedometer image
    composite_image.paste(sign_image, (sign_x, sign_y), mask=sign_image)

    # Create a drawing context
    draw = ImageDraw.Draw(composite_image)

    # Set the font and size for travel times
    font = ImageFont.truetype('./Demo/assets/font/FreeMono.ttf', size=20)
    mile_font = ImageFont.truetype('./Demo/assets/font/FreeMonoBold.ttf', size=16)


    # Extract the travel times and additional mile values
    travel_time1, travel_time2 = travel_times
    mile_value1, mile_value2 = additional_mile_values

    # Calculate the text positions for travel times
    text_width1, text_height1 = draw.textsize(travel_time1, font=font)
    text_x1 = 530
    text_y1 = ((composite_image.height - sign_image.height) // 2) + 50

    text_width2, text_height2 = draw.textsize(travel_time2, font=font)
    text_x2 = 620
    text_y2 = ((composite_image.height - sign_image.height) // 2) + 50

    # Calculate the text positions for mile values
    mile_width1, mile_height1 = draw.textsize(mile_value1, font=mile_font)
    mile_x1 = 518
    mile_y1 = text_y1 - mile_height1 - 28  # Place above travel time

    mile_width2, mile_height2 = draw.textsize(mile_value2, font=mile_font)
    mile_x2 = 608
    mile_y2 = text_y2 - mile_height2 - 28 # Place above travel time

    # Draw the background for mile values using the specific color
    draw.rectangle((mile_x1, mile_y1, mile_x1 + mile_width1+2, mile_y1 + mile_height1+5), fill=(24, 69, 134, 255))
    draw.rectangle((mile_x2, mile_y2, mile_x2 + mile_width2+2, mile_y2 + mile_height2+5), fill=(24, 69, 134, 255))

    # Draw the text on the image for travel times and mile values
    draw.text((text_x1, text_y1), travel_time1, fill='white', font=font)
    draw.text((text_x2, text_y2), travel_time2, fill='white', font=font)
    draw.text((mile_x1, mile_y1), mile_value1, fill='white', font=mile_font)
    draw.text((mile_x2, mile_y2), mile_value2, fill='white', font=mile_font)
     # Create a drawing context
    draw = ImageDraw.Draw(composite_image)
    # Set the font and size for the interstate text
    font = ImageFont.truetype('./Demo/assets/font/FreeMono.ttf', size=23)

    # Calculate the text position for the interstate text
    interstate_text = f"I-{interstate_value} {interstate_direction}"
    text_width, text_height = draw.textsize(interstate_text, font=font)
    text_x = (speedometer_image.width - text_width) // 2
    text_y = 20  # Adjust the Y position as needed
    # Draw the interstate text on the composite image
    draw.text((text_x, text_y), interstate_text, fill='white', font=font)


def create_text_image(text, composite_image):
    # Create a drawing context
    draw = ImageDraw.Draw(composite_image)

    # Set the font and size
    font = ImageFont.truetype('./Demo/assets/font/FreeMono.ttf', size=22)

    # Calculate the text position
    text_width, text_height = draw.textsize(text, font=font)
    text_x = (composite_image.width - text_width) // 2
    text_y = ((composite_image.height - text_height) // 2) + 100

    # Draw the text on the image
    draw.text((text_x, text_y), text, fill='yellow', font=font)


def on_connect(client, userdata, flags, rc, properties):
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("V2X-Traveller-Information")
    print("Connected with result code " + str(rc))


window = tk.Tk()
window.title("Road messages")  # Set the title of the window

# Load the speedometer image
speedometer_image = Image.open(speedometer_image_path)
speedometer_image = speedometer_image.resize((1200, 500))

# Create a new blank image with the size of the speedometer image
# composite_image = Image.new('RGBA', speedometer_image.size)

def on_message(client, userdata, msg):
    print("message received: ", msg.payload)
    msg_json = json.loads(msg.payload)
    msg_json['received_time'] = time.time()
    msg_json_counter = msg_json['counter']
    with open("./Demo/received_msgs/{}.json".format(msg_json_counter), "w") as f:
        json.dump(msg_json, f, indent=2)
    
    try:
        message_type = msg_json['message_type']
        
        # Create a new blank image with the size of the speedometer image
        composite_image = Image.new('RGBA', (1200, 500))  # Use fixed size for consistency
        load_base_image(composite_image)  # Pass the interstate value here
        
        if message_type == 'tts':
            travelTimes = str(int(msg_json['message1']) + msg_json['time_to_rsu']), str(int(msg_json['message2']) + msg_json['time_to_rsu'])
            mile_values = ("{} MILES".format(msg_json['lsign_distance']), "{} MILES".format(msg_json['rsign_distance']))
            prev_tt['device_nbr'] = msg_json['device_nbr']
            prev_tt['travelTimes'] = travelTimes
            prev_tt['mile_values'] = mile_values
            prev_tt['interstate'] = msg_json.get('interstate', '')
            prev_tt['direction'] = msg_json.get('direction','')
            if prev_dms:
                create_text_image(prev_dms['input_text'], composite_image)
            load_image(msg_json['device_nbr'], travelTimes, composite_image, mile_values, msg_json.get('interstate', ''),msg_json.get('direction',''))
        else:
            textLines = msg_json['message1'], msg_json['message2'], msg_json['message3']
            input_text = "\n".join(textLines)
            prev_dms['input_text'] = input_text
            if prev_tt:
                load_image(prev_tt['device_nbr'], prev_tt['travelTimes'], composite_image, prev_tt['mile_values'],prev_tt.get('interstate', ''),prev_tt.get('direction',''))
            create_text_image(input_text, composite_image)

        # Display the composite image with all content
        photo = ImageTk.PhotoImage(composite_image)
        label.config(image=photo)
        label.image = photo

        window.update()  # Update the window to display the new image
        time.sleep(2.5)  # Delay of 2.5 seconds
    except KeyError:
        # Handle the case where required keys are not present in the message
        print("Error: Required keys not found in the message.")



# Create a Label widget to display the image
label = tk.Label(window)
label.pack()


def main(vehicle_name,  is_local):
    global sub_messages
    global prev_tt
    global prev_dms
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,vehicle_name,clean_session=False,reconnect_on_failure=True)
    # client.username_pw_set('mqtt-test1', 'tasi')
    client.on_connect = on_connect
    client.on_message = on_message

    if is_local == 'true':    
        client.connect("localhost", 1883, 60)
    else:
        client.connect("10.0.2.11", 1883, 60)
    try:
        client.loop_start()
        window.mainloop()
    except KeyboardInterrupt:
        print("exiting")
        client.disconnect()
        client.loop_stop()


if __name__ == "__main__":
    #read user input and assign the vehicle name to the client by passing to main function
    vehicle_name = sys.argv[1]
    is_local = sys.argv[2]
    main(vehicle_name,is_local)
