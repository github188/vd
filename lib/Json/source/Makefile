libJson.so:json_reader.o json_value.o json_writer.o
	/home/bitcom/IPNC_RDK_V3.8.0/Source/ti_tools/linux_devkit/bin/arm-arago-linux-gnueabi-g++  -shared -fPIC json_reader.o json_value.o json_writer.o -o libJson.so
json_reader.o:
	/home/bitcom/IPNC_RDK_V3.8.0/Source/ti_tools/linux_devkit/bin/arm-arago-linux-gnueabi-g++  -c json_reader.cpp -o json_reader.o -I /usr/include
json_value.o:
	/home/bitcom/IPNC_RDK_V3.8.0/Source/ti_tools/linux_devkit/bin/arm-arago-linux-gnueabi-g++  -c json_value.cpp -o json_value.o -I /usr/include
json_writer.o:
	/home/bitcom/IPNC_RDK_V3.8.0/Source/ti_tools/linux_devkit/bin/arm-arago-linux-gnueabi-g++  -c json_writer.cpp -o json_writer.o -I /usr/include
clean:
	rm *.o libJson.so
