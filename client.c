#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 12345
#define BUFFER_SIZE 1024

// 클라이언트 명령 함수 선언
void send_command(int client_socket, int command);
void recv_locker_info(int client_socket);
void register_locker(int client_socket);
bool open_locker(int client_socket);
void add_items(int client_socket);
void remove_items(int client_socket);
void empty_locker(int client_socket);

// 사물함 정보 요청 함수
void recv_locker_info(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // 서버로부터 사물함 정보 수신
    if (recv(client_socket, buffer, sizeof(buffer), 0) < 0) {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }

    printf("Locker Information:\n%s\n", buffer);
}

void register_locker(int client_socket) {
    while (1) {
        int locker_num;
        printf("Enter locker number: ");
        scanf("%d", &locker_num);
        send(client_socket, &locker_num, sizeof(locker_num), 0);

        bool isEmpty;
        if (recv(client_socket, &isEmpty, sizeof(isEmpty), 0) < 0) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        if (isEmpty) {
            char name[50];
            char pw[20];
            printf("Locker is available!\n");

            printf("Enter your name: ");
            scanf("%s", name);
            if (send(client_socket, name, strlen(name) + 1, 0) < 0) {
                perror("Send failed");
                exit(EXIT_FAILURE);
            }

            printf("Enter password (less than 20 characters): ");
            scanf("%s", pw);
            if (send(client_socket, pw, strlen(pw) + 1, 0) < 0) {
                perror("Send failed");
                exit(EXIT_FAILURE);
            }
            break;
        } else {
            printf("This locker is unavailable. Choose another locker.\n");
        }
    }
}

bool open_locker(int client_socket) {
    char name[50];
    char pw[20];
    bool found;

    printf("Enter your name: ");
    scanf("%s", name);

    // 이름을 서버로 전송
    if (send(client_socket, name, strlen(name) + 1, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    // 이름 일치 여부 받음
    if (recv(client_socket, &found, sizeof(found), 0) < 0) {
        perror("Receive failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (found) {
        int locker_num;
        if (recv(client_socket, &locker_num, sizeof(locker_num), 0) < 0) {
            perror("Receive failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        printf("Your locker number is %d\n", locker_num);

        // 패스워드를 서버로 전송
        printf("Enter password: ");
        scanf("%s", pw);
        send(client_socket, pw, strlen(pw) + 1, 0);

        // 패스워드 일치 검사
        bool correct;
        if (recv(client_socket, &correct, sizeof(correct), 0) < 0) {
            perror("Receive failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        if (correct) {
            printf("Hurray! Your locker has been opened successfully!\n");
            return true;
        } else {
            printf("Open failed...Incorrect password\n");
            return false;
        }
    } else {
        printf("Name not found\n");
        return false;
    }
}

void add_items(int client_socket) {
    int num_items;
    printf("Enter the number of items to add: ");
    scanf("%d", &num_items);

    // 서버로 아이템 개수 전송
    send(client_socket, &num_items, sizeof(num_items), 0);

    for (int i = 0; i < num_items; i++) {
        bool isSpace;

        // 사물함에 공간이 있는지 수신
        if (recv(client_socket, &isSpace, sizeof(isSpace), 0) <= 0) {
            perror("Failed to receive space availability");
            break;
        }

        if (isSpace) {
            char item_name[256];
            printf("Enter the name of item %d: ", i + 1);
            scanf("%s", item_name);

            // 아이템 이름을 서버로 전송
            if (send(client_socket, item_name, strlen(item_name) + 1, 0) <= 0) {
                perror("Failed to send item name");
                break;
            }
            fflush(stdout);
        } else {
            printf("No space available in the locker.\n");
            break;
        }
    }
}

void remove_items(int client_socket) {
    int num_items;
    printf("Enter the number of items to remove: ");
    scanf("%d", &num_items);

    // 서버로 아이템 개수 전송
    send(client_socket, &num_items, sizeof(num_items), 0);

    for (int i = 0; i < num_items; i++) {
        bool isSpace;

        // 사물함에 공간이 있는지 수신
        if (recv(client_socket, &isSpace, sizeof(isSpace), 0) <= 0) {
            perror("Failed to receive space availability");
            break;
        }

        if (isSpace) {
            char item_name[256];
            printf("Enter the name of item %d to remove: ", i + 1);
            scanf("%s", item_name);

            // 아이템 이름을 서버로 전송
            if (send(client_socket, item_name, strlen(item_name) + 1, 0) <= 0) {
                perror("Failed to send item name");
                break;
            }

            //제거할 아이템 이름이 맞는지 수신
            bool found;
            if (recv(client_socket, &found, sizeof(found), 0) <= 0) {
                perror("Failed to receive space availability");
                break;
            }
            if (found) {
                printf("Item %s removed successfully.\n", item_name);
            } else {
                printf("**Item %s not found\n", item_name);
            }
        } else {
            printf("No space available in the locker.\n");
            break;
        }
    }
}

void empty_locker(int client_socket) {
    printf("Locker emptied successfully.\n");
}

void reset_password(int client_socket) {
    char new_password[20];

    printf("Enter new password: ");
    scanf("%s", new_password);

    // 서버로 새로운 비밀번호 전송
    if (send(client_socket, new_password, sizeof(new_password), 0) < 0) {
        perror("Send failed");
        return;
    }

    printf("Password reset successfully.\n");
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    // 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버의 IP 주소
    server_addr.sin_port = htons(PORT);

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");
    printf("Welcome to Van Gogh's museum!\n");

    // 클라이언트 명령 입력 및 서버로 전송
    while (1) {
        int command;
        printf("Enter command number (1: GET_LOCKER, 2: OPEN_LOCKER, 3: RESET PASSWORD, 4: EXIT): ");
        scanf("%d", &command);

        // 명령을 서버로 전송
        if (send(client_socket, &command, sizeof(command), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        // 명령에 따라 함수 호출
        switch (command) {
            case 1:
                recv_locker_info(client_socket);
                register_locker(client_socket);
                add_items(client_socket);
                break;
            case 2:
                printf("<<Manage your items>>\n");
                if(open_locker(client_socket)){
                    int sub_command;
                    printf("Enter command (1: ADD, 2: REMOVE, ANY OTHER NUMBER TO CANCEL): ");
                    scanf("%d", &sub_command);
                    send(client_socket, &sub_command, sizeof(sub_command), 0);
                    if (sub_command == 1) {
                        add_items(client_socket);
                    } else if (sub_command == 2) {
                        remove_items(client_socket);
                    } else {
                        printf("No changes made.\n");
                    }
                }
                break;
            case 3:
                printf("<<RESET PASSWORD>>\n");
                if(open_locker(client_socket)){
                    reset_password(client_socket);
                }
                break;
            case 4:
                if(open_locker(client_socket)){
                    empty_locker(client_socket);
                    printf("GOOD BYE!\n");
                    close(client_socket);
                    exit(EXIT_SUCCESS);
                }
                break;
            default:
                printf("Invalid command. Please enter 1, 2, 3, or 4.\n");
        }
    }

    return 0;
}
