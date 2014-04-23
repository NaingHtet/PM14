CC = gcc-arm
CFLAGS = -pthread -lm
all: pm14

pm14: 
	$(CC) $(CFLAGS)  main_controller.c errorhandler.c serial_controller.c i2c_controller.c safety_checker.c display_controller.c lib/libconfig.a -o pm14

clean:
	rm pm14

.PHONY: all pm14 safety_checker clean