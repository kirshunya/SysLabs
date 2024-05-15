#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_CHILDREN 10

pid_t child_pids[MAX_CHILDREN];
int child_count = 0;

void sigusr1_handler(int signum) {}


void sigusr2_handler(int signum) {}

void create_child_process() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork error");
    } else if (pid == 0) {
        // В дочернем процессе
        char child_index[10];
        snprintf(child_index, sizeof(child_index), "%d", child_count + 1);
        execl("./child", "child", child_index, NULL);
        perror("Exec error");
        exit(EXIT_FAILURE);
    } else {
        // В родительском процессе
        child_pids[child_count++] = pid;
        printf("Child C_%d created with PID %d\n", child_count, pid);
    }
}

void remove_last_child_process() {
    if (child_count > 0) {
        kill(child_pids[--child_count], SIGKILL);
        printf("Child C_%d killed. %d children left\n", child_count + 1, child_count);
    } else {
        printf("No child processes to remove\n");
    }
}

void print_process_info() {
    printf("Parent PID: %d\n", getpid());
    for (int i = 0; i < child_count; ++i) {
        printf("Child C_%d PID: %d\n", i + 1, child_pids[i]);
    }
}

int main() {
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    char input[100];

    while (1) {
        printf("\nEnter command: \n + \n - \n l \n k \n s \n g \n p<num> \n q<num> \nCommand:");
        if (!fgets(input, sizeof(input), stdin)) {
            perror("Failed to read input");
            continue;
        }

        switch (input[0]) {
            case '+':
                create_child_process();
                break;
            case '-':
                remove_last_child_process();
                break;
            case 'l':
                print_process_info();
                break;
            case 'k':
                while (child_count > 0) {
                    kill(child_pids[--child_count], SIGKILL);
                }
                printf("All child processes killed :/// ur the murder\n");
                break;
            case 's':
            case 'g':
                if (strlen(input) > 1 && input[1] >= '1' && input[1] <= '9') {
                    int child_index = input[1] - '1';
                    if (child_index < child_count) {
                        int signal_type = (input[0] == 's') ? SIGUSR1 : SIGUSR2;
                        kill(child_pids[child_index], signal_type);
                        printf("Signal %c sent to child C_%d\n", input[0], child_index + 1);
                    } else {
                        printf("Child C_%d does not exist\n", child_index + 1);
                    }
                } else {
                    int signal_type = (input[0] == 's') ? SIGUSR1 : SIGUSR2;
                    for (int i = 0; i < child_count; ++i) {
                        kill(child_pids[i], signal_type);
                    }
                    printf("Signal %c sent to all children\n", input[0]);
                }
                break;
            case 'p':
                if (strlen(input) > 1 && input[1] >= '1' && input[1] <= '9') {
                    int child_index = input[1] - '1';
                    if (child_index < child_count) {
                        kill(child_pids[child_index], SIGUSR2);
                        printf("Requested child C_%d to print statistics\n", child_index + 1);
                    } else {
                        printf("Child C_%d does not exist\n", child_index + 1);
                    }
                }
                break;
            case 'q':
                while (child_count > 0) {
                    kill(child_pids[--child_count], SIGKILL);
                }
                printf("All child processes killed. Exiting...\n");
                exit(EXIT_SUCCESS);
            default:
                printf("Invalid command\n");
                break;
        }
    }
}