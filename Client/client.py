import serial
import json
import argparse
import time

parser = argparse.ArgumentParser(description="Communicate with Hardware Password Manager")
parser.add_argument('--dev', type=str, default="/dev/ttyUSB0", nargs='?', const="/dev/ttyUSB0")
parser.add_argument('--speed', type=int, default=115200, nargs='?', const=115200)
parser.add_argument('--command', type=str)
parser.add_argument('--master_key', type=str, nargs='?')
parser.add_argument('--pass_file', type=str, nargs='?')
args = parser.parse_args()


serial = serial.Serial(args.dev, args.speed)
time.sleep(0.5)
serial.write(b"set_maintance")
time.sleep(0.5)
maintance_mode_ok = input("Device in maintance mode? (y/n)")
if maintance_mode_ok == 'y':
    if args.command == "add_pass":
        with open(args.pass_file) as fd:
            passwords = json.load(fd)

        data = {"data": passwords, "master_key": args.master_key}
        serial.write(b"add_pass")
        while serial.in_waiting < 1: pass # Waiting for dev request for command
        serial.flush()
        serial.write(json.dumps(data).encode("utf-8"))
        time.sleep(1)


    serial.write(b"sf_reboot")
