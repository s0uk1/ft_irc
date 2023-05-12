#include "Client.hpp"

Client::Client()
{
    _nickname = "";
    _username = "";
    _isOperator = false;
}

Client::Client(const std::string &nickname, const std::string &username, bool isOperator)
{
    _nickname = nickname;
    _username = username;
    _isOperator = isOperator;
}

Client::~Client()
{
}

std::string Client::getNickname() const
{
    return _nickname;
}

std::string Client::getUsername() const
{
    return _username;
}

int Client::getFd() const
{
    return _fd;
}

void Client::setFd(int fd)
{
    _fd = fd;
}
bool Client::isOperator() const
{
    return _isOperator;
}

//
