#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define PID_FORMAT "[PID %d]"
#define WORKER_FORMAT "[Worker #%lu]"
#define PROJ_ID 100

#define MESSAGE_LEN 32
#define MESSAGE_DUMP_BUF (MESSAGE_LEN) + 100

#define LENGTH(x) (sizeof(x) / sizeof(*(x)))

typedef enum _cmd_type {
    CMD_PRINT_MESSAGE,
    CMD_SLEEP_MSECS,
} cmd_type_t;

typedef struct _message_t {
    long type;
    struct {
        cmd_type_t cmd;
        union {
            char text[MESSAGE_LEN + 1];
            uint32_t sleep_msecs;
        } data;
    } m;
} message_t;

bool g_any_worker_handle = false;
long g_worker_num = 0;
int g_queue_id = 0;

int rand_int(int l, int r) {
    int d = r - l + 1;
    return l + rand() % d;
}

char rand_alnum_char() {
    int d = rand_int(0, 61);
    if (d < 10)
        return '0' + d;
    else if (d < 10 + 26)
        return 'a' + d - 10;
    else
        return 'A' + d - 10 - 26;
}

void random_fill_message(message_t *message)
{
    memset(message, 0, sizeof(message_t));

    switch (rand() & 1) {
    case 0:
        message->m.cmd = CMD_PRINT_MESSAGE;
        for (int i = 0; i < MESSAGE_LEN; i++)
            message->m.data.text[i] = rand_alnum_char();
        break;

    case 1:
        message->m.cmd = CMD_SLEEP_MSECS;
        message->m.data.sleep_msecs = rand_int(100, 1000);
        break;
    }
}

void dump_message(const message_t *message)
{
    if (NULL == message)
        return;

    char buffer[MESSAGE_LEN + 1];
    memset(&buffer, 0, sizeof(buffer));

    switch (message->m.cmd) {
    case CMD_PRINT_MESSAGE:
        memcpy(buffer, message->m.data.text, MESSAGE_LEN * sizeof(char));
        break;
    case CMD_SLEEP_MSECS:
        snprintf(buffer, MESSAGE_LEN + 1, "Sleep %u msecs", message->m.data.sleep_msecs);
        break;
    }

    printf(PID_FORMAT" Message (type %d): %s (assigned to worker #%lu)\n", getpid(), message->m.cmd, buffer, message->type);
}

void parent_process_atexit()
{
    printf(PID_FORMAT " finished gracefully!\n", getpid());
}

int do_parent_process(long workers, long messages)
{
    int rc;

    rc = atexit(parent_process_atexit);
    if (0 != rc) {
        perror("atexit");
        return 1;
    }

    /* Wait for all ready. Nice to have some sort of sync */
    sleep(1);

    srand((unsigned int)time(NULL));
    for (long i = 1; i <= messages; i++) {
        message_t message;
        random_fill_message(&message);
        message.type = 1 + (i % workers);
        dump_message(&message);

        rc = msgsnd(g_queue_id, &message, sizeof(message.m), 0);
        if (-1 == rc) {
            perror("msgsnd");
            return 1;
        }

        sleep(1);
    }

    rc = msgctl(g_queue_id, IPC_RMID, NULL);
    if (-1 == rc) {
        perror("msgctl");
        return 1;
    }

    /* Wait for all children processes */
    pid_t pid;
    while ((pid = wait(NULL)) > 0);

    return 0;
}

void handle_message(const message_t *message)
{
    if (NULL == message)
        return;

    switch (message->m.cmd) {
    case CMD_PRINT_MESSAGE:
        printf(PID_FORMAT WORKER_FORMAT " Message: %s\n", getpid(), g_worker_num, message->m.data.text);
        break;
    case CMD_SLEEP_MSECS:
        printf(PID_FORMAT WORKER_FORMAT " Sleeping %u msecs...\n", getpid(), g_worker_num, message->m.data.sleep_msecs);
        usleep(message->m.data.sleep_msecs * 1000);
        printf(PID_FORMAT WORKER_FORMAT " Finished sleeping %u msecs\n", getpid(), g_worker_num, message->m.data.sleep_msecs);
        break;
    }
}

void child_process_atexit()
{
    printf(PID_FORMAT WORKER_FORMAT " finished gracefully!\n", getpid(), g_worker_num);
}

/* For such types of applications (main app / workers),
 * from my POV better to have separated worker application.
 * Also if are workers are equal, better to use 0 instead of 'g_worker_num' in 'msg_rcv'
 */
int do_child_process()
{
    int rc;

    rc = atexit(child_process_atexit);
    if (0 != rc) {
        perror("atexit");
        return 1;
    }

    while (true) {
        message_t message;

        int rc = msgrcv(g_queue_id, &message, sizeof(message.m), g_worker_num, 0);
        if (-1 == rc) {
            if (EIDRM == errno || EINVAL == errno) {
                /* Message queue is not valid --> stop it */
                break;
            } else {
                perror("msgrcv");
                return 1;
            }
        }

        handle_message(&message);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int opt;

    long workers = 2;
    long messages = 10;

    while ((opt = getopt(argc, argv, "w:m:h")) != -1) {
        switch (opt) {
        case 'w':
            workers = atoi(optarg);
            break;
        case 'm':
            messages = atoi(optarg);
            break;
        case 'h':
            g_any_worker_handle = true;
            break;
        default:
            fprintf(stderr, "Usage: %s [-w workers_num] [-m messages_amount] [-h]\n", argv[0]);
            return 1;
        }
    }

    printf(PID_FORMAT" Parent process launched\n", getpid());

    g_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (-1 == g_queue_id) {
        perror("msgget");
        return 1;
    }

    for (long i = 0; i < workers; i++) {
        pid_t pid = fork();
        switch (pid) {
        case -1:
            perror("fork");
            return 1;
        case 0:
            g_worker_num = i + 1;
            printf(PID_FORMAT WORKER_FORMAT " launched\n", getpid(), g_worker_num);
            return do_child_process();
        default:
            break;
        }
    }

    return do_parent_process(workers, messages);
}
