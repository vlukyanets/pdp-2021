#include <unistd.h>
#include <stdio.h>


int main()
{
    pid_t pid = fork();

    if (pid < 0) {
        printf("Fork failed!\n");
    } else if (pid == 0) {
        printf("Hello from child process, PID %d (parent PID %d)\n", getpid(), getppid());
    } else {
        printf("Hello from parent process, PID %d\n", getpid());
    }

    usleep(1000);
    return 0;
}
