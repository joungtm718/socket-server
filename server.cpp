#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

void handle_client(SOCKET client_socket, SOCKET other_client_socket) {
    char buffer[1024] = { 0 };
    while (true) {
        // 클라이언트로부터 데이터 수신
        int bytesReceived = recv(client_socket, buffer, 1024, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // 문자열 끝에 NULL 문자 추가
            std::cout << "Message from client: " << buffer << std::endl;

            // 다른 클라이언트에게 데이터 전송
            send(other_client_socket, buffer, bytesReceived, 0);
            std::cout << "Message sent to other client" << std::endl;
        }
        else if (bytesReceived == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // 소켓 종료
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    SOCKET server_fd, client_socket1, client_socket2;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 소켓 파일 디스크립터 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 소켓 옵션 설정
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        std::cerr << "Setsockopt failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 소켓을 해당 포트에 바인딩
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 연결 대기 상태로 설정
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 첫 번째 클라이언트 연결 수락
    if ((client_socket1 = accept(server_fd, (struct sockaddr*)&address, &addrlen)) == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    std::cout << "First client connected" << std::endl;

    // 두 번째 클라이언트 연결 수락
    if ((client_socket2 = accept(server_fd, (struct sockaddr*)&address, &addrlen)) == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    std::cout << "Second client connected" << std::endl;

    // 두 클라이언트를 처리하는 스레드 생성
    std::thread thread1(handle_client, client_socket1, client_socket2);
    std::thread thread2(handle_client, client_socket2, client_socket1);

    // 스레드가 종료될 때까지 대기
    thread1.join();
    thread2.join();

    // 서버 소켓 종료
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
