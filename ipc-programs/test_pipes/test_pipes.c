#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

typedef struct _child_process_info_t {
    pid_t pid;
    int write_pipe_fd;
} child_process_info_t;

#define PID_FORMAT "[PID %d]"
#define CHILD_PROCESSES 5

child_process_info_t child_processes[CHILD_PROCESSES];

unsigned char* generate_message(int size, bool add_ff)
{
    static const unsigned char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const size_t chars_size = (sizeof(chars) / sizeof(char)) - 1;

    int i;
    unsigned char* message = malloc(size + 1 + (add_ff ? 1 : 0));
    for (i = 0; i < size; i++) {
        message[i] = chars[rand() % chars_size];
    }
    if (add_ff) {
        message[i++] = 0xFF;
    }
    message[i] = 0;
    return message;
}

int random_int(int a, int b)
{
    int d = b - a + 1;
    return a + (rand() % d);
}

int do_parent_process()
{
    static const int message_lengths[CHILD_PROCESSES] = {32, 64, 100, 128, 150};
    static const unsigned char ff_char = 0xFF;
    srand((unsigned int)time(NULL));
    for (int process_num = 0; process_num < CHILD_PROCESSES; process_num++) {
        int message_len = message_lengths[process_num];
        bool add_ff = process_num & 1;
        char *message = (char*)generate_message(message_len, add_ff);

        printf(PID_FORMAT" Sending message %s (length %d) to PID %d (0xFF %s)\n", getpid(), message, message_len,
               child_processes[process_num].pid, add_ff ? "in the message" : "in next message");
        if (add_ff) {
            write(child_processes[process_num].write_pipe_fd, message, message_len + 1);
        } else {
            write(child_processes[process_num].write_pipe_fd, message, message_len);
            write(child_processes[process_num].write_pipe_fd, &ff_char, 1);
        }

        free(message);
        waitpid(child_processes[process_num].pid, NULL, 0);
    }

    usleep(1000);
    return 0;
}

int do_child_process(int read_fd)
{
#define MAX_MESSAGE_LEN 128
#define BUFFER_SIZE 64
    static const unsigned char ff_char = 0xFF;
    unsigned char message[MAX_MESSAGE_LEN + 1];
    unsigned char buf[BUFFER_SIZE];
    int message_len = 0;
    bool end_of_transmission = false;

    memset(message, 0, sizeof(message));
    while (!end_of_transmission) {
        ssize_t read_bytes = read(read_fd, buf, BUFFER_SIZE);
        for (unsigned char *c = buf; read_bytes > 0 && *c; c++, read_bytes--) {
            if (*c == ff_char || message_len >= MAX_MESSAGE_LEN) {
                end_of_transmission = true;
            } else {
                message[message_len++] = *c;
            }
        }
    }
    message[message_len] = 0;

    {
        printf(PID_FORMAT" Message: %s (length %d)\n", getpid(), message, message_len);
        char *hex_string = malloc(3 * message_len + 16);
        char *hex_string_buf = hex_string;
        hex_string_buf += snprintf(hex_string_buf, 16, PID_FORMAT, getpid());
        for (int i = 0; i < message_len; i++) {
            hex_string_buf += snprintf(hex_string_buf, 4, " %02X", (int)message[i]);
        }
        printf("%s\n", hex_string);
        free(hex_string);
    }

    return 0;
#undef MAX_MESSAGE_LEN
#undef BUFFER_SIZE
}

int main()
{
    memset(&child_processes, 0, sizeof(child_process_info_t) * CHILD_PROCESSES);
    for (int i = 0; i < CHILD_PROCESSES; i++) {
        int fd[2];
        pipe(fd);

        pid_t pid = fork();
        switch (pid) {
        case -1:
            perror("fork");
            return 1;
        case 0:
            close(fd[1]);
            return do_child_process(fd[0]);
        default:
            child_processes[i].pid = pid;
            child_processes[i].write_pipe_fd = fd[1];
            close(fd[0]);
            break;
        }
    }

    return do_parent_process();
}
