#ifndef LIBEPOLL_HPP
#define LIBEPOLL_HPP
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <list>

#include "logger.hpp"
#include "connection.hpp"

class Epoller
{
private:
	struct epoll_event ev;
	struct epoll_event events[65535];
	int EventsNum;

	Logger log;
	int EpollFd;
	ConnectionList *connectionList;

public:
	Epoller(ConnectionList *connectionList);
	void process();
	~Epoller();
};

Epoller::Epoller(ConnectionList *conList)
{
	memset(&ev, 0, sizeof ev);
	EpollFd = epoll_create1(0);
	if (EpollFd < 0)
	{
		log(Logger::ERROR) << "Get epoll queue failed!" << std::endl;
		exit(1);
	}
	this->connectionList = conList;
}

Epoller::~Epoller()
{
}

void Epoller::process()
{
	//update connection
	if (!this->connectionList->empty())
	{
		if (this->connectionList->isReady())
		{
			int s = this->connectionList->ReadyList.size();
			for (int i = 0; i < s; i++)
			{
				//watch recv with ET mode
				if (connectionList->ReadyList.front()->status == listening)
				{
					ev.events = EPOLLIN;
				}
				else
				{
					ev.events = EPOLLIN | EPOLLET;
				}
				ev.data.fd = connectionList->ReadyList.front()->getFd();
				if (epoll_ctl(EpollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0)
				{
					log(Logger::ERROR) << "epoll set insertion error" << std::endl;
					//FIXME:what if get epoll fd failed?
				}
				this->connectionList->merge();
			}
			log << "merge connection list" << std::endl;
		}
	}
	else
	{
		log << "no connections" << std::endl;
	}
	Connection *cCon = NULL;
	//send and wait for recv
	while (!connectionList->SendList.empty())
	{
		cCon = connectionList->SendList.front();
		if (cCon->status == ready) //ready to send
		{
			//send
			int s = sizeof(cCon->SendBuffer->Front().c_str());
			int r = send(cCon->getFd(), cCon->SendBuffer->Front().c_str(), s, 0);
			//process result
			if (r == s)
			{
				//process buffer
				// log << "send " << r << " bytes" << std::endl;
				// log << "send: " << cCon->SendBuffer->Front() << std::endl;
				cCon->SendBuffer->Pop();
			}
			else if (r == 0)
			{
				log << "send 0" << std::endl;
			}
			else if (r < 0)
			{
				if (errno == EINTR)
					log << "send interrupted" << std::endl;
				if (errno == EAGAIN)
				{
					log << cCon->getFd() << "is not ready for writing anymore" << std::endl;
					r = 0; // Handled by ret2 < ret case
				}
				else
				{
					log(Logger::ERROR) << "send error" << std::endl;
					cCon->disconnect();
					return;
				}
			}
			else if (r < s) //Short write or EINTR at send
			{
				//process buffer
				cCon->SendBuffer->Front().erase(cCon->SendBuffer->Front().begin(), cCon->SendBuffer->Front().begin() + r);
			}
		}
		connectionList->SendList.pop_front();
	}

	EventsNum = epoll_wait(EpollFd, events, 65535, 0);

	if (EventsNum == -1)
	{
		if (errno != EAGAIN && errno != EINTR)
		{
			log(Logger::ERROR) << "epoll_pwait time out" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	int evt;
	for (int i = 0; i < EventsNum; ++i)
	{
		cCon = this->connectionList->queryfd(events[i].data.fd);
		evt = events[i].events;
		if (cCon->status == listening) //listening
		{
			log << "waiting to accept" << std::endl;
			this->connectionList->ReadyList.push_back(cCon->accept());
			log << "accepted" << std::endl;
		}
		else if (cCon->status > 0)
		{
			//since unlimited send buffer, this event is useless
			if (evt & EPOLLOUT) //when use EPOLLET mode, send util EAGAIN
			{
				log(Logger::ERROR) << "Unreachable event: EPOLLOUT" << std::endl;
			}
			if (evt & EPOLLIN) //when use EPOLLET mode, recv util EAGAIN
			{
				//recv
				char cbuffer[4096]; //4k
				std::string RecvLine;
				int r = 0;
				//process result
				do
				{
					r = recv(cCon->getFd(), cbuffer, sizeof(cbuffer), 0);
					if (r == -1 && RecvLine.size() == 0)
					{
						log(Logger::ERROR) << "recv error" << std::endl;
					}
					else if (r != -1)
					{
						RecvLine.append(cbuffer);
					}
				} while (r < 4096 && r != -1);
				//process recv buffer
				if (RecvLine.size() > 0)
				{
					cCon->RecvBuffer->Push(RecvLine);
					// log << "recv " << RecvLine.size() << " bytes" << std::endl;
					// log << "recv: " << RecvLine << std::endl;
					connectionList->RecvList.push_back(cCon);
				}
			}
			if (evt & (EPOLLERR | EPOLLHUP)) //Error or Hang up
			{
				cCon->disconnect();
			}
		}
		else
		{
			log(Logger::ERROR) << "Connection closed!" << std::endl;
		}
	}
}

#endif //LIBEPOLL_HPP