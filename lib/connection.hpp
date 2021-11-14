#ifndef CONNECTION_HPP
#define CONNECTION_HPP
#include <arpa/inet.h> //glibc inet_aton
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <unordered_map>

#include "logger.hpp"
#include "buffer.hpp"

enum ConnectionStatus
{
	listening = 0,
	connected = 1,
	ready = 2,
	closed = -1,
	pending = -2,
};

class Connection
{
private:
	bool SendFlag = false;
	bool ResvFlag = false;
	Logger log;
	u_int32_t ID = 0;
	int SocketFd;
	sockaddr *SocketAddr;
	struct InternetAddr
	{
		std::string ip;
		uint16_t port;
	} iAddr;

public:
	std::chrono::system_clock::time_point timestamp;
	Buffer *SendBuffer = NULL;
	Buffer *RecvBuffer = NULL;
	ConnectionStatus status = pending;
	Connection();
	Connection(InternetAddr iAddr, sockaddr *SocketAddr, int SocketFd);
	bool listen(std::string ip, uint16_t port);
	Connection *accept();
	bool connect(std::string ip, uint16_t port);
	int send(std::string SendLine);
	std::string recv();
	int disconnect();
	int getFd();
	int getid();
	void setid(int id);
};

Connection::Connection()
{
	this->SendBuffer = new Buffer();
	this->RecvBuffer = new Buffer();
}

Connection::Connection(InternetAddr iAddr, sockaddr *SocketAddr, int SocketFd)
{
	this->SendBuffer = new Buffer();
	this->RecvBuffer = new Buffer();
	this->iAddr = iAddr;
	this->SocketAddr = SocketAddr;
	this->SocketFd = SocketFd;
}

bool Connection::listen(std::string ip, uint16_t port)
{
	//Get Socket fd with non-blocking by default
	SocketFd = socket(PF_INET, SOCK_NONBLOCK | SOCK_STREAM, IPPROTO_TCP);
	//Log
	if (SocketFd == -1)
	{
		log(Logger::ERROR) << "Get socket failed!" << std::endl;
		exit(1);
	}
	else
	{
		log << "Get socket: " << SocketFd << std::endl;
	}
	//Convert ip and port
	iAddr.ip = ip;
	iAddr.port = port;
	struct sockaddr_in SocketAddress;
	memset(&SocketAddress, 0, sizeof(SocketAddress));
	SocketAddress.sin_family = PF_INET;
	SocketAddress.sin_port = htons(iAddr.port);
	inet_aton(ip.c_str(), &SocketAddress.sin_addr); // from glibc
	SocketAddr = (struct sockaddr *)&SocketAddress;
	int r = ::bind(SocketFd, SocketAddr, sizeof(sockaddr_in));
	//Log
	if (r < 0)
	{
		log(Logger::ERROR) << "Socket[" << SocketFd << "] bind at " << iAddr.ip << ":" << iAddr.port << " failed!" << std::endl;
		return false;
	}
	else
	{
		log << "Socket[" << SocketFd << "] bind at " << iAddr.ip << ":" << iAddr.port << std::endl;
		r = ::listen(SocketFd, 65535);
		//Log
		if (r < 0)
		{
			log(Logger::ERROR) << "Socket[" << SocketFd << "] listen to " << iAddr.ip << ":" << iAddr.port << " failed!" << std::endl;
			return false;
		}
		else
		{
			log << "Socket[" << SocketFd << "] listen to " << iAddr.ip << ":" << iAddr.port << std::endl;
			this->status = listening;
			return true;
		}
	}
}

Connection *Connection::accept()
{
	sockaddr *ConnectedSocketAddr = new sockaddr();
	socklen_t len = sizeof ConnectedSocketAddr;
	//accept socket with non-blocking by default
	int r = accept4(SocketFd, ConnectedSocketAddr, &len, SOCK_NONBLOCK);
	if (r < 0)
	{
		log(Logger::ERROR) << "Socket[" << SocketFd << "] accepting " << iAddr.ip << ":" << iAddr.port << " failed!" << std::endl;
	}
	else
	{
		log << "Socket[" << SocketFd << "] accepted: " << iAddr.ip << ":" << iAddr.port << std::endl;
	}
	Connection *con = new Connection(this->iAddr, ConnectedSocketAddr, r);
	con->status = connected;
	return con;
}

bool Connection::connect(std::string ip, uint16_t port)
{
	//Get Socket fd with non-blocking by default
	SocketFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//Log
	if (SocketFd == -1)
	{
		log(Logger::ERROR) << "Get socket failed!" << std::endl;
		exit(1);
	}
	else
	{
		log << "Get socket: " << SocketFd << std::endl;
	}
	//Convert ip and port
	iAddr.ip = ip;
	iAddr.port = port;
	struct sockaddr_in SocketAddress;
	memset(&SocketAddress, 0, sizeof(SocketAddress));
	SocketAddress.sin_family = PF_INET;
	SocketAddress.sin_port = htons(iAddr.port);
	inet_aton(ip.c_str(), &SocketAddress.sin_addr); // from glibc
	SocketAddr = (struct sockaddr *)&SocketAddress;
	int r = ::connect(this->SocketFd, this->SocketAddr, static_cast<socklen_t>(sizeof(struct sockaddr)));
	//connect should block when connect and then set socket non-block
	fcntl(SocketFd, F_SETFL, O_NONBLOCK);
	if (r < 0)
	{
		log(Logger::ERROR) << "Socket[" << SocketFd << "] connect to " << iAddr.ip << ":" << iAddr.port << " failed!" << std::endl;
		close(this->SocketFd);
		return false;
	}
	else
	{
		log << "Socket[" << SocketFd << "] connect to " << iAddr.ip << ":" << iAddr.port << std::endl;
		this->status = connected;
		return true;
	}
}

int Connection::send(std::string SendLine)
{
	this->SendBuffer->Push(SendLine);
	this->status = ready;
	return SendBuffer->Size();
}

//Unsafe
std::string Connection::recv()
{
	if (this->RecvBuffer->Empty())
	{
		return NULL;
	}
	else
	{
		std::string str = RecvBuffer->Front();
		RecvBuffer->Pop();
		return str;
	}
}

int Connection::disconnect()
{
	//if other side in error state
	close(SocketFd);
	return 0;
}

int inline Connection::getFd()
{
	return this->SocketFd;
}

inline int Connection::getid()
{
	return this->ID;
}
void Connection::setid(int id)
{
	this->ID = id;
}

class ConnectionList
{
public:
	std::unordered_map<int, Connection *> fdmap;
	std::unordered_map<int, Connection *> idmap;
	std::list<Connection *> MainList;
	std::list<Connection *> ReadyList;
	std::list<Connection *> RecvList;
	std::list<Connection *> SendList;
	Connection *queryfd(int fd);
	Connection *queryid(int id);
	void setid(int id, Connection *con);
	void merge();
	bool empty();
	int size();
	bool isReady();
};

bool ConnectionList::empty()
{
	if (this->MainList.empty() && this->ReadyList.empty())
	{
		return true;
	}
	return false;
}

bool inline ConnectionList::isReady()
{
	return !ReadyList.empty();
}

int inline ConnectionList::size()
{
	return MainList.size() + ReadyList.size();
}
void ConnectionList::merge()
{
	this->MainList.push_back(this->ReadyList.front());
	this->fdmap.insert(std::make_pair<int, Connection *&>(ReadyList.front()->getFd(), ReadyList.front()));
	this->ReadyList.pop_front();
}

inline Connection *ConnectionList::queryfd(int fd)
{
	return this->fdmap.find(fd)->second;
}

Connection *ConnectionList::queryid(int id)
{
	return this->idmap.find(id)->second;
}
void ConnectionList::setid(int id, Connection *con)
{
	con->setid(id);
	this->idmap.insert(std::make_pair<int &, Connection *&>(id, con));
}

#endif //CONNECTION_HPP