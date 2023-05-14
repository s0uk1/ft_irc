#include "Server.hpp"
#include "Client.hpp"
//********************** - Exceptions - **********************//

const char *Server::InvalidSocketFd::what() const throw()
{
    return "Invalid socket file descriptor";
}

const char *Server::FcntlError::what() const throw()
{
    return "Error setting socket to non-blocking";
}

const char *Server::SetsockoptError::what() const throw()
{
    return "Error setting socket options";
}

const char *Server::BindError::what() const throw()
{
    return "Error binding socket";
}

const char *Server::ListenError::what() const throw()
{
    return "Error listening socket";
}

//********************** - Private methods - **********************//

void Server::removeClient(int client_fd)
{
    std::map<int, Client*>::iterator it = _clients.find(client_fd);
    if (it != _clients.end())
    {
        delete it->second;
        _clients.erase(it);
    }
}

void	Server::checkAndAuth(Client *clt)
{
	if (!clt->isAuthenticated() && !clt->getNickname().empty() && !clt->getUsername().empty() && clt->getClaimedPsswd() == this->_password)
	{
		clt->setAuthentication(true);
		std::cout<<clt->getNickname()<<" authenticated"<<std::endl;
		sendMessage(NULL, clt, 0, 1, "WELCOME TO THE BEST IRC SERVER MADE WITH LOVE BY SOUKI && SIXIE WE NAMED SOUKIXIE AS IT'S A TEAM WORK AND WE ARE THE BEST TEAM EVER");
		sendMessage(NULL, clt, 0, 2, "your host is " + this->_serverName +", running the ver 0.0.1");
		sendMessage(NULL, clt, 0, 3, "this server Was creates at " + this->creationDate );
		sendMessage(NULL, clt, 0, 4, "AHAHA this feels like HELLO WORLD");
	}
}

/// @brief returns the ip of the client if NULL is passed returns the server domain
/// @param clt the client

char *Server::getAddr(Client *clt)
{
	struct sockaddr_in _host;
	socklen_t	len;

	len = sizeof(struct sockaddr);
	getsockname(clt->getFd(), (sockaddr *)&_host, &len);
	return (inet_ntoa(_host.sin_addr));
}

/// @brief this function sends a message from a source to a destination
/// it automatically adds the message prefixes to the message depending
/// on the source and destination, refer to rfc1459 section 2.3.1
/// @param src the source of the message client object pointer if it's from the client
/// NULL if it's the server
/// @param dst the destination of the message it is the client object NULL if it's a channel
/// @param ERRCODE the ERRCODE corespondig to the error faced
/// @param message the pure message that wants to be sent from 
/// src to dst the prefixes will be added in the function.
void Server::sendMessage(Client *src, Client *dst, int ERRCODE, int RPLCODE ,std::string message)
{
	//we need to find a way for RPL
	std::string _host;
	char		*tmphost;

	if (!src)
		_host = "irc.soukixie.local";
	else
	{
		tmphost = getAddr(src);
		_host = tmphost;
		free(tmphost); //check if we are allowed to use it
	}
	// std::cout<<this->rplCodeToStr[RPLCODE]<<std::endl;
	if (!src && RPLCODE)
		message = ":" + _host + " " + this->rplCodeToStr[RPLCODE] + " " + dst->getNickname() + " :" + message + "\n\r";
	else if (!src && !ERRCODE)
		message = ":" + this->_serverName + "!" + this->_serverName +"@"+_host +" PRIVMSG " + dst->getNickname() + " :"+ message +"\n\r";
	else if (!ERRCODE && !RPLCODE)
		message = ":" + src->getNickname() + "!" + src->getUsername() +"@"+_host +" PRIVMSG " + dst->getNickname() + " :"+ message +"\n\r"; // works perfect for private messages can not send messages from server
	else
		message = ":" + this->_serverName + " ERROR " + this->errCodeToStr[ERRCODE] + " sixie :fuck you\n\r"; // still not working
	// send(dst->getFd(), "welcome t", message.length(), 0);
	send(dst->getFd(), message.c_str(), message.length(), 0);

}

void	Server::_userCommand(Client *client, std::vector<std::string> tokens)
{
	if (tokens.size() < 4)
    {
        // send error message to client
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS ,0,"NULL");
        return;
    }
	client->setUsername(tokens[1]);
	// sendMessage(NULL, client, 0, 0,"Username Set to " + tokens[1]);
	checkAndAuth(client);
}

void Server::_nickCommand(Client *client, std::vector<std::string> tokens)
{
	// we need to add more checks on the nickname as it has a format and length
    if (tokens.size() < 2)
    {
        // send error message to client
        sendMessage(NULL, client, ERR_NONICKNAMEGIVEN ,0,"NULL");
        return;
    }
	if (!nickAvailable(tokens[1]))
	{
		sendMessage(NULL, client, ERR_NICKNAMEINUSE, 0,"");
		return ;
	}
	client->setNickname(tokens[1]);
	// sendMessage(NULL, client, 0, 0, "Nickname Set to " + tokens[1]);
	checkAndAuth(client);
}

void	Server::_passCommand(Client *clt, std::vector<std::string> tokens)
{
	if (tokens.size() < 2)
    {
        // send error message to client
        sendMessage(NULL, clt, ERR_NEEDMOREPARAMS ,0,"NULL");
        return;
    }
	clt->setClaimedPsswd(tokens[1]);
	// sendMessage(NULL, clt, 0, 0, "PASSWD SET ");
	checkAndAuth(clt);
}

// Channel *Server::_findChannel(std::string channelName) const
// {
//     std::vector<Channel*>::const_iterator i;
//     for(i = _channels.cbegin(); i != _channels.cend(); i++)
//     {
//         if ((*i)->getChannelName() == channelName)
//                 return (*i);
//     }
//     return (NULL);
// }

// void    Server::findChannelAndSendMessage(Client *client, std::string channelName, std::string message)
// {
//     //gotta add a authorization check here
//     // if the user is not in the channel he cannot send a message to it
//     Channel *target = _findChannel(channelName); 
//     if (target)
//     {
//         std::vector<Client*> clients = target->getClients();
//         while (!clients.empty())
//         {
//             // sendMessage(clients.back()->getFd(), ":" + client->getNickname() + " PRIVMSG " + channelName + " :" + message + "\r\n");
//             clients.pop_back();
//         }
//     }
// }

// void    Server::findTargetsAndSendMessage(Client *client, std::vector<std::string> recipients, std::string message)
// {
//     //this function checks if the recipient is a channel of a user and sends the message to the appropriate function
//     while (!recipients.empty())
//     {
//         std::string recipient = recipients.back();
//         //find if recipient is a channel
//         if (recipient[0] == '#')
//             findChannelAndSendMessage(client, recipient, message);
//     }

// }

//PRIVMSG command 
// void Server::_privmsgCommand(Client *client, std::vector<std::string> tokens)
// {
//     //check number of parameters
//     if (tokens.size() < 3)
//     {
//         // sendMessage(client->getFd(), ":localhost 461 " + (client->getNickname().empty() ? "*" : client->getNickname()) + " " + tokens[1] +  " :Not enough parameters\r\n");
//         return;
//     }
//     //fetch target and message
//     std::string target = tokens[1];
//     std::string message = tokens[2];
//     std::vector<std::string> recipients;
//     //separate recoipients
//     if (target.find(',') != std::string::npos)
//     {
//         std::istringstream tokenStream(target);
//         std::string token;
//         while (std::getline(tokenStream, token, ','))
//         {
//             recipients.push_back(token);
//         }
//     }
//     else
//     {
//         recipients.push_back(target);
//     }
//     //send message to all recipients
//     //find targets and send message
//     findTargetsAndSendMessage(client, recipients, message);
// }

void Server::processCommand(Client *client, std::vector<std::string> tokens)
{
    //remove \n from last token if it exists
    if (!tokens.empty())
    {
        std::string& lastMessage = tokens.back();
        if (!lastMessage.empty() && lastMessage.back() == '\n')
        {
            lastMessage.pop_back();
        }
    }
    // std::cout << "--" << tokens[0] << "--" << std::endl;
    if (tokens.empty())
        return;
    const std::string &command = tokens[0];
    if (command == "NICK" || command == "nick")
        _nickCommand(client, tokens);
	else if (command == "USER" || command == "user")
		_userCommand(client, tokens);
	else if (command == "PASS" || command == "pass")
		_passCommand(client, tokens);
	
    // else if (command == "PRIVMSG" || command == "privmsg") // needs fixes
    //     _privmsgCommand(client, tokens);
}

std::string Server::normalizeLineEnding(std::string &str)
{
    std::string nstring = str;
    nstring.erase(std::remove(nstring.begin(), nstring.end(), '\r'), nstring.end());
    return nstring;
}

// parsing a single command from a client
void Server::parseCommand(Client *client, std::string &command)
{
    std::vector<std::string> tokens;
    std::string token;
    std::string nstring = normalizeLineEnding(command);
    std::istringstream tokenStream(nstring);
    //line ending is normalized  from windows style to unix style
    // and i then look for \n meaning that i have a full command
    if (nstring.find('\n') != std::string::npos && nstring.size() > 1)
    {
        // i split the command into tokens
        while (std::getline(tokenStream, token, ' '))
        {
            tokens.push_back(token);
        }
    }
    //process the command
    processCommand(client, tokens);
}

// void Server::handleClient(Client *client)
// {
//     std::cout << "Handling client " << client->getFd() << std::endl;
//     std::string message = readFromClient(client->getFd());
//     if (message.empty())
//         return;
//     parseCommand(client, message);
// }

std::string Server::readFromClient(int client_fd)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // std::cout << "Reading from client " << client_fd << std::endl;
    int len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	std::string message = buffer;
    if (len > 0)
    {
        std::cout << message;
        
    }
    else if (len == 0)
    {
        std::cout << "Client :" << client_fd << " disconnected" << std::endl;
        close(client_fd);
        removeClient(client_fd);
        return "";
    }
    // else
    // {
    //     std::cout << "Error reading from client " << client_fd << std::endl;
    //     close(client_fd);
    //     removeClient(client_fd);
    //     return "";
    // }
    return message;
}

void Server::initCode()
{
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOSUCHNICK, "401"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOSUCHCHANNEL, "403"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_CANNOTSENDTOCHAN, "404"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_TOOMANYCHANNELS, "405"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_WASNOSUCHNICK, "406"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_TOOMANYTARGETS, "407"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOORIGIN, "409"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NORECIPIENT, "411"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOTEXTTOSEND, "412"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOTOPLEVEL, "413"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_WILDTOPLEVEL, "414"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_UNKNOWNCOMMAND, "421"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_FILEERROR, "424"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NONICKNAMEGIVEN, "431"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_ERRONEUSNICKNAME, "432"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NICKNAMEINUSE, "433"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERNOTINCHANNEL, "441"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOTONCHANNEL, "442"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERONCHANNE, "443"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOLOGIN, "444"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NEEDMOREPARAMS, "461"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_ALREADYREGISTRED, "462"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOPERMFORHOST, "463"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_PASSWDMISMATCH, "464"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_YOUREBANNEDCREEP, "465"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_CHANNELISFULL, "471"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_BANNEDFROMCHAN, "474"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_BADCHANNELKEY, "475"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOPRIVILEGES, "481"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_CHANOPRIVSNEEDED, "482"));
	this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERSDONTMATCH, "502"));
	this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_WELCOME, "001"));
	this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_YOURHOST, "002"));
	this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_CREATED, "003"));
	this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_MYINFO, "004"));
}

void Server::InitSocket()
{
    struct sockaddr_in server_addr;

    this->_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
        throw InvalidSocketFd();

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        throw FcntlError();
        close(_fd);
    }
    int opt = 1;
    if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw SetsockoptError();
        close(_fd);
    }
    if (bind(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        throw BindError();
        close(_fd);
    }
    if (listen(_fd, 5) < 0)
    {
        throw ListenError();
        close(_fd);
    }
}

bool	Server::nickAvailable(std::string nick)
{
	return (!this->_nicknames[nick]);
}

//********************** - Constr destr and getters - **********************//
Server::Server() : _fd(-1), _port(-1), _running(false), _password("")
{
}

std::string	Server::getDate(void)
{
	time_t rawtime;
	char	*tmp;
	std::string	now;

  	time (&rawtime);
	tmp = ctime(&rawtime);
	now = tmp;
	// free(tmp);
	now = now.substr(0, now.size() - 1);
	return (now);
}

Server::Server(int port, const std::string &password) : _fd(-1), _port(port), _running(true), _password(password)
{

	this->_serverName = "soukixie"; //9 characters and a merge between our names ^_^
	this->creationDate =  this->getDate();
	this->_nicknames.insert(std::pair<std::string, Client*>(this->_serverName, new Client));
    this->InitSocket();
	this->initCode();
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct pollfd client_poll_fd;
    pollfd server_poll_fd;
    int client_fd;
	std::string rawMessage;

    server_poll_fd.fd = _fd;
    server_poll_fd.events = POLLIN;
    _fdsVector.push_back(server_poll_fd);
    while (this->_running == true)
    {
        if (poll(_fdsVector.data(), _fdsVector.size(), -1) < 0)
        {
            std::cout << RED << "Error polling" << RESET << std::endl;
            break;
        }
        if (_fdsVector[0].revents & POLLIN)
        {
            client_fd = accept(_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0)
            {
                std::cout << RED << "Error accepting client" << RESET << std::endl;
                break;
            }
            client_poll_fd.fd = client_fd;
            client_poll_fd.events = POLLIN;
            _fdsVector.push_back(client_poll_fd);
            _clients.insert(std::pair<int, Client *>(client_fd, new Client()));
			_clients[client_fd]->setConnection(true);
            // std::cout << GREEN << "New client connected" << RESET << std::endl;
        }
        for (size_t i = 1; i < _fdsVector.size(); i++)
        {
            client_fd = _fdsVector[i].fd; 
            if (_fdsVector[i].revents & POLLIN)
            {
			    // if (_clients[client_fd] && _clients[client_fd]->isConnected() && !_clients[client_fd]->isAuthenticated())
			    	// authenticateUser(client_fd);
                // std::cout << "Client " << client_fd << " is ready to read" << std::endl;
                Client *client = _clients[_fdsVector[i].fd];
                client->setFd(_fdsVector[i].fd);
				rawMessage = readFromClient(_fdsVector[i].fd);
				if (!rawMessage.empty())
					parseCommand(client, rawMessage);
                // handleClient(client);
            }
        }
    }
}



Server::~Server()
{
    close(_fd);
}

int Server::getFd() const
{
    return _fd;
}

int Server::getPort() const
{
    return _port;
}

std::string Server::getPassword() const
{
    return _password;
}

bool Server::isRunning() const
{
    return _running;
}

//example of the error handling
// :localhost 434 ssabbaji :Pass is not set

//rpl message format
//:server_name reply_code target :reply_message

//error message format
//:server_name ERROR error_code target :error_message

//privmsg format
//:sender_nick!sender_user@sender_host PRIVMSG target :message_text