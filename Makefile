# `pkg-config --cflags --libs gtk+-3.0`
controller: controller.c net.c net.h
	gcc -g controller.c net.c -o controller `pkg-config --cflags --libs libevdev`
	
receiver: receiver.c sabertooth.c sabertooth.h net.c net.h
	gcc -g receiver.c net.c -o receiver