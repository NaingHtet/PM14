CC = gcc-arm

all: pm14

pm14: 
	$(CC) main_controller.c serial_controller.c i2c_controller.c -o pm14