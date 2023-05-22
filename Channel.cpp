#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

Channel::Channel()
{
}

Channel::Channel(const std::string &name, const std::string &topic, const std::string &mode, int maxUsers)
{
    _name = name;
    _topic = topic;
    _mode = mode;
    _maxUsers = maxUsers;
    _isPrivate = false;
}

Channel::Channel(const std::string &name, Client *owner)
{
    _name = name;
    _topic = "";
    _mode = "";
    _maxUsers = 100;
    _isPrivate = false;
    _owner = owner;
}

bool Channel::isOnChannel(Client *client) const
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i] == client)
        {
            return true; 
        }
    }
    return false; 
}

bool Channel::isEmpty() const
{
    if (_clients.size() == 0)
        return true;
    return false;
}

bool Channel::isPrivate() const
{
    return _isPrivate;
}

void Channel::removeClient(Client *client)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i] == client)
        {
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
}

bool Channel::CheckOperator(Client *client)
{
    for (size_t i = 0; i < _operators.size(); i++)
    {
        if (_operators[i] == client->getNickname())
            return true;
    }
    return false;
}

bool    Channel::CheckMember(Client *client)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i] == client)
            return true;
    }
    return false;
}


void    Channel::SendJoinReplies(Client *client)
{
    // this->_server->sendMessage(NULL, client, 0, RPL_NAMREPLY, " = " + this->getChannelName() + " :" + this->getUsersList());
    // this->_server->sendMessage(NULL, client, 0, RPL_ENDOFNAMES, " " + this->getChannelName() + " :End of /NAMES list");
    std::string NamesReply = ":irc.soukixie.local 353 " + client->getNickname() + " = " + this->getChannelName() + " :" + this->getUsersList() + "\r\n";
    send(client->getFd(), NamesReply.c_str(), NamesReply.length(), 0);
    std::string EndOfNamesReply = ":irc.soukixie.local 366 " + client->getNickname() + " " + this->getChannelName() + " :End of /NAMES list" + "\r\n";
    send(client->getFd(), EndOfNamesReply.c_str(), EndOfNamesReply.length(), 0);
    if (this->getTopic() != "")
        this->_server->sendMessage(NULL, client, 0, RPL_TOPIC, " " + this->getChannelName() + " TOPIC :" + this->getTopic());
    // this->_server->sendMessage(NULL, client, RPL_MOTDSTART, 0, " :- " + this->getChannelName() + " Message of the day - ");
    // this->_server->sendMessage(NULL, client, RPL_TOPIC, 0, " " + this->getChannelName() + " :" + this->getTopic(
    // this->_server->sendMessage(NULL, client, RPL_MOTD, 0, " :- " + this->getChannelName() + "  " + this->getTopic());
    // this->_server->sendMessage(NULL, client, RPL_ENDOFMOTD, 0, " :End of MOTD command");
}

void    Channel::TheBootlegBroadcast(std::string message)
{
    for (size_t i = 0; i < this->getClients().size(); i++)
        send(this->getClients()[i]->getFd(), message.c_str(), message.length(), 0);
}


int Channel::getMemberCount() const
{
    return (_clients.size());
}

void Channel::CheckJoinErrors(Client *client, std::string password)
{
    if (this->getMode() == "i" && !this->isInvited(client))
    {
        this->_server->sendMessage(NULL, client, ERR_INVITEONLYCHAN, 0, " " + this->getChannelName() + " :Cannot join channel (+i)");
        return;
    }
    if (this->isBanned(client))
    {
        this->_server->sendMessage(NULL, client, ERR_BANNEDFROMCHAN, 0, " " + this->getChannelName() + " :Cannot join channel (+b)");
        return;
    }
    if (this->isFull())
    {
        this->_server->sendMessage(NULL, client, ERR_CHANNELISFULL, 0, " " + this->getChannelName() + " :Cannot join channel (+l)");
        return;
    }
    if (password != "" && password != this->getKey() && this->getMode() == "k")
    {
        this->_server->sendMessage(NULL, client, ERR_BADCHANNELKEY, 0, " " + this->getChannelName() + " :Cannot join channel (+k)");
        return;
    }
}

void    Channel::AddMember(Client *client, std::string password)
{
    //check if channel is invite mode and if client is invited
    CheckJoinErrors(client, password);
    if (this->isOnChannel(client))
    {
        this->_server->sendMessage(NULL, client, ERR_USERONCHANNEL, 0, " "+ this->getName() + " :is already on channel");
        return;
    }
    else 
    {
        _clients.push_back(client);
        //print clients on channel
        if (this->isEmpty())
        {
            this->setOperator(client);
            this->_owner = client;
        }
        BroadcastJoinMessage(client);
        this->SendJoinReplies(client);
    }
}

void    Channel::BroadcastJoinMessage(Client *client)
{
    std::string BroadcastMessage = ":" + client->getNickname() + "!~" + client->getUsername() + "@localhost" +  " JOIN :" + this->getChannelName() + "\r\n";
    std::vector<Client *> clients = this->getClients();
    for (size_t i = 0; i < clients.size(); i++)
    {
        Client *dst = clients[i];
        send(dst->getFd(), BroadcastMessage.c_str(), BroadcastMessage.length(), 0);
    }
}

std::string  Channel::getKey() const
{
    return _key;
}

bool Channel::isFull() const
{
    int size = _clients.size();
    if (size == _maxUsers)
        return true;
    return false;
}

std::string Channel::getUsersList() const
{
    std::string userList;
    for (size_t i = 0; i < _clients.size(); i++)
    {
        std::string name;
        if (_clients[i] == this->_owner)
            name = "@" + _clients[i]->getNickname();
        else
            name = _clients[i]->getNickname();
        userList += name + " ";
    }
    return userList;
}

bool Channel::isInvited(Client *client) const
{
    for (size_t i = 0; i < _invitedUsers.size(); i++)
    {
        if (_invitedUsers[i] == client->getNickname())
        {
            return true;
        }
    }
    return false;
}

bool Channel::isBanned(Client *client) const
{
    for (size_t i = 0; i < _bannedUsers.size(); i++)
    {
        if (_bannedUsers[i] == client->getNickname())
        {
            return true;
        }
    }
    return false;
}

void Channel::setOperator(Client *client)
{
    _operators.push_back(client->getNickname());
}


Channel::~Channel()
{
}

std::string Channel::getName() const
{
    return _name;
}

std::string Channel::getTopic() const
{
    return _topic;
}

std::string Channel::getMode() const
{
    return _mode;
}

int Channel::getMaxUsers() const
{
    return _maxUsers;
}

std::string Channel::getChannelName() const
{
    return _name;
}
std::vector<Client *> Channel::getClients() const
{
    return _clients;
}

std::vector<std::string> Channel::getBannedUsers() const
{
    return _bannedUsers;
}

std::vector<std::string> Channel::getOperators() const
{
    return _operators;
}

Client *Channel::getOwner() const
{
    return _owner;
}


//assignment operator overload
Channel &Channel::operator=(const Channel &rhs)
{
    if (this != &rhs)
    {
        this->_name = rhs._name;
        this->_topic = rhs._topic;
        this->_mode = rhs._mode;
        this->_maxUsers = rhs._maxUsers;
        this->_key = rhs._key;
        this->_owner = rhs._owner;
        this->_clients = rhs._clients;
        this->_bannedUsers = rhs._bannedUsers;
        this->_invitedUsers = rhs._invitedUsers;
        this->_operators = rhs._operators;
    }
    return *this;
}