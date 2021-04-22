#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>

#define SIZE 64

int main()
{
    int *ptr = mmap(NULL, 2 * SIZE * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    pid_t pid = fork();
    size_t start_idx, finish_idx;
    bool positive_values = false;

    switch (pid) {
    case -1:
        perror("fork");
        return 1;
    case 0:
        start_idx = 0;
        finish_idx = SIZE;
        positive_values = true;
        break;
    default:
        start_idx = SIZE;
        finish_idx = 2 * SIZE;
        break;
    }

    int counter = 0;
    for (size_t i = start_idx; i < finish_idx; i++)
        ptr[i] = positive_values ? counter++ : -(counter++);

    if (pid > 0) {
        waitpid(pid, NULL, 0);
        for (size_t i = 0, j = SIZE; i < SIZE && j < 2 * SIZE; i++, j++)
            if (ptr[i] != -ptr[j]) {
                fprintf(stderr, "Shared memory not working as expected\n");
                return 1;
            }

        printf("Shared memory worked as expected\n");
    }

    if (0 != munmap(ptr, 2 * SIZE * sizeof(int))) {
        perror("munmap");
        return 1;
    }

    return 0;
}
