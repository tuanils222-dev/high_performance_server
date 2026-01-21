# High Performance HTTP/1.1 Server

Implementation of a high performance HTTP server in C++

## Features

- Can handle multiple concurrent connections, tested up to 10k.
- Support basic HTTP request and response. Provide an extensible framework to implement other HTTP features.
- HTTP/1.1: Persistent connection is enabled by default.

## Quick start

```bash
mkdir build && cd build
cmake ..
make
./test_high_performance_server # Run unit tests
./high_performance_server          # Start the HTTP server on port 8080
```

- There are two endpoints available at `/` and `/welcome` which are created for demo purpose.
- In order to have multiple concurrent connections, make sure to raise the resource limit (with `ulimit`) before running the server. A non-root user by default can have about 1000 file descriptors opened, which corresponds to 1000 active clients.

## Design

The server program consists of:

- 1 main thread for user interaction.
- 1 listener thread to accept incoming clients.
- 5 worker threads to process HTTP requests and sends response back to client.
- Utility functions to parse and manipulate HTTP requests and repsonses conveniently.

## Benchmark

I used a tool called [wrk](https://github.com/wg/wrk) to benchmark this HTTP server. The tests were performed on my laptop with the following specs:

```bash
Model: Dell Pro 14 Essential PV14250
OS: Ubuntu 24.04.3 LTS x86_64
CPU: Intel 7 150U (12) @ 5.400GHz
GPU: Intel Graphics
Memory: 8386MiB / 15678MiB
```

Here are the results for two test runs. Each test ran for 1 minute, with 10 client threads. The first test had only 500 concurrent connections, while the second test had 10000.

```bash
$ wrk -t10 -c500 -d60s http://0.0.0.0:8080/
Running 1m test @ http://0.0.0.0:8080/
  10 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.97ms  671.43us  19.06ms   61.20%
    Req/Sec    25.44k     3.46k   37.56k    66.90%
  15188735 requests in 1.00m, 1.10GB read
Requests/sec: 253007.77
Transfer/sec:     18.82MB
```

```bash
$ wrk -t10 -c10000 -d60s http://0.0.0.0:8080/
Running 1m test @ http://0.0.0.0:8080/
  10 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    47.89ms   19.48ms 511.78ms   59.99%
    Req/Sec    20.85k     2.43k   30.79k    66.62%
  12446504 requests in 1.00m, 0.90GB read
Requests/sec: 207104.70
Transfer/sec:     15.41MB

```
