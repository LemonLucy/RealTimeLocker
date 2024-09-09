#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 12345
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define NUM_LOCKERS 10 // 사물함 개수
#define MAX_ITEMS_PER_LOCKER 5

// 사물함 정보 구조체
typedef struct Item {
    char name[50];
    struct Item* next;
    struct Item* prev;
} Item;

typedef struct {
    char password[20];
    bool empty;
    char name[50];
    int locker_number;
    Item *items; 
    int item_count;
} Locker;

Locker lockers[NUM_LOCKERS];

void init_locker(Locker* locker){
    locker->empty = true;   
    locker->password[0] = '\0';
    locker->name[0] = '\0';
    locker->item_count = 0;
}
void initialize_lockers() {
    for (int i = 0; i < NUM_LOCKERS; i++) { // 사물함 10개 초기화
        lockers[i].locker_number = i + 1;
        init_locker(&lockers[i]);
    }
}

// Function to add item to the locker
bool add_item(int client_socket,Locker* locker) {
    bool success=false;
    char name[256];

    if (locker->item_count >= MAX_ITEMS_PER_LOCKER) {
        printf("Locker is full\n");
        send(client_socket, &success, sizeof(success), 0);
        return false;
    }
    success=true;
    send(client_socket, &success, sizeof(success), 0);
    if (recv(client_socket, name, sizeof(name), 0) <= 0) {
        perror("Failed to receive item name");
        return false;
    }

    // 새로운 아이템을 생성하고 이름을 설정
    Item* new_item = (Item*)malloc(sizeof(Item));
    if (new_item == NULL) {
        perror("Failed to allocate memory for new item");
        return false;
    }
    strcpy(new_item->name, name);
    new_item->next = NULL;
    new_item->prev = NULL;

    // 연결 리스트의 끝에 새로운 아이템을 추가
    if (locker->items == NULL) {
        // 리스트가 비어있는 경우
        locker->items = new_item;
    } else {
        // 리스트가 비어있지 않은 경우
        Item* current = locker->items;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_item;
        new_item->prev = current;
    }

    locker->item_count++;
    printf("Item added: %s to Locker %d\n", name, locker->locker_number);
    fflush(stdout);
    return true;
}

// Function to remove item from the locker
void remove_item(int client_socket,Locker* locker) {
    // 제거할 item갯수 받음
    int num_items;
    char name[256];
    recv(client_socket, &num_items, sizeof(num_items), 0);

    for (int i = 0; i < num_items; i++) {
        bool isSpace=false;
        if (locker->items == NULL) {
            printf("Locker is empty\n");
            send(client_socket, &isSpace, sizeof(isSpace), 0);
            break;
        }else{
            isSpace=true;
            send(client_socket, &isSpace, sizeof(isSpace), 0);

            recv(client_socket, name, sizeof(name), 0);
            Item* current = locker->items;

            bool found=false;
            while (current != NULL) {
                if (strcmp(current->name, name) == 0) {
                    // 아이템을 찾은 경우
                    found=true;
                    send(client_socket, &found, sizeof(found), 0);
                    if (current->prev != NULL) {
                        current->prev->next = current->next;
                    } else {
                        // 첫 번째 아이템인 경우
                        locker->items = current->next;
                    }
                    if (current->next != NULL) {
                        current->next->prev = current->prev;
                    }
                    free(current);
                    locker->item_count--;
                    printf("Item removed: %s from Locker %d\n", name, locker->locker_number);
                    fflush(stdout);
                    break;
                }
                current = current->next;
            }

            // 아이템을 찾지 못한 경우
            if(!found){
                printf("Item not found: %s\n", name);
                send(client_socket, &found, sizeof(found), 0);
                fflush(stdout);
            }
        }
    }
}
void clean_locker(Locker* locker) {
    Item* current = locker->items;
    while (current != NULL) {
        Item* to_delete = current;
        current = current->next;
        free(to_delete);
    }
    locker->items = NULL;
    locker->item_count = 0;
    printf("Locker %d cleaned.\n", locker->locker_number);
    fflush(stdout);
}
void view_contents(Locker* locker) {
    if (locker->item_count == 0) {
        printf("Locker %d is empty.\n", locker->locker_number);
        return;
    }

    printf("Contents of Locker %d:\n", locker->locker_number);
    Item* current = locker->items;
    while (current != NULL) {
        printf("- %s\n", current->name);
        current = current->next;
    }
    fflush(stdout);
    return;
}
void reset_password(int client_socket,Locker* locker){
    printf("Reset password\n");
    char new_password[20];

    // 클라이언트로부터 새로운 비밀번호 받기
    if (recv(client_socket, new_password, sizeof(new_password), 0) < 0) {
        perror("Receive failed");
        close(client_socket);
        pthread_exit(NULL);
    }
    // 비밀번호 업데이트
    strcpy(locker->password, new_password);
    printf("Password is reset to %s\n",new_password);
}

void send_locker_info(int client_socket, Locker* lockers) {
    char info[BUFFER_SIZE];
    memset(info, 0, BUFFER_SIZE);
    for (int i = 0; i < NUM_LOCKERS; i++) {
        if (lockers[i].empty) {
            sprintf(info + strlen(info), "Locker Number: %d - empty\n", lockers[i].locker_number);
        } else {
            sprintf(info + strlen(info), "Locker Number: %d - full\n", lockers[i].locker_number);
        }
    }
    if (send(client_socket, info, strlen(info), 0) < 0) {
            perror("Send failed");
            close(client_socket);
            pthread_exit(NULL);
    }
}

bool open_locker(int client_socket, int* client_locker_num, Locker* lockers) {
    char entered_name[256];
    char entered_pw[256];
    bool success = false;

    // 1. client로부터 이름을 받음
    if (recv(client_socket, entered_name, sizeof(entered_name), 0) < 0) {
        perror("Receive failed");
        close(client_socket);
        pthread_exit(NULL);
    }

    bool found = false;
    for (int i = 0; i < NUM_LOCKERS; i++) {
        // 1-1. 입력한 이름이 locker 시스템에 저장되어 있음
        if (strcmp(entered_name, lockers[i].name) == 0) {
            found = true;
            send(client_socket, &found, sizeof(found), 0);
            // locker number를 client에게 알려줌
            *client_locker_num = lockers[i].locker_number;
            if (send(client_socket, client_locker_num, sizeof(*client_locker_num), 0) < 0) {
                perror("Send failed");
                close(client_socket);
                pthread_exit(NULL);
            }

            // 2. client에게 password를 입력받음
            if (recv(client_socket, entered_pw, sizeof(entered_pw), 0) < 0) {
                perror("Receive failed");
                close(client_socket);
                pthread_exit(NULL);
            }
            // 3. 입력받은 password가 일치하면
            bool correct = false;
            if (strcmp(entered_pw, lockers[i].password) == 0) {
                correct = true;
                send(client_socket, &correct, sizeof(correct), 0);
                printf("Locker opened for %s\n", entered_name);
                return true;
            } else {
                correct = false;
                send(client_socket, &correct, sizeof(correct), 0);
                return false;
            }
        }
    }
    if (!found) {
        send(client_socket, &found, sizeof(found), 0);
        return false;
    }
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int command;

    printf("Client connected\n");

    // 클라이언트와 통신
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        // 클라이언트로부터 요청 받음
        if (recv(client_socket, &command, sizeof(command), 0) < 0) {
            perror("Receive failed");
            close(client_socket);
            pthread_exit(NULL);
        }
        // 요청 처리
        if (command == 1) {
            // 1. 사물함 정보 전송
            send_locker_info(client_socket, lockers);

            while (1) {
                // 4. 클라이언트가 선택한 사물함 번호 받음
                int selected_locker_num;
                if (recv(client_socket, &selected_locker_num, sizeof(selected_locker_num), 0) < 0) {
                    perror("Receive failed");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                // 5. 선택한 사물함 번호에 해당하는 사물함이 비었는지 확인하고 empty 여부 전송
                bool is_empty = lockers[selected_locker_num - 1].empty;

                if (send(client_socket, &is_empty, sizeof(is_empty), 0) < 0) {
                    perror("Send failed");
                    close(client_socket);
                    pthread_exit(NULL);
                }

                if (is_empty) {
                    // 6. 비었다면 클라이언트로부터 이름 받음
                    char name[50];
                    memset(name, 0, sizeof(name));
                    if (recv(client_socket, name, sizeof(name), 0) < 0) {
                        perror("Receive failed");
                        close(client_socket);
                        pthread_exit(NULL);
                    }
                    strcpy(lockers[selected_locker_num - 1].name, name);
                    // 7. 클라이언트로부터 패스워드 받음
                    char received_pw[20];
                    memset(received_pw, 0, sizeof(received_pw));
                    if (recv(client_socket, received_pw, sizeof(received_pw), 0) < 0) {
                        perror("Receive failed");
                        close(client_socket);
                        pthread_exit(NULL);
                    }
                    // 7-1. 패스워드 받은 후 사물함을 비어있지 않은 상태로 변경
                    strcpy(lockers[selected_locker_num - 1].password, received_pw);
                    lockers[selected_locker_num - 1].empty = false;
                    printf("password received.\n");
                    printf("<<Customer Info>>\n");
                    printf("Locker Number: %d\nName: %s\nPassword: %s\n", selected_locker_num, lockers[selected_locker_num - 1].name, lockers[selected_locker_num - 1].password);

                    // 8. add stuffs into locker
                    // item을 몇 개 받을지 client로부터 받음
                    int num_items;
                    recv(client_socket, &num_items, sizeof(num_items), 0);

                    for (int i = 0; i < num_items; i++) {
                        if (!add_item(client_socket,&lockers[selected_locker_num - 1])) {
                            printf("Failed to add item\n");
                            break;
                        }
                    }
                    view_contents(&lockers[selected_locker_num-1]);
                    break;
                } else { // 7-2. 비지 않은 사물함에 접근 => 4번으로 돌아가 다시 클라이언트에게 사물함 번호 받음
                    printf("Customer access to unavailable locker\n");
                }
            }
        } else if (command == 2 || command == 3||command==4) { // 사물함 열기
            int client_locker_num;

            if (open_locker(client_socket, &client_locker_num, lockers)) { // 고객의 정보 체크
                if (command == 2) {// manage stuff
                    printf("Manage client's stuff\n");
                    int stuff_command;
                    recv(client_socket, &stuff_command, sizeof(stuff_command), 0);

                    if (stuff_command == 1) { // 아이템을 추가
                        // 추가할 item갯수 받아옴
                        int num_items;
                        recv(client_socket, &num_items, sizeof(num_items), 0);

                        for (int i = 0; i < num_items; i++) {
                            if (!add_item(client_socket,&lockers[client_locker_num - 1])) {
                                printf("Failed to add item\n");
                                break;
                            }
                        }
                    } else if (stuff_command == 2) { // 아이템을 제거
                        remove_item(client_socket,&lockers[client_locker_num - 1]);
                    } 
                    view_contents(&lockers[client_locker_num-1]);
                }
                
                else if(command==3){//password reset
                    reset_password(client_socket,&lockers[client_locker_num - 1]);
                }
                else if (command == 4) {
                    // 클라이언트 종료
                    
                    clean_locker(&lockers[client_locker_num - 1]);
                    init_locker(&lockers[client_locker_num - 1]); //사물함 초기화
                    printf("Client disconnected\n");
                    close(client_socket);
                    pthread_exit(NULL);
                } else {
                    // 모든 케이스에도 속하지 않을 때
                    if (send(client_socket, "Invalid command.\n", strlen("Invalid command.\n"), 0) < 0) {
                        perror("Send failed");
                        close(client_socket);
                        pthread_exit(NULL);
                    }
                }
            }
        } 
    }
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 사물함 초기화
    initialize_lockers();

    // 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 바인딩
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket); // close the socket before exiting
        exit(EXIT_FAILURE);
    }

    // 리스닝
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket); // close the socket before exiting
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for clients...\n");

    while (1) {
        int* client_socket = malloc(sizeof(int));
        if (client_socket == NULL) {
            perror("Memory allocation failed");
            continue;
        }

        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_socket) != 0) {
            perror("Failed to create thread");
            free(client_socket);
        }
        pthread_detach(tid);
    }

    close(server_socket);
    return 0;
}
