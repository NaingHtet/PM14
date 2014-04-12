gcc-arm safety_checker.c main_controller.c i2c_controller.c serial_controller.c -pthread -o testing
scp testing root@139.147.103.17:~/PM14