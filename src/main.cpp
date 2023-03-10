#include "Req_Rsp.h"
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
#include <ostream>
#include <errno.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
using namespace std;

const int SERVER_PORT = 8888;  // 服务器端口号
const int BROAD_SIZE = 225;
const string END = "\r\n";

int cnt;
vector<vector<short>> s;

void perror_exit(const char *description);
int start_Listen();
string cat_text_file(string path);
string cat_binary_file(string path);
string get_content(httpRequest &req);
bool judge(short x, short y, short player);
int main(int argc, char *args[]) {
    s = vector<vector<short>>(50, vector<short>(50));

    int ret, len;
    
    // 创建服务器套接字bind并且listen
    int server_fd = start_Listen();

    // 连接完成后 开始接收请求并响应请求
    while(true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len;
        client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_fd < 0) perror_exit("Accept Error");
        char client_ip[64];
        // 输出客户端的网络信息
        cout << "Client's IP:     " << 
                inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, 
                                   client_ip, sizeof(client_ip)) << endl;
        cout << "Client's PORT:   " << ntohs(client_addr.sin_port) << endl << endl;

        
        httpRequest req;
        req.read_analysis(client_fd);
        string content = get_content(req);
        
        // cout << req << endl;


        httpResponse rsp(req.version, req.type, content);
        // cout << rsp << endl;

        string msg = rsp.to_string();
        len = send(client_fd, msg.c_str(), msg.size(), 0);
        msg.clear();
        if (len <= 0) {
            cout << "Send Error!" << endl;
        } else {
            cout << "Send Succeeded!" << endl;
        }

        cout << "--------------------------------------------------------------------------------------------" << endl;
        
        close(client_fd);
    }
    close(server_fd);

    return 0;
}

httpResponse::httpResponse(string &req_version, string &type_of_reqfile, string &content) {
    version = req_version;
    status_code = "200";
    status_msg = "OK";
    headers.clear();
    headers["Content-Type"] = httpResponse::MIME[type_of_reqfile];
    body = content;
}

unordered_map<std::string, std::string> httpResponse::MIME = {
    { ".html", "text/html;charset=utf-8" },
    { ".js", "text/javascript" },
    { ".txt", "text/plain" },
    { ".json", "application/json" },
    { ".xml", "application/xml" },
    { ".jpg", "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".png", "image/png" },
    { ".ico", "image/x-icon" },
    { ".mp3", "audio/mpeg" },
    { ".mp4", "video/mp4" },
    { "data", "0" }
};

void perror_exit(const char *description) {
    perror(description);
    exit(-1);
}

int start_Listen() {
    int ret;
    // 创建服务器套接字描述符
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) perror_exit("Server Socket Error");
    cout << "Server Socket Succeeded!" << endl;

    struct sockaddr_in server_addr;
    // 清空服务器地址信息, 并且填入协议族, IP, 端口号
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地所有IP地址, host to net
    server_addr.sin_port = htons(SERVER_PORT);

    // 实现 服务器信息 与 套接字 的绑定
    ret = bind(server_fd, (struct sockaddr*) &server_addr, sizeof server_addr);
    if (ret == -1) perror_exit("Bind Error");
    cout << "Bind Succeeded!" << endl;

    // 服务器开始监听, 同时最多有10个
    ret = listen(server_fd, 10);
    if (ret == -1) perror_exit("Listen Error");

    cout << "Waiting for connection from client......" << endl;
    cout << "=======================================================================================================" << endl;

    return server_fd;
}

// httpRequest 输出请求头
ostream & operator << (ostream& os, const httpRequest &obj) {
    os << "Method: " << obj.method << endl;
    os <<  "Url: " << obj.url << endl;
    os <<  "Type: " << obj.type << endl;
    os << "Version: " << obj.version << endl;
    os << "Args: " << endl;
    for (auto &p : obj.args) {
        os << "     " << p.first << ": " << p.second << endl;
    }
    os << "Headers: " << endl;
    for (auto &p : obj.headers) {
        os << "     " << p.first << ": " << p.second << endl;
    }
    return os;
}

// httpResponse 输出响应头
ostream & operator << (ostream& os, const httpResponse &obj) {
    os << "Version: " << obj.version << endl;
    os << "status_code: " << obj.status_code << endl;
    os << "status_msg: " << obj.status_msg << endl;
    os << "Headers: " << endl;
    for (auto &p : obj.headers) {
        os << "     " << p.first << ": " << p.second << endl;
    }
    return os;
}


void Format(string &s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.empty()) return;
    size_t pos = 0;
    while (pos < s.size() && (s[pos] == '\n' || s[pos] == '\r')) pos += 1;
    s = s.substr(pos);
}

vector<string> split(string &str, char c) { // c 为分隔符
    stringstream ss(str);
    vector<string> res;
    string item;
    while (getline(ss, item, c)) res.push_back(item);
    return res;
}

void httpRequest::read_analysis(int &client_fd) {
    cout << "ENTER read_analysis" << endl;
    // char* buff = new char[10240];
    char* buff = new char[10240];
    // int len = read(client_fd, buff, sizeof(buff) - 1);
    int len = read(client_fd, buff, 10240 - 1);
    buff[len] = '\0';
    if (len <= 0) perror_exit("Read Error");
    stringstream sline(buff);
    string line;
    
    //  处理请求行
    getline(sline, line), Format(line);
    vector<string> first_line = split(line, ' ');
    method = first_line[0];                                // method
    version = first_line[2];                               // version
    vector<string> url_args = split(first_line[1], '?');
    url = url_args[0];                                     // url
    if (url == "/") url = "/index.html";
    size_t pos = url.find('.');
    if (pos != string::npos) {
        type = url.substr(pos, url.size() - pos);          // type
    } else {
        type = "data";
    }
    vector<string> args = split(url_args[1], '&');
    vector<string> key_value;                                  
    for (auto &item : args) {
        key_value = split(item, '=');
        key_value.resize(2);
        this->args[key_value[0]] = key_value[1];           // args[key] = value
        key_value.clear();
    }

    //  处理请求头
    while (getline(sline, line)) {
        Format(line);
        if (line == "") break;
        key_value = split(line, ':');
        key_value.resize(2);
        headers[key_value[0]] = key_value[1];              // headers[key] = value
        key_value.clear();
    }

    body = "";
    while (getline(sline, line)) {
        body += line;
    }
    first_line.clear();
    url_args.clear();
    args.clear();
    key_value.clear();
    delete[] buff;
}

string httpResponse::to_string() {
    cout << "ENTER to_string" << endl;
    string result = "";
    result += version + ' ' + status_code + ' ' + status_msg + END;
    for (auto p : headers) result += p.first + ": " + p.second + END;
    result += END;         //  响应行与响应体之间的空行
    result += body + END;
    return result;
}

string cat_text_file(string path) {
    string filename = "../resource" + path;
    cout << "Request for " << filename << endl;
    string line, content = "";
    ifstream fs(filename);
    if (fs.is_open()) {
        while (getline(fs, line)) {
            content += line + '\n';
        }
        fs.close();
    } else {
        fs.close();
        cout << "Error: Cannot open html file" << endl;
        exit(-1);
    }
    return content;
}

string cat_binary_file(string path) {
    string filename = "../resource" + path;
    cout << "Request for " << filename << endl;
    string line, content = "";
    ifstream fs(filename, ios::in|ios::binary|ios::ate);
    if (fs) {
        streampos len = fs.tellg();
        content.resize(len);
        fs.seekg(0, ios::beg);
        fs.read(&content[0], len);
        fs.close();
        return content;
    } else {
        cout << "Error: Cannot open file" << endl;
        fs.close();
        exit(-1);
    }
}

string get_content(httpRequest &req) {
    cout << "ENTER get_content" << endl;
    string content = "";
    if (req.type == ".html") {           
        if (req.url == "/index.html") {
            cnt = 0;
            for (auto &v : s)
                for (auto &x : v)
                    x = 0;
        }
        content = cat_text_file(req.url);
    } else if (req.type == ".ico" || req.type == ".jpg" || req.type == ".jpeg" || req.type == ".png") {
        content = cat_binary_file(req.url);
    } else if (req.type == "data") {
        short x, y, player;
        cnt += 1;
        for (auto &p : req.args) {
            if (p.first == "x") {
                x = stoi(p.second);
            } else if (p.first == "y") {
                y = stoi(p.second);
            } else {
                player = stoi(p.second);
            }
        }
        if (judge(x, y, player) || cnt == BROAD_SIZE) {
            req.url = "/reset_the_game.js";
            req.type = ".js";
            cout << "GAME OVER!" << endl;
            content = cat_text_file(req.url);
        }
    } else {
        cout << "Cannot recognize the type of the requested file!" << endl;
        exit(-1);
    }
    content += END;
    return content;
}

bool judge(short x, short y, short player) {
    cout << "ENTER JUDGE" << endl;
    s[x][y] = player;
    short L_R, U_D, LU_RD, LD_RU, i, j;
    short lx, ly, mx, my;
    lx = ly = 10000;
    mx = my = -10000;
    L_R = U_D = LU_RD = LD_RU = 1;

    for (j = y - 1; s[x][j] == player; j--) L_R += 1;        
    for (j = y + 1; s[x][j] == player; j++) L_R += 1;
    mx = max(mx, x), my = max(my, y);
    lx = min(lx, x), ly = min(ly, y);

    for (i = x - 1; s[i][y] == player; i--) U_D += 1;
    for (i = x + 1; s[i][y] == player; i++) U_D += 1;

    for (i = x - 1, j = y - 1; s[i][j] == player; i--, j--) LU_RD += 1;
    for (i = x + 1, j = y + 1; s[i][j] == player; i++, j++) LU_RD += 1;

    for (i = x + 1, j = y - 1; s[i][j] == player; i++, j--) LD_RU += 1;
    for (i = x - 1, j = y + 1; s[i][j] == player; i--, j++) LD_RU += 1;

    // if (lx < 0 || ly < 0 || mx >= 50 || my >= 50) {
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    //     cout << "ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRR\n";
    // }

    for (int i = 0; i < 19; i++) cout << "--";
    cout << endl;
    for (int x = 10; x <= 26; x++) {
        cout << "| ";
        for (int y = 10; y <= 26; y++)
            cout << (s[x][y] == 0 ? ' ' : "OX"[s[x][y] == 2]) << ' ';
        cout << " |" << endl;
    }
    for (int i = 0; i < 19; i++) cout << "--";
    cout << endl;
        

    cout << "Player" << player  << ' ' << x << ' ' << y << endl;
    cout << "MaxLength: " << max({L_R, U_D, LU_RD, LD_RU}) << endl;
    return max({L_R, U_D, LU_RD, LD_RU}) >= 5;
}