//Sends SIGUSR1 signal to the pm14. This will enable the logging.


#include <stdio.h>
#include <signal.h>

int main() {
    FILE* fp;
    fp = fopen("/var/run/pm14.pid","r");
    int i = 0;
    fscanf(fp, "%d", &i);

    pid_t pid = i;
    kill(pid, SIGUSR1);
    printf("Toggled logging successfully. Check out.dat");
    fclose(fp);
    exit(0);
}