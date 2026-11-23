#include "connection.hpp"

#include <unistd.h>

int GetProcessId()
{
	return getpid();
}

static BaseConnection Connection;

BaseConnection *BaseConnection::Create()
{
	return &Connection;
}

void BaseConnection::Destroy(BaseConnection *&c)
{
	if (c)
		c->Close();

	c = nullptr;
}

bool BaseConnection::Open()
{
	isOpen = false;
	return false;
}

bool BaseConnection::Close()
{
	bool wasOpen = isOpen;
	isOpen = false;
	return wasOpen;
}

bool BaseConnection::Write(const void *data, size_t length)
{
	(void)data;
	(void)length;
	return false;
}

bool BaseConnection::Read(void *data, size_t length)
{
	(void)data;
	(void)length;
	return false;
}
