#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
using namespace std;

const int SERVER_PORT = 8000;  // 服务器端口号

int main(void) {
    int ret;


    // 创建服务器套接字描述符
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    // 清空服务器地址信息, 并且填入协议族, IP, 端口号
    bzero(&server_addr, sizeof server_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地所有IP地址, host to net
    server_addr.sin_port = htons(SERVER_PORT);

    // 实现 服务器信息 与 套接字 的绑定
    ret = bind(server_fd, (struct sockaddr*) &server_addr, sizeof server_addr);
    if (ret == -1) {
        cout << "Bind Error!" << endl;
        exit(-1);
    }
    cout << "Bind Succeeded!" << endl;

    // 服务器开始监听, 同时最多有10个
    listen(server_fd, 10);

    cout << "Waiting for connection from client......" << endl;

    // 开始接收
    while(true) {
        // 接受 客户端地址信息 与 客户端套接字描述符
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_fd < 0) {
            cout << "Connection Error!" << endl;
            break;
        }
        
        char client_ip[64];
        
        // 输出客户端的网络信息
        cout << "Client's IP:     " << 
                inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, 
                                   client_ip, sizeof(client_ip)) << endl;
        cout << "Client's PORT:   " << ntohs(client_addr.sin_port) << endl << endl;

        char buff[1024];
        // len为客户端发送数据的长度, read不会在buff后添加结束符
        // -1 是为了之后添加结束符
        // int len = read(client_fd, buff, sizeof(buff) - 1);
        // buff[len] = '\0';
        int len = recv(client_fd, buff, sizeof(buff) - 1, 0);
        buff[len] = '\0';

        if (len <= 0) {
            cout << "Receive Error" << endl;
        }

        int p = 0;
        for (int i = 0; i < len; i++) {
            if (buff[i] == '\n') {
                for (int j = p; j < i; j++) {
                    cout << buff[j];
                }
                p = i;
            }
        }

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Connection: close\r\n"; //close after transmission

        // Add the content-length field and the actual content
        response += "Content-Length: " + to_string(len) + "\r\n";
        response += "\r\n";
        // response += buff;
        cout << endl << buff << endl << endl;

        send(client_fd, response.c_str(), response.size(), 0);

        cout << response << endl;

        // write(client_fd, response.c_str(), response.size());



        close(client_fd);
    }
    close(server_fd);





    return 0;
}