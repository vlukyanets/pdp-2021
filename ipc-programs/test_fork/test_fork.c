#include <unistd.h>
#include <stdio.h>

int main()
{
    pid_t pid = fork();

    switch (pid) {
    case -1:
        printf("Fork failed!\n");
        break;
    case 0:
        printf("Hello from child process, PID %d (parent PID %d)\n", getpid(), getppid());
        break;
    default:
        printf("Hello from parent process, PID %d\n", getpid());
        break;
    }

    usleep(1000);
    return 0;
}
