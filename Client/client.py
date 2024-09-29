import serial
import json
import argparse


parser = argparse.ArgumentParser(description="Communicate with Hardware Password Manager")
parser.add_argument('--dev', type=str, default="/dev/ttyUSB0", nargs='?', const="/dev/ttyUSB0")
parser.add_argument('--speed', type=int, default=115200, nargs='?', const=115200)
parser.add_argument('--command', type=str)
parser.add_argument('--password', type=str, nargs='?')
args = parser.parse_args()

serial = serial.Serial(args.dev, args.speed)
serial.write(b"set_maintance")
if args.command == "internal_dump":
    serial.write(b"internal_dump")
    # serial.write(args.password.encode("utf-8")) # password
    


serial.write(b"sf_reboot")