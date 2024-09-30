import serial
import json
import argparse
import time

parser = argparse.ArgumentParser(description="Communicate with Hardware Password Manager")
parser.add_argument('--dev', type=str, default="/dev/ttyUSB0", nargs='?', const="/dev/ttyUSB0")
parser.add_argument('--speed', type=int, default=115200, nargs='?', const=115200)
parser.add_argument('--command', type=str)
parser.add_argument('--password', type=str, nargs='?')
args = parser.parse_args()

serial = serial.Serial(args.dev, args.speed)
if args.command == "add_pass":
    serial.write(b"add_pass")
    time.sleep(2)
    serial.write(b"123123")
    time.sleep(0.5)
    print(serial.in_waiting)
    while serial.in_waiting > 0:
        print(serial.read(serial.in_waiting), end="")
    print()
    # serial.write(args.password.encode("utf-8")) # password
    


serial.write(b"sf_reboot")
