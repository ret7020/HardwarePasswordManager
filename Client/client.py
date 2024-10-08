import serial
import json
import argparse
import time

parser = argparse.ArgumentParser(description="Communicate with Hardware Password Manager")
parser.add_argument('--dev', type=str, default="/dev/ttyUSB0", nargs='?', const="/dev/ttyUSB0")
parser.add_argument('--speed', type=int, default=115200, nargs='?', const=115200)
parser.add_argument('--command', type=str)
# parser.add_argument('--master_key', type=str, nargs='?') # Can be saved in *sh history, so only stdin input
parser.add_argument('--pass_file', type=str, nargs='?')
parser.add_argument('--bypass_checks', type=int, nargs='?', default=0, const=0)
args = parser.parse_args()


if not args.bypass_checks:
    input("Highly recomend to unlock device if it is locked. Press Enter")

serial = serial.Serial(args.dev, args.speed)
time.sleep(0.1)
if not args.bypass_checks:
    serial.write(b"set_maintance")
    time.sleep(0.5)
    maintance_mode_ok = input("Device in maintance mode? (y/n)")
else: maintance_mode_ok = 'y'

if maintance_mode_ok == 'y':
    if args.command == "add_pass":
        with open(args.pass_file) as fd:
            passwords = json.load(fd)

        master_key = input("Master Key>")
        data = {"data": passwords, "master_key": master_key}
        serial.write(b"add_pass")
        while serial.in_waiting < 1: pass # Waiting for dev request for command
        serial.reset_input_buffer()
        serial.write(json.dumps(data).encode("utf-8"))
        time.sleep(1)
    elif args.command == "read_notes":
        print("Warning! This function will work only if firmware with SECURE_NOTES defined!")
        master_key = input("Master Key>")
        data = json.dumps({"master_key": master_key}).encode("utf-8")
        serial.write(b"read_notes")
        while serial.in_waiting < 1: pass
        serial.reset_input_buffer()
        serial.write(data)

        time.sleep(1)
        while serial.in_waiting > 0:
            print(serial.read().decode("utf-8"))

        print()
    
    elif args.command == "get_curr_value":
        serial.write(b"get_curr_value")
        time.sleep(0.2)
        res = ""
        while serial.in_waiting > 0:
            res += serial.read(serial.in_waiting).decode("utf-8")
        res = res[:-1]
        if res: print(res)
        else: print("N/A")

    if not args.bypass_checks:
        serial.write(b"sf_reboot")
