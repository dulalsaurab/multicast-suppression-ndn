#define MAX_BUF 1024
using namespace std;

char* strrev(char *str){
    int i = strlen(str)-1,j=0;

    char ch;
    while(i>j)
    {
        ch = str[i];
        str[i]= str[j];
        str[j] = ch;
        i--;
        j++;
    }
    return str;

}


int main()
{
    int fd;
    char *myfifo = "test-ipc";
    char buf[MAX_BUF];

    /* open, read, and display the message from the FIFO */
    fd = open(myfifo, O_RDONLY);
    read(fd, buf, MAX_BUF);
    cout<<"Received:"<< buf<<endl;
    cout<<"The reversed string is \n"<<strrev(buf)<<endl;
    close(fd);
    return 0;
}
