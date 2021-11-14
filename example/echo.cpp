#include <iostream>
#include "../lib/libnet.hpp"

class echo : public Service
{
private:
    bool MessageHandler(Connection *con);
    void init();

public:
};

bool echo::MessageHandler(Connection *con)
{
    while (!con->RecvBuffer->Empty())
    {
        std::string str = con->RecvBuffer->Front();
        this->send(con, str);
        con->RecvBuffer->Pop();
    }
    return true;
}

void echo::init()
{
    this->listen("127.0.0.1", 20000);
}

int main(int argc, char const *argv[])
{
    echo server;
    server.start();
    return 0;
}