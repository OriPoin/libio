#include <iostream>
#include <vector>
#include "../lib/libnet.hpp"

class relaycli : public Service
{
private:
    Logger log;
    int port;
    std::string ip;
    int size = 4096;
    std::string teststr;
    int maxconpair = 256;
    int conedpair = 0;
    std::vector<int> lantency;
    bool LoopFrontHandler();
    bool MessageHandler(Connection *con);
    void init();

public:
    void setport(int p);
    void setsize(int s);
    void setmaxconpair(int s);
    void setip(std::string ip);
};

void relaycli::setmaxconpair(int c)
{
    this->maxconpair = c;
}

void relaycli::setsize(int s)
{
    this->size = s;
}
void relaycli::setport(int p)
{
    this->port = p;
}
void relaycli::setip(std::string ip)
{
    this->ip = ip;
}
bool relaycli::LoopFrontHandler()
{

    return true;
}

bool relaycli::MessageHandler(Connection *con)
{
    while (!con->RecvBuffer->Empty())
    {
        int id = con->getid();
        std::string str = this->recv(con);
        if (str == "ready")
        {
            if (id % 2 != 0)
            {
                this->send(con, teststr);
            }
            else if (id % 2 == 0)
            {
                this->send(this->connectionList->queryid(con->getid() - 1), teststr);
            }
        }
        else if (str.find("msg:") == 0)
        {
            std::chrono::system_clock::time_point tnow = std::chrono::system_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::microseconds>(tnow - con->timestamp).count();
            if (lantency.size() != 2000)
            {
                lantency.push_back(dt);
            }
            else if (lantency.size() == 2000)
            {
                int l = 0;
                for (int i = 0; i < 2000; i++)
                {
                    l += lantency.back();
                    lantency.pop_back();
                }
                l /= 2000;
                log << "lantency: " << l << " us" << std::endl;
            }
            this->send(this->connectionList->queryid(con->getid() - 1), teststr);
        }
    }
    con->timestamp = std::chrono::system_clock::now();
    return true;
}

void relaycli::init()
{
    Connection *con = NULL;
    while (conedpair != maxconpair)
    {
        con = this->connect(this->ip, this->port);
        if (con != NULL)
        {
            this->connectionList->setid(2 * conedpair + 1, con);
            std::string str = "ID:" + std::to_string(2 * conedpair + 1);
            this->send(con, str);
        }
        con = this->connect(this->ip, this->port);
        if (con != NULL)
        {
            this->connectionList->setid(2 * conedpair + 2, con);
            std::string str = "ID:" + std::to_string(2 * conedpair + 2);
            this->send(con, str);
        }
        conedpair++;
    }
    teststr = "msg:";
    for (int i = 0; i < this->size; i++)
    {
        teststr.append("0");
    }
}

int main(int argc, char **argv)
{
    relaycli server;
    int opt;
    while ((opt = getopt(argc, argv, "i:p:c:s:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            server.setip(argv[2]);
            break;
        case 'p':
            server.setport(atoi(argv[4]));
            break;
        case 'c':
            server.setmaxconpair(atoi(argv[6]));
            break;
        case 's':
            server.setsize(atoi(argv[8]));
            break;
        default:
            std::cout << "error" << std::endl;
            break;
        }
    }
    server.start();
    return 0;
}