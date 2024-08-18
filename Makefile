manager:
	arduino-cli compile ./Manager/Manager.ino --fqbn esp32:esp32:nodemcu-32s --port $(PORT) --upload --verbose
