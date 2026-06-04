import paho.mqtt.client as mqtt

broker = "broker.hivemq.com"
topic = "buet/cse/2105152/led" # TODO: Put the same topic you used in the ESP code

client = mqtt.Client()
client.connect(broker)

# TODO: the following is an example of publishing a message. You have to modify it so that the python code will run infinitely and wait for input from keyboard. If user presses 'y', it will send "ON"; it will send "OFF" if 'n' is pressed. The program will terminate if user presses 'q'.
while True:
    user_input = input("Enter 'y' for ON, 'n' for OFF, 'q' to quit: ")
    
    if user_input == 'y':
        client.publish(topic, "ON")
        print("Sent: ON")
    elif user_input == 'n':
        client.publish(topic, "OFF")
        print("Sent: OFF")
    elif user_input == 'q':
        print("Quit")
        break
    else:
        print("Invalid input, plz enter 'y', 'n', or 'q'.")

client.disconnect()

# python3 -m venv venv
# source venv/bin/activate
# pip install paho-mqtt
# python sample_for_control.py