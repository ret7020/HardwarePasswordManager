# manager:
# 	arduino-cli compile ./Manager/Manager.ino --fqbn esp32:esp32:nodemcu-32s --port $(PORT) --upload --verbose

flah:
	arduino-cli compile ./firmware/firmware.ino --fqbn esp32:esp32:nodemcu-32s --port $(PORT) --upload --verbose