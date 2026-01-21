// HTTP server with epoll-based event handling and thread pool for concurrent connections

#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <sys/epoll.h>

#include <chrono>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "http_message.h"
#include "socket.h"
#include "uri.h"

namespace high_performance_server {

// Maximum HTTP message size per socket read/write operation
constexpr size_t kMaxBufferSize = 4096;

struct EventData {
  EventData() : file_descriptor(0), length(0), cursor(0), buffer() {}
  int file_descriptor;
  size_t length;
  size_t cursor;
  char buffer[kMaxBufferSize];
};

// A request handler should expect a request as argument and returns a response
using HttpRequestHandler_t = std::function<HttpResponse(const HttpRequest &)>;

// HTTP server with multi-threaded architecture:
// - Main thread for user interaction
// - Listener thread for accepting connections
// - Worker thread pool for processing requests
class HttpServer {
public:
  explicit HttpServer(const std::string &host, std::uint16_t port);
  ~HttpServer() = default;

  HttpServer() = default;
  HttpServer(HttpServer &&) = default;
  HttpServer &operator=(HttpServer &&) = default;

  void Start();
  void Stop();
  void RegisterHttpRequestHandler(const std::string &path, HttpMethod method,
                                  const HttpRequestHandler_t callback) {
    Uri uri(path);
    request_handlers_[uri].insert(std::make_pair(method, std::move(callback)));
  }
  void RegisterHttpRequestHandler(const Uri &uri, HttpMethod method,
                                  const HttpRequestHandler_t callback) {
    request_handlers_[uri].insert(std::make_pair(method, std::move(callback)));
  }

  bool running() const { return running_; }

private:
  static constexpr int kMaxEvents = 10000;
  static constexpr int kThreadPoolSize = 5;

  std::unique_ptr<Socket> socket_;
  bool running_;
  std::thread listener_thread_;
  std::thread worker_threads_[kThreadPoolSize];
  int worker_epoll_fd_[kThreadPoolSize];
  epoll_event worker_events_[kThreadPoolSize][kMaxEvents];
  std::map<Uri, std::map<HttpMethod, HttpRequestHandler_t>> request_handlers_;
  std::mt19937 random_generator_;
  std::uniform_int_distribution<int> sleep_times_;

  void SetUpEpoll();
  void Listen();
  void ProcessEvents(int worker_id);
  void HandleEpollEvent(int epoll_fd, EventData *event, std::uint32_t events);
  void HandleHttpData(const EventData &request, EventData *response);
  HttpResponse HandleHttpRequest(const HttpRequest &request);

  void control_epoll_event(int epoll_fd, int op, int fd,
                           std::uint32_t events = 0, void *data = nullptr);
};

} // namespace high_performance_server

#endif // HTTP_SERVER_H_
