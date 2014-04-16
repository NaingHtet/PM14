CC = gcc-arm
CFLAGS = -pthread
all: pm14

pm14: 
	$(CC) $(CFLAGS)  main_controller.c serial_controller.c i2c_controller.c safety_checker.c lib/libconfig.a -o pm14

clean:
	rm pm14

.PHONY: all pm14 clean