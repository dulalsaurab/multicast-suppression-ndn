//  #include "algorithm.hpp"
//  #include "common/logger.hpp"
// #include <vector>
// #include <sys/stat.h>
// #include <iostream>
// #include <unistd.h>
// #include <fcntl.h>
// #include <sstream>
// #include "common/global.hpp"


// -/* quick and dirty _FIFO*/
// class 
// -_FIFO::_FIFO(const char *fifo_suppression_value, const char *fifo_object_details)
// -  : m_fifo_suppression_value(fifo_suppression_value)
// -  , m_fifo_object_details(fifo_object_details)
// -  {
// -    // mknod(m_fifo_object_details, S_IFIFO | 0666, 0);
// -    mkfifo(m_fifo_suppression_value, 0666);
// -    mkfifo(m_fifo_object_details, 0666);
// -  }
// -
// -  void
// -  _FIFO::fifo_write(std::string  message, int duplicate)
// -  {
// -    auto fifo = open(m_fifo_object_details, O_RDWR);
// -    if (!fifo)
// -    {
// -      std::cout << "Couldn't open fifo file" << std::endl;
// -      return;
// -    }
// -    try {
// -        std::string message_dup = "|"+message + "|"+std::to_string(duplicate)+"|";
// -        std::cout << "This is the message to write: " << message_dup << std::endl;
// -        write(fifo, message_dup.c_str(), sizeof(message_dup));
// -        // write(fifo, dup.c_str(),  sizeof(dup));
// -      }
// -      catch (const std::exception &e) {
// -        std::cout << "Unfortunately came here" << std::endl;
// -        std::cerr << e.what() << '\n';
// -      }
// -      close(fifo);
// -  }
// -
// -  time::milliseconds
// -  _FIFO::fifo_read()
// -  {
// -    char buf[10];
// -    auto fifo = open(m_fifo_suppression_value, O_RDONLY);
// -    read(fifo, &buf, sizeof(char) * 10);
// -    std::cout << "Trying to get message from python" << buf << std::endl;
// -    int suppression_value = 0;
// -    std::string s (buf);
// -    std::string delimiter = "|";
// -    size_t pos = 0;
// -    std::string token;
// -    while ((pos = s.find(delimiter)) != std::string::npos)
// -    {
// -        token = s.substr(0, pos);
// -        s.erase(0, pos + delimiter.length());
// -    }
// -    try {
// -      suppression_value = std::stoi(token);
// -    } catch (std::exception const &e) {
// -      std::cout << "Couldnt convert char to string" << std::endl;
// -      // return time::milliseconds(suppression_value);
// -    }
// -    return  time::milliseconds(suppression_value);
// -  }
// -