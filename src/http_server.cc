#include "http_server.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "http_message.h"
#include "uri.h"

namespace high_performance_server {

HttpServer::HttpServer(const std::string &host, std::uint16_t port)
    : running_(false), worker_epoll_fd_(),
      random_generator_(std::chrono::steady_clock::now().time_since_epoch().count()),
      sleep_times_(10, 100), socket_(std::make_unique<Socket>(host, port)) {}

void HttpServer::Start() {
  if (!socket_->Start()) {
    throw std::runtime_error("Failed to set socket");
  }

  SetUpEpoll();
  running_ = true;
  listener_thread_ = std::thread(&HttpServer::Listen, this);
  for (int i = 0; i < kThreadPoolSize; i++) {
    worker_threads_[i] = std::thread(&HttpServer::ProcessEvents, this, i);
  }
}

void HttpServer::Stop() {
  running_ = false;
  listener_thread_.join();
  for (int i = 0; i < kThreadPoolSize; i++) {
    worker_threads_[i].join();
  }
  for (int i = 0; i < kThreadPoolSize; i++) {
    close(worker_epoll_fd_[i]);
  }
  close(socket_->GetSocketFd());
}

void HttpServer::SetUpEpoll() {
  for (int i = 0; i < kThreadPoolSize; i++) {
    if ((worker_epoll_fd_[i] = epoll_create1(0)) < 0) {
      throw std::runtime_error(
          "Failed to create epoll file descriptor for worker");
    }
  }
}

void HttpServer::Listen() {
  EventData *client_data;
  sockaddr_in client_address;
  socklen_t client_len = sizeof(client_address);
  int client_fd;
  int current_worker = 0;
  bool active = true;

  while (running_) {
    if (!active) {
      std::this_thread::sleep_for(
          std::chrono::microseconds(sleep_times_(random_generator_)));
    }
    client_fd = accept4(socket_->GetSocketFd(), (sockaddr *)&client_address,
                        &client_len, SOCK_NONBLOCK);
    if (client_fd < 0) {
      active = false;
      continue;
    }

    active = true;
    client_data = new EventData();
    client_data->file_descriptor = client_fd;
    control_epoll_event(worker_epoll_fd_[current_worker], EPOLL_CTL_ADD,
                        client_fd, EPOLLIN, client_data);
    current_worker++;
    if (current_worker == HttpServer::kThreadPoolSize)
      current_worker = 0;
  }
}

void HttpServer::ProcessEvents(int worker_id) {
  EventData *data;
  int epoll_fd = worker_epoll_fd_[worker_id];
  bool active = true;

  while (running_) {
    if (!active) {
      std::this_thread::sleep_for(
          std::chrono::microseconds(sleep_times_(random_generator_)));
    }
    int num_events = epoll_wait(worker_epoll_fd_[worker_id],
                          worker_events_[worker_id], HttpServer::kMaxEvents, 0);
    if (num_events <= 0) {
      active = false;
      continue;
    }

    active = true;
    for (int i = 0; i < num_events; i++) {
      const epoll_event &current_event = worker_events_[worker_id][i];
      data = reinterpret_cast<EventData *>(current_event.data.ptr);
      if ((current_event.events & EPOLLHUP) ||
          (current_event.events & EPOLLERR)) {
        control_epoll_event(epoll_fd, EPOLL_CTL_DEL, data->file_descriptor);
        close(data->file_descriptor);
        delete data;
      } else if ((current_event.events == EPOLLIN) ||
                 (current_event.events == EPOLLOUT)) {
        HandleEpollEvent(epoll_fd, data, current_event.events);
      } else {
        control_epoll_event(epoll_fd, EPOLL_CTL_DEL, data->file_descriptor);
        close(data->file_descriptor);
        delete data;
      }
    }
  }
}

void HttpServer::HandleEpollEvent(int epoll_fd, EventData *data,
                                  std::uint32_t events) {
  int fd = data->file_descriptor;
  EventData *request, *response;

  if (events == EPOLLIN) {
    request = data;
    ssize_t byte_count = recv(fd, request->buffer, kMaxBufferSize, 0);
    if (byte_count > 0) {
      response = new EventData();
      response->file_descriptor = fd;
      HandleHttpData(*request, response);
      control_epoll_event(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
      delete request;
    } else if (byte_count == 0) {
      control_epoll_event(epoll_fd, EPOLL_CTL_DEL, fd);
      close(fd);
      delete request;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        request->file_descriptor = fd;
        control_epoll_event(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
      } else {
        control_epoll_event(epoll_fd, EPOLL_CTL_DEL, fd);
        close(fd);
        delete request;
      }
    }
  } else {
    response = data;
    ssize_t byte_count =
        send(fd, response->buffer + response->cursor, response->length, 0);
    if (byte_count >= 0) {
      if (byte_count < response->length) {
        response->cursor += byte_count;
        response->length -= byte_count;
        control_epoll_event(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
      } else {
        request = new EventData();
        request->file_descriptor = fd;
        control_epoll_event(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
        delete response;
      }
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        control_epoll_event(epoll_fd, EPOLL_CTL_ADD, fd, EPOLLOUT, response);
      } else {
        control_epoll_event(epoll_fd, EPOLL_CTL_DEL, fd);
        close(fd);
        delete response;
      }
    }
  }
}

void HttpServer::HandleHttpData(const EventData &raw_request,
                                EventData *raw_response) {
  std::string request_string(raw_request.buffer), response_string;
  HttpRequest http_request;
  HttpResponse http_response;

  try {
    http_request = string_to_request(request_string);
    http_response = HandleHttpRequest(http_request);
  } catch (const std::invalid_argument &e) {
    http_response = HttpResponse(HttpStatusCode::BadRequest);
    http_response.SetContent(e.what());
  } catch (const std::logic_error &e) {
    http_response = HttpResponse(HttpStatusCode::HttpVersionNotSupported);
    http_response.SetContent(e.what());
  } catch (const std::exception &e) {
    http_response = HttpResponse(HttpStatusCode::InternalServerError);
    http_response.SetContent(e.what());
  }

  response_string =
      to_string(http_response, http_request.method() != HttpMethod::HEAD);
  memcpy(raw_response->buffer, response_string.c_str(), kMaxBufferSize);
  raw_response->length = response_string.length();
}

HttpResponse HttpServer::HandleHttpRequest(const HttpRequest &request) {
  auto it = request_handlers_.find(request.uri());
  if (it == request_handlers_.end()) {
    return HttpResponse(HttpStatusCode::NotFound);
  }
  auto callback_it = it->second.find(request.method());
  if (callback_it == it->second.end()) {
    return HttpResponse(HttpStatusCode::MethodNotAllowed);
  }
  return callback_it->second(request);
}

void HttpServer::control_epoll_event(int epoll_fd, int op, int fd,
                                     std::uint32_t events, void *data) {
  if (op == EPOLL_CTL_DEL) {
    if (epoll_ctl(epoll_fd, op, fd, nullptr) < 0) {
      throw std::runtime_error("Failed to remove file descriptor");
    }
  } else {
    epoll_event ev;
    ev.events = events;
    ev.data.ptr = data;
    if (epoll_ctl(epoll_fd, op, fd, &ev) < 0) {
      throw std::runtime_error("Failed to add file descriptor");
    }
  }
}

} // namespace high_performance_server
