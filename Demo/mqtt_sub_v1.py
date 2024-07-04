import paho.mqtt.client as mqtt
import json
import time

import tkinter as tk
from PIL import ImageTk, Image, ImageDraw, ImageFont
import requests
from io import BytesIO


sub_messages     = 0
ts_map = {}

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("V2X-Traveller-Information")


window = tk.Tk()
window.title("Road messages")  # Set the title of the window

# Create a Label widget to display the image
label = tk.Label(window)
label.pack()


def load_image(url, travel_times):
    # Load the speedometer image
    speedometer_image = Image.open("./Demo/speedometer.png")  # Replace "speedometer_image.jpg" with the path to your speedometer image
    speedometer_image = speedometer_image.resize((1200,500))

    response = requests.get(url)
    image_data = response.content
    sign_image = Image.open(BytesIO(image_data))
    sign_image = sign_image.resize((190, 190))  # Adjust the size of the sign image as per your requirement

    # Create a new blank image with the size of the speedometer image
    composite_image = Image.new('RGBA', speedometer_image.size)

    # Paste the speedometer image as the background
    composite_image.paste(speedometer_image, (0, 0))

    # Calculate the position to place the sign image in the middle
    sign_x = (composite_image.width - sign_image.width) // 2
    sign_y = (composite_image.height - sign_image.height) // 2

    # Paste the sign image on top of the speedometer image
    composite_image.paste(sign_image, (sign_x, sign_y), mask=sign_image)

    # Create a drawing context
    draw = ImageDraw.Draw(composite_image)

    # Set the font and size
    font = ImageFont.truetype('/usr/share/fonts/truetype/freefont/FreeMono.ttf', size=20)

    # Extract the travel times
    travel_time1, travel_time2 = travel_times

    # Calculate the text positions
    text_width1, text_height1 = draw.textsize(travel_time1, font=font)
    text_x1 = 530
    text_y1 = 305

    text_width2, text_height2 = draw.textsize(travel_time2, font=font)
    text_x2 = 620
    text_y2 = 305

    # Draw the text on the image
    draw.text((text_x1, text_y1), travel_time1, fill='white', font=font)
    draw.text((text_x2, text_y2), travel_time2, fill='white', font=font)

    photo = ImageTk.PhotoImage(composite_image)
    label.config(image=photo)  # Update the image in the Label widget
    label.image = photo  # Save a reference to the image to prevent garbage collection

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print ("message received: " , msg.payload)
    msg_json = json.loads( msg.payload)


    def create_text_image(text):
       # Load the speedometer image
        speedometer_image = Image.open("./Demo/speedometer.png")  # Replace "speedometer.png" with the path to your speedometer image
        speedometer_image = speedometer_image.resize((1200,500))
        
        # Create a new blank image with the size of the speedometer image
        composite_image = Image.new('RGBA', speedometer_image.size)

        # Paste the speedometer image as the background
        composite_image.paste(speedometer_image, (0, 0))

        # Create a drawing context
        draw = ImageDraw.Draw(composite_image)

        # Set the font and size
        font = ImageFont.truetype('/usr/share/fonts/truetype/freefont/FreeMono.ttf', size=22)

        # Calculate the text position
        text_width, text_height = draw.textsize(text, font=font)
        text_x = (composite_image.width - text_width) // 2
        text_y = (composite_image.height - text_height) // 2

        # Draw the text on the image
        draw.text((text_x, text_y), text, fill='yellow', font=font)

        # Display the image
        photo = ImageTk.PhotoImage(composite_image)
        label.config(image=photo)  # Update the image in the Label widget
        label.image = photo  # Save a reference to the image to prevent garbage collection


    try:
            if  msg_json['signDisplayType']== 'OVERLAY_TRAVEL_TIME':
                msgImg = msg_json['views'][0]['imageUrl']
                travelTimes= msg_json['views'][0]['travelTimes']

                url = "https://511in.org" + msgImg
                print("\nurl:" + url)
                load_image(url,travelTimes)
                window.update()  # Update the window to display the new image
                time.sleep(2.5) 
                #image_urls.append(url)
    except KeyError:
            # Handle the case where 'imageUrl' key is not present
            print(f"Skipping index  as 'imageUrl' is not available")

    try:
        textLines = msg_json['views'][0]['textLines']
        # Create and display an image with input text
        input_text = textLines[0]+'\n'+textLines[1]+'\n'+textLines[2]
        create_text_image(input_text)
        window.update()  # Update the window to display the new image
        time.sleep(2.5)  # Delay of

    except KeyError:
        # Handle the case where 'imageUrl' key is not present
        print(f"Skipping index as 'textLines' is not available")

    
    now_time = time.time()
    travel_time = (now_time - msg_json['time'] )* 1000

    # print the message data to the terminal
    # print("Received message from {}: {}".format(address, msg_json))


    # print("Elapsed time: {} seconds".format(elapsed_time))
    # print("Travel time: {} seconds for {} message".format(travel_time,msg_json['counter']))

def main():
    global sub_messages

    client = mqtt.Client()
    client.username_pw_set('mqtt-test1','tasi')
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect("10.0.2.11", 1883, 60)
    #client.connect("localhost", 1883, 60)

   
    # Blocking call that processes network traffic, dispatches callbacks and
    # handles reconnecting.
    # Other loop*() functions are available that give a threaded interface and a
    # manual interface.
    client.loop_forever()

main()