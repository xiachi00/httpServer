#include <string>
#include <unordered_map>
#include <ostream>


class httpRequest {
public:
    std::string method;                          // 请求方法
    std::string url;                             // 要请求的URL地址
    std::string type;
    std::unordered_map<std::string, std::string> args;     // 请求参数
    std::string version;                         // 版本协议号
    std::unordered_map<std::string, std::string> headers;  // 请求头
    std::string body;                            // 请求体
    void read_analysis(int &client_fd);
    friend std::ostream & operator << (std::ostream& os, const httpRequest &obj); // 只输出头
};

class httpResponse {
public:
    std::string version;                              // 版本号
    std::string status_code;                          // 状态码
    std::string status_msg;                           // 状态消息
    std::unordered_map<std::string, std::string> headers;  // 响应头
    std::string body;                                 // 响应体
    std::string to_string();
    static std::unordered_map<std::string, std::string> MIME;
    httpResponse();
    httpResponse(std::string &req_version, std::string &type_of_reqfile, std::string &content);
    friend std::ostream & operator << (std::ostream& os, const httpResponse &obj); // 只输出头
};

