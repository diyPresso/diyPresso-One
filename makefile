upload:
	pio run -t upload --upload-port=/dev/ttyACM0

docker:
	docker run -e HOME=`pwd` -u `id -u` -w `pwd` -v `pwd`:`pwd` --rm -it takigama/platformio platformio run

release:
	pio run
	cp .pio/build/mkr_wifi1010/firmware.bin releases

clean:
	rm -rf .pio .platformio
