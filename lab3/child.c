#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>

#define INTERVAL 100000000L

struct pair {
    int x, y;
};

struct pair pair;

int statOl = 0, statOO = 0, statll = 0, statlO = 0;
long counter = 0;
bool output_allowed = false;

void alarm_handler(int signo) {
    if (signo != SIGALRM)
        return;
    if (pair.x > pair.y)
        statlO++;
    else if (pair.x < pair.y)
        statOl++;
    else if (pair.x && pair.y)
        statll++;
    else
        statOO++;

    return;
}

void sig1_handler(int signo) {
    if (signo != SIGUSR1)
        return;
    output_allowed = false;
    return;
}

void sig2_handler(int signo) {
    if (signo != SIGUSR2)
        return;
    output_allowed = true;
    return;
}

int main(int argc, char** argv) {
    signal(SIGALRM, alarm_handler);
    signal(SIGUSR1, sig1_handler);
    signal(SIGUSR2, sig2_handler);

    char mes[200] = {'\0'};

    struct itimerval timerval;
    timerval.it_interval.tv_sec = timerval.it_value.tv_sec = 0;
    timerval.it_interval.tv_usec = timerval.it_value.tv_usec = 5000;

    setitimer(ITIMER_REAL, &timerval, NULL);

    while (1) {
        pair.x = pair.y = counter % 2;
        if (++counter == INTERVAL) {
            if (output_allowed) {
                sprintf(mes, "Process Name: %s\n"
                             "Parent PID: %d\n"
                             "Process PID: %d\n"
                             "00 Count: %d\n"
                             "01 Count: %d\n"
                             "10 Count: %d\n"
                             "11 Count: %d\n\n", argv[0], getppid(), getpid(), statOO, statOl, statlO, statll);
                fputs(mes, stdout);
            }
            statOO = statOl = statlO = statll = counter = 0;
        }
    }

}