#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#define NUM_PROCESSES 5
#define WORK_ITERATIONS 100000000  

struct metrics {
    pid_t pid;
    double start_time;
    double end_time;
    double turnaround_time;
    double waiting_time;
    double response_time;
};

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main() {
    struct metrics m[NUM_PROCESSES];
    int pipefd[NUM_PROCESSES][2];

    printf("Launching %d CPU-bound processes...\n\n", NUM_PROCESSES);

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pipe(pipefd[i]);
        double launch_time = get_time_ms();
        pid_t pid = fork();

        if (pid == 0) {
            // Child
            close(pipefd[i][0]); // Close read end
            double start = get_time_ms();
            write(pipefd[i][1], &start, sizeof(start));
            close(pipefd[i][1]);

            volatile unsigned long counter = 0;
            for (unsigned long j = 0; j < WORK_ITERATIONS; j++) {
                counter += j;
            }
            exit(0);
        } else {
            // Parent
            close(pipefd[i][1]); // Close write end
            m[i].pid = pid;
            m[i].start_time = launch_time;
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(m[i].pid, NULL, 0);
        m[i].end_time = get_time_ms();

        double actual_start;
        read(pipefd[i][0], &actual_start, sizeof(actual_start));
        close(pipefd[i][0]);

        m[i].response_time = actual_start - m[i].start_time;
        m[i].turnaround_time = m[i].end_time - m[i].start_time;
        m[i].waiting_time = m[i].turnaround_time - (m[i].end_time - actual_start);
    }

    printf("PID\tTurnaround\tWaiting\t\tResponse (ms)\n");
    printf("------------------------------------------------------\n");

    double avg_turnaround = 0, avg_wait = 0, avg_response = 0;

    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("%d\t%.2f\t\t%.2f\t\t%.2f\n",
               m[i].pid,
               m[i].turnaround_time,
               m[i].waiting_time,
               m[i].response_time);

        avg_turnaround += m[i].turnaround_time;
        avg_wait += m[i].waiting_time;
        avg_response += m[i].response_time;
    }

    printf("\nAverage Turnaround Time: %.2f ms\n", avg_turnaround / NUM_PROCESSES);
    printf("Average Waiting Time: %.2f ms\n", avg_wait / NUM_PROCESSES);
    printf("Average Response Time: %.2f ms\n", avg_response / NUM_PROCESSES);

    return 0;
}
