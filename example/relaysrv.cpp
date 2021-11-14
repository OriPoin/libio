#include <iostream>
#include <unistd.h>
#include "../lib/libnet.hpp"

class relaysrv : public Service
{
private:
    int port;
    bool MessageHandler(Connection *con);
    void init();

public:
    void setport(int p);
};

void relaysrv::setport(int p)
{
    this->port = p;
}
bool relaysrv::MessageHandler(Connection *con)
{
    while (!con->RecvBuffer->Empty())
    {
        std::string str = this->recv(con);
        if (str.find("ID:") == 0)
        {
            str.erase(str.begin(), str.begin() + 3);
            this->connectionList->setid(std::stoi(str), con);
            this->send(con, "ready");
        }
        else if (str.find("msg:") == 0)
        {
            this->send(this->connectionList->queryid(con->getid() + 1), str);
        }
    }
    return true;
}

void relaysrv::init()
{
    this->listen("127.0.0.1", this->port);
}

int main(int argc, char **argv)
{
    relaysrv server;
    int opt = getopt(argc, argv, "p:");
    int port;
    switch (opt)
    {
    case 'p':
        port = atoi(argv[2]);
        server.setport(port);
        break;
    default:
        std::cout << "error" << std::endl;
        break;
    }
    server.start();
    return 0;
}