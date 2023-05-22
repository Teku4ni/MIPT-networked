#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include "socket_tools.h"
#include <thread>
#include <unistd.h>

void send_msg(int sfd, addrinfo res_addr)
{
  while (true)
  {
    std::thread sender([&]() {
      std::string msg;
      std::getline(std::cin, msg);
      ssize_t res = sendto(sfd, msg.c_str(), msg.size(), 0, res_addr.ai_addr, res_addr.ai_addrlen);
      if (res == -1)
        std::cout << strerror(errno) << '\n';
      printf("-> %s\n", msg.c_str());
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    sender.join();
  }
}

void listen_msg(int sfd)
{
  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
        printf("<- %s\n", buffer); // assume that buffer is a string
    }
  }
}

void keep_alive(int sfd, addrinfo res_addr)
{
  while (true)
  {
    std::string msg = "Staying alive!";
    ssize_t res = sendto(sfd, msg.c_str(), msg.size(), 0, res_addr.ai_addr, res_addr.ai_addrlen);
    if (res == -1)
      std::cout << strerror(errno) << '\n';
    sleep(10);
  }
}

int main(int argc, const char** argv)
{
  const char* port_send = "2022";
  const char* port_listen = "2023";
  addrinfo resAddrInfo;

  int sfd_send = create_dgram_socket("localhost", port_send, &resAddrInfo);
  if (sfd_send == -1)
  {
    printf("Cannot create socket!\n");
    return 1;
  }

  int sfd_listen = create_dgram_socket(nullptr, port_listen, nullptr);
  if (sfd_listen == -1)
    return 1;

  printf("Started listening. Type your message and press ENTER\n");

  std::thread t0(keep_alive, sfd_send, resAddrInfo);
  std::thread t1(send_msg, sfd_send, resAddrInfo);
  std::thread t2(listen_msg, sfd_listen);

  t0.join();
  t1.join();
  t2.join();

  return 0;
}