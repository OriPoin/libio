#ifndef LIBNET_HPP
#define LIBNET_HPP

#include <fstream>
#include <string>
#include <list>
#include <queue>
#include <time.h>

#include "logger.hpp"
#include "buffer.hpp"
#include "connection.hpp"

//If use epoll
#include "libepoll.hpp"

class Service
{
private:
	Logger log;
	enum ServiceStatus
	{
		EMPTY,
		READY,
		RUNNING,
		STOP
	};
	ServiceStatus status = EMPTY;
	Buffer SharedBuffer;
	//If use epoll
	Epoller *epoller;
	//init before start,only run once
	virtual void init();
	virtual bool LoopFrontHandler();
	virtual bool LoopBackHandler();
	virtual bool MessageHandler(Connection *con);
	void loop();

public:
	ConnectionList *connectionList;
	Service(/* args */);
	bool listen(uint16_t port);
	bool listen(std::string ip, uint16_t port);
	Connection *connect(std::string ip, uint16_t port);
	int send(Connection *con, std::string str);
	//send string to send buffer and add connection to activelist
	std::string recv(Connection *con);
	void start();
	void stop();
};

Service::Service(/* args */)
{
	this->connectionList = new ConnectionList();
	//If use epoll
	this->epoller = new Epoller(this->connectionList);
	//If use io_uring
}

bool Service::listen(std::string ip, uint16_t port)
{
	Connection *con = new Connection();
	if (con->listen(ip, port))
	{
		this->connectionList->ReadyList.push_back(con);
		//If use epoll
		epoller->process();
		//If use io_uring
		return true;
	}
	return false;
}

bool Service::listen(uint16_t port)
{
	return listen("127.0.0.1", port);
}

Connection *Service::connect(std::string ip, uint16_t port)
{
	Connection *con = new Connection();
	if (con->connect(ip, port))
	{
		this->connectionList->ReadyList.push_back(con);
		con->status = connected;
		return con;
	}
	return NULL;
}
void Service::init()
{
	// log << "No init" << std::endl;
}
bool Service::LoopFrontHandler()
{
	//log << "No loop front handler" << std::endl;
	return false;
}
bool Service::LoopBackHandler()
{
	//log << "No loop back handler" << std::endl;
	return false;
}
bool Service::MessageHandler(Connection *con)
{
	// log << "No message handler" << std::endl;
	return false;
}

void Service::start()
{
	this->init();
	// if (!connectionList->empty())
	// {
	this->status = READY;
	log << "Serive ready to run" << std::endl;
	this->status = RUNNING;
	this->loop();
	// }
	// else
	// {
	// 	log << "No Connections" << std::endl;
	// }
}

void Service::loop()
{
	//Infinity loop
	while (this->status == RUNNING)
	{
		/*
		Loop front handler
		need to process ready list and before read from activelist
		*/
		this->LoopFrontHandler();
		//Message handler
		if (!this->connectionList->MainList.empty())
		{
			int s = connectionList->RecvList.size();
			for (int i = 0; i < s; i++)
			{
				//message handler for every
				this->MessageHandler(connectionList->RecvList.front());
				//only pop out at here
				connectionList->RecvList.pop_front();
			}
		}
		/*
		Loop back handler
		write or process after reading from all connnections
		*/
		this->LoopBackHandler();
		//If use epoll
		epoller->process();
		//If use io_uring

		//Check status
		if (this->connectionList->empty())
		{
			this->status = EMPTY;
		}
	}
}

int Service::send(Connection *con, std::string str)
{
	con->send(str);
	this->connectionList->SendList.push_back(con);
	return con->SendBuffer->Size();
}

inline std::string Service::recv(Connection *con)
{
	return con->recv();
}

#endif //LIBNET_HPP