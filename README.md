# High-Performance C++ HTTP Server

A lightweight, ultra-fast HTTP/1.1 web server built from scratch using C++ and the Linux System API. This project demonstrates advanced system programming techniques to achieve high concurrency and throughput.

## 🚀 Performance Highlights

Tested on a modern Linux environment with `wrk`:

* **Throughput:** **~141,000+ Requests Per Second (RPS)**.
* **Concurrency:** Successfully handles **10,000+ concurrent connections** (C10K).
* **Latency:** Average latency of **~33ms** under extreme 10k connection load.
* **Efficiency:** Minimal CPU and memory overhead due to a non-blocking event-driven design.

## 🛠 Technical Features

* **I/O Multiplexing:** Powered by **Linux `epoll**` in **Edge-Triggered (EPOLLET)** mode for  event notification.
* **Multi-threading:** Scalable architecture using `SO_REUSEPORT` to distribute incoming traffic across all available CPU cores.
* **Non-blocking I/O:** Every socket operation is non-blocking to prevent thread stalls.
* **HTTP/1.1 Support:** * Persistent connections (**Keep-Alive**) for reduced TCP handshake overhead.
* Static routing for multiple pages (Home, About).
* Accurate `Content-Length` and `Content-Type` header management.


* **Optimizations:**
* Disabled Nagle's algorithm (`TCP_NODELAY`) for low-latency response.
* Pre-cached HTTP responses to eliminate string manipulation overhead during hot paths.
* Zero-dependency: Built strictly using Linux System APIs and Standard C++11.



## 📁 Project Structure

* `server.cpp`: Main source code containing the event loop, thread pool, and routing logic.
* `README.md`: Project documentation and benchmark results.

## ⚡ Getting Started

### Prerequisites

* Linux Kernel 3.9 or later (required for `SO_REUSEPORT`).
* GCC/G++ compiler.

### Compilation

```bash
g++ -o server server.cpp

```

### Configuration

Before running, increase the system's open file limit to support high concurrency:

```bash
ulimit -n 65535

```

### Running the Server

```bash
./server

```

The server will start and listen on `http://localhost:8080`.

## 📊 Benchmarking

To reproduce the results, use `wrk`:

```bash
wrk -t4 -c10000 -d10s http://localhost:8080/

```

## 📝 Supported Routes

* `GET /` : Returns the home page with a navigation link.
* `GET /about` : Returns a simple about page.
* `Other` : Returns a standard `404 Not Found` response.
