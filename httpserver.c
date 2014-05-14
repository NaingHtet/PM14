// Disabled because this doesn't really help. For the future students, please don't do things
// because your professor wants you to. Chances are he is not going to be satisfied by it either way.
// Just do what you want and what you like. Life is too short to live by someone else's words.
/**
//   * A simple text webserver to monitor the status of the pack manager. 
//   * Adapted from http://blog.manula.org/2011/05/writing-simple-web-server-in-c.html
//   */


// #include <netinet/in.h>    
// #include <stdio.h>    
// #include <stdlib.h>    
// #include <sys/socket.h>    
// #include <sys/stat.h>    
// #include <sys/types.h>    
// #include <string.h>
// #include <unistd.h>    
// #include <syslog.h>

// #include "httpserver.h"
// #include "pack_controller.h"
    
// void* handle_server(void* arg){

//    int create_socket, new_socket;    
//    socklen_t addrlen;    
//    int bufsize = 1024;    
//    char *buffer = malloc(bufsize);    
//    struct sockaddr_in address;    
 
//    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){    
//       // printf("The socket was created\n");
//    }
    
//    address.sin_family = AF_INET;    
//    address.sin_addr.s_addr = INADDR_ANY;    
//    address.sin_port = htons(8000);    
    
//    if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){    
//       // printf("Binding Socket\n");
//    }
    
//    syslog(LOG_NOTICE, "Server created\n");

//    while (1) {    
//       if (listen(create_socket, 10) < 0) {    
//          perror("server: listen");    
//          exit(1);    
//       }    
    
//       if ((new_socket = accept(create_socket, (struct sockaddr *) &address, &addrlen)) < 0) {    
//          perror("server: accept");    
//          exit(1);    
//       }    
    
//       if (new_socket > 0){    
//          // printf("The Client is connected...\n");
//       }
//       recv(new_socket, buffer, bufsize, 0);    
//       // printf("%s\n", buffer);    
//       write(new_socket, "HTTP/1.1 200 OK\n", 16);
//       write(new_socket, "Content-length:1000\n", 19);
//       write(new_socket, "Content-Type: text/html\n\n", 25);
//       char datacpy[2000];
//       get_html_str(datacpy);

//       char header[2000];
//       sprintf(header,"<html><head><meta http-equiv=\"refresh\" content=\"5\"></head><body><pre>%s</pre></body></html>", datacpy);
//       // printf("string length is %d", strlen(header));
//       write(new_socket, header,strlen(header));
//       close(new_socket);    
//    }    
//    close(create_socket);    
//    return 0;    
// }