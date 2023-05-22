#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <iostream>
#include "socket_tools.h"

int main(int argc, const char** argv)
{
  const char* port_listen = "2022";
  const char* port_send = "2023";

  addrinfo resAddrInfo;

  int sfd_send = create_dgram_socket("localhost", port_send, &resAddrInfo);
  if (sfd_send == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  int sfd_listen = create_dgram_socket(nullptr, port_listen, nullptr);
  if (sfd_listen == -1)
    return 1;
  
  printf("Started listening.\n");

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd_listen, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd_listen + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd_listen, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd_listen, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
      {
        printf("<- %s\n", buffer); // assume that buffer is a string
        std::string input = "get package";
        ssize_t res = sendto(sfd_send, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
        if (res == -1)
          std::cout << strerror(errno) << std::endl;
      }
    }
  }
  return 0;
}