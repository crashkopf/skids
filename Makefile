
all: controller receiver jcmd

controller: controller.c net.c net.h
	gcc -g controller.c net.c -o controller `pkg-config --cflags --libs libevdev`
	
receiver: receiver.c control.c control.h sabertooth.h net.c net.h timer.c timer.h intmath.h
	gcc -g receiver.c net.c control.c timer.c -o receiver -lrt
	
jcmd: jcmd.c sabertooth.h
	gcc -g jcmd.c -o jcmd

clean:
	rm controller receiver jcmd
