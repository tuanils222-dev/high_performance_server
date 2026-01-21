#include "socket.h"

namespace {
constexpr int kBacklogSize = 1000;
} // namespace

namespace high_performance_server {

Socket::Socket(const std::string &host, std::uint16_t port)
    : host_(host), port_(port) {}

int Socket::GetSocketFd() const { return sock_fd_; }

bool Socket::Start() {
  int opt = 1;
  sockaddr_in server_address;

  if ((sock_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
    return false;
  }

  if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) < 0) {
    return false;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  inet_pton(AF_INET, host_.c_str(), &(server_address.sin_addr.s_addr));
  server_address.sin_port = htons(port_);

  if (bind(sock_fd_, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
    return false;
  }

  if (listen(sock_fd_, kBacklogSize) < 0) {
    // std::ostringstream msg;
    // msg << "Failed to listen on port " << port_;
    return false;
  }

  return true;
}

} // namespace high_performance_server