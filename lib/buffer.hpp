#ifndef BUFFER_HPP
#define BUFFER_HPP
#include <string>
#include <list>
// Inherit from string queue
class Buffer
{
private:
	std::list<std::string> strbuffer;

public:
	std::string Front();
	void Push(std::string StringLine);
	void Pop();
	bool Empty();
	int Size();
};

std::string Buffer::Front()
{
	return strbuffer.front();
}

void Buffer::Push(std::string StringLine)
{
	strbuffer.push_back(StringLine);
}

void Buffer::Pop()
{
	strbuffer.pop_front();
}

bool Buffer::Empty()
{
	return strbuffer.empty();
}

int Buffer::Size()
{
	return strbuffer.size();
}

#endif //BUFFER_HPP