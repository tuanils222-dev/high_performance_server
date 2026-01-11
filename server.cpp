#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_EVENTS 10000
#define BUFFER_SIZE 2048
#define MAX_CONNECTIONS 20000

// --- 1. CHUẨN BỊ NỘI DUNG TĨNH (PRE-CACHING) ---
std::string build_http_res(const std::string& body) {
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Connection: keep-alive\r\n"
           "Content-Length: " + std::to_string(body.length()) + "\r\n"
           "\r\n" + body;
}

const std::string RES_INDEX = build_http_res(
    "<html><head><style>body{font-family:sans-serif;text-align:center;padding-top:50px;}"
    ".btn{padding:10px 20px;background:#007bff;color:white;text-decoration:none;border-radius:5px;}</style></head>"
    "<body><h1>🚀 Trang Chủ</h1><p>Server C++ siêu tốc đang chạy!</p>"
    "<a href='/about' class='btn'>Xem trang Giới Thiệu</a></body></html>"
);

const std::string RES_ABOUT = build_http_res(
    "<html><head><style>body{font-family:sans-serif;text-align:center;padding-top:50px;}</style></head>"
    "<body><h1>ℹ️ Trang Giới Thiệu</h1><p>Server này sử dụng Linux Epoll và Non-blocking I/O.</p>"
    "<a href='/'>Quay lại Trang Chủ</a></body></html>"
);

const std::string RES_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";

// --- 2. LOGIC XỬ LÝ EVENT LOOP ---
void worker_thread() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, MAX_CONNECTIONS);
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    int epoll_fd = epoll_create1(0);
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    char buffer[BUFFER_SIZE];

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                while (true) {
                    int client_fd = accept(server_fd, NULL, NULL);
                    if (client_fd == -1) break;
                    fcntl(client_fd, F_SETFL, O_NONBLOCK);
                    int flag = 1;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
                    event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                }
            } else {
                int fd = events[i].data.fd;
                ssize_t count = read(fd, buffer, BUFFER_SIZE - 1);
                if (count <= 0) {
                    close(fd);
                } else {
                    buffer[count] = '\0';
                    // --- 3. ROUTING LOGIC ---
                    if (strstr(buffer, "GET / ")) {
                        send(fd, RES_INDEX.c_str(), RES_INDEX.length(), MSG_NOSIGNAL);
                    } else if (strstr(buffer, "GET /about ")) {
                        send(fd, RES_ABOUT.c_str(), RES_ABOUT.length(), MSG_NOSIGNAL);
                    } else {
                        send(fd, RES_404.c_str(), RES_404.length(), MSG_NOSIGNAL);
                    }
                }
            }
        }
    }
}

int main() {
    int cores = std::thread::hardware_concurrency();
    std::cout << "Server khoi tao voi " << cores << " luong tren port " << PORT << "..." << std::endl;
    std::vector<std::thread> threads;
    for(int i = 0; i < cores; i++) threads.emplace_back(worker_thread);
    for(auto& t : threads) t.join();
    return 0;
}
