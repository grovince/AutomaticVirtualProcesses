#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CONTAINERS 5
#define NAME_LEN 50

typedef struct {
    int id;
    char name[NAME_LEN];
    int running;      // 1: 실행 중, 0: 중지
    int cpu_limit;    // CPU 제한 (시뮬레이션)
    int mem_limit;    // 메모리 제한 (시뮬레이션)
    pid_t pid;        // 실제 프로세스 ID
} Container;

Container containers[MAX_CONTAINERS];
int container_count = 0;

void create_container(const char* name, int cpu_limit, int mem_limit) {
    if (container_count >= MAX_CONTAINERS) {
        printf("더 이상 컨테이너를 생성할 수 없습니다.\n");
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork 실패");
        return;
    }
    if (pid == 0) {
        execl("/bin/sleep", "sleep", "100", NULL);
        perror("exec 실패");
        exit(1);
    } else {
        containers[container_count].id = container_count + 1;
        strncpy(containers[container_count].name, name, NAME_LEN - 1);
        containers[container_count].name[NAME_LEN - 1] = '\0';
        containers[container_count].running = 1;
        containers[container_count].cpu_limit = cpu_limit;
        containers[container_count].mem_limit = mem_limit;
        containers[container_count].pid = pid;
        printf("컨테이너 '%s' (ID: %d, PID: %d) 생성 및 실행됨 (CPU 제한: %d, 메모리 제한: %d)\n",
               name, containers[container_count].id, pid, cpu_limit, mem_limit);
        container_count++;
    }
}

void delete_container(int id) {
    int found = 0;
    for (int i = 0; i < container_count; i++) {
        if (containers[i].id == id) {
            found = 1;
            if (containers[i].running) {
                kill(containers[i].pid, SIGKILL);
                waitpid(containers[i].pid, NULL, 0);
            }
            for (int j = i; j < container_count - 1; j++) {
                containers[j] = containers[j + 1];
            }
            container_count--;
            printf("컨테이너 ID %d 삭제됨\n", id);
            break;
        }
    }
    if (!found) {
        printf("ID %d인 컨테이너를 찾을 수 없습니다.\n", id);
    }
}

void list_containers() {
    if (container_count == 0) {
        printf("실행 중인 컨테이너가 없습니다.\n");
        return;
    }
    printf("%-4s\t%-8s\t    %-8s\t%-8s\t   %-8s \t%-8s\t\n", "ID", "이름", "상태", "CPU제한", "메모리", "PID");
    for (int i = 0; i < container_count; i++) {
        printf("%-4d\t %-8s %-7s \t  %-8d   %-8d \t%-8d\n",
            containers[i].id,
            containers[i].name,
            containers[i].running ? "Running" : "Stopped",
            containers[i].cpu_limit,
            containers[i].mem_limit,
            containers[i].pid);
    }
}

void stop_container(int id) {
    for (int i = 0; i < container_count; i++) {
        if (containers[i].id == id) {
            if (containers[i].running) {
                kill(containers[i].pid, SIGSTOP);
                containers[i].running = 0;
                printf("컨테이너 ID %d 중지됨\n", id);
            } else {
                printf("이미 중지된 컨테이너입니다.\n");
            }
            return;
        }
    }
    printf("ID %d인 컨테이너를 찾을 수 없습니다.\n", id);
}

void start_container(int id) {
    for (int i = 0; i < container_count; i++) {
        if (containers[i].id == id) {
            if (!containers[i].running) {
                kill(containers[i].pid, SIGCONT);
                containers[i].running = 1;
                printf("컨테이너 ID %d 시작됨\n", id);
            } else {
                printf("이미 실행 중인 컨테이너입니다.\n");
            }
            return;
        }
    }
    printf("ID %d인 컨테이너를 찾을 수 없습니다.\n", id);
}

int main() {
    char command[100];
    char arg1[NAME_LEN];
    int id, cpu_limit, mem_limit;

    printf("가상 컨테이너 관리 시스템에 오신 것을 환영합니다.\n");
    printf("명령어: create <name> <cpu_limit> <mem_limit>, delete <id>, list, stop <id>, start <id>, exit\n");

    while (1) {
        printf("> ");
        if (!fgets(command, sizeof(command), stdin)) break;

        if (sscanf(command, "create %49s %d %d", arg1, &cpu_limit, &mem_limit) == 3) {
            create_container(arg1, cpu_limit, mem_limit);
        } else if (sscanf(command, "delete %d", &id) == 1) {
            delete_container(id);
        } else if (strcmp(command, "list\n") == 0) {
            list_containers();
        } else if (sscanf(command, "stop %d", &id) == 1) {
            stop_container(id);
        } else if (sscanf(command, "start %d", &id) == 1) {
            start_container(id);
        } else if (strcmp(command, "exit\n") == 0) {
            printf("프로그램을 종료합니다.\n");
            break;
        } else {
            printf("알 수 없는 명령어입니다.\n");
        }
    }
    // 남아있는 자식 프로세스 종료
    for (int i = 0; i < container_count; i++) {
        if (containers[i].running) {
            kill(containers[i].pid, SIGKILL);
            waitpid(containers[i].pid, NULL, 0);
        }
    }
    return 0;
}
