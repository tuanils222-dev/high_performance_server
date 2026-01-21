
#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>
#include <string>

namespace high_performance_server {

class Socket {
public:
  Socket(const std::string &host, std::uint16_t port);
  ~Socket() = default;

  bool Start();

  int GetSocketFd() const;

private:
  std::string host_;

  std::uint16_t port_;

  int sock_fd_;
};

} // namespace high_performance_server

#endif // SOCKET_H
