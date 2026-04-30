PIO = C:\Users\zhl11\.platformio\penv\Scripts\platformio.exe
PORT = com3

pull:
	git $@

upload:
	$(PIO) run -t upload --upload-port $(PORT)

monitor:
	$(PIO) device monitor -b 115200 --port $(PORT)

list:
	$(PIO) device list

run:
	$(PIO) run

all: pull list run upload monitor