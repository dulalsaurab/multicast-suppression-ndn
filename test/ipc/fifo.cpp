#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>

class _FIFO
{
    public:
        _FIFO(char* fifo_suppression_value, char* fifo_object_details)
        : m_fifo_suppression_value(fifo_suppression_value)
        , m_fifo_object_details(fifo_object_details)
        {
            // mknod(m_fifo_object_details, S_IFIFO | 0666, 0);
            mkfifo(m_fifo_suppression_value, 0666);
            mkfifo(m_fifo_object_details, 0666);
        }
public: 
    void fifo_write()
    {   
        std::cout << m_fifo_object_details << "reached here" << std::endl;
        auto fifo = open(m_fifo_object_details, O_RDWR);
        try
        {
            char* message = "this is the message";
            write(fifo, message, sizeof(message));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    }
    void fifo_read()
    {
        std::ifstream f(m_fifo_suppression_value);
        std::string line;
        getline(f, line);
        auto data_size = std::stoi(line);
        std::cout << "Size: " << data_size << std::endl;
        std::string data;
        {
            std::vector<char> buf(data_size);
            f.read(buf.data(), data_size);
            data.assign(buf.data(), buf.size());
        }
        if (!f.good())
        {
            std::cerr << "Read failed" << std::endl;
        }
        std::cout << "Data size: " << data.size() << " content: " << data << std::endl;
    }
private:
    char* m_fifo_suppression_value;
    char* m_fifo_object_details;
};

int main(int argc, char *argv[])
{
    char fifo_obj[] = "/tmp/fifo_suppression_value";
    char fifo_val[] = "/tmp/fifo_object_details";
     _FIFO _f(fifo_obj, fifo_val);
    _f.fifo_write();
}
