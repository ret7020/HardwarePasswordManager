flash:
	arduino-cli compile ./firmware/firmware.ino --fqbn esp32:esp32:nodemcu-32s --port $(PORT) --upload --verbose