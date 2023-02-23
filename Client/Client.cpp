#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>





std::string parse(std::string packet, std::string mark)
{
	if (packet.find(mark) == std::string::npos)
		return "";
	return packet.substr(packet.find("{", packet.find(mark)) + 1, packet.find("}", packet.find("{", packet.find(mark))) - packet.find("{", packet.find(mark)) - 1);
}

std::string findPrivateUserName(std::string text)
{
	if (text.size() > 0 && text[0] == '@' && text.find(" ") != std::string::npos)
	{
		return text.substr(1, text.find(" ") - 1);
	}

	return "";
}


struct Message
{
	std::string _nickName;
	std::string _text;
	int _numberOnServer;

	Message(std::string nickName, std::string text, int numberOnServer) : _nickName(nickName), _text(text), _numberOnServer(numberOnServer) {}
};

struct Request
{
	std::string _method;
	std::string _login;
	std::string _passwd;
	std::string _lastClientMessage;
	std::string _nickName;
	std::string _text;
	std::string _privateUser;

	Request(std::string method, std::string login, std::string passwd, std::string lastClientMessage, std::string nickName = "", std::string text = "", std::string privateUser = "")
		: _method(method), _login(login), _passwd(passwd), _lastClientMessage(lastClientMessage), _nickName(nickName), _text(text), _privateUser(privateUser) {}


	std::string toString()
	{
		std::string tmp;
		tmp = "method{" + _method + "}" + "login{" + _login + "}" + "passwd{" + _passwd + "}" + "lastClientMessage{" + _lastClientMessage + "}"
			+ "nickName{" + _nickName + "}" + "text{" + _text + "}" + "privateUser{" + _privateUser + "}";
		return tmp;
	}
};


struct Response
{
	std::string _status;
	std::string _owner;
	std::string _data;
	int _messageCount;


	Response(std::string packet, int bytes)
	{

		packet.resize(bytes);

		_status = parse(packet, "status");
		_owner = parse(packet, "owner");
		_data = parse(packet, "data");

		try
		{
			_messageCount = stoi(parse(packet, "messageCount"));
		}
		catch (std::invalid_argument& e)
		{
			_messageCount = 0;
		}
		catch (std::out_of_range& e)
		{
			_messageCount = 0;
		}
	}



	Message toMessage()
	{
		return Message(_owner, _data, _messageCount);
	}

};



std::string dialogue(std::string context)
{
	while (true)
	{
		std::string tmp;
		std::cout << context << std::endl;
		std::getline(std::cin, tmp);

		if (tmp.size() > 0 && tmp.find("{") == std::string::npos && tmp.find("}") == std::string::npos)
			return tmp;

		std::cout << "Wrong input!!!" << std::endl;
	}
}

struct Client
{
	std::string _login;
	std::string _passwd;
	std::string _nickName;
	std::vector<Message> _messages;
	int _lastMessage = 0;



	Response registration(SOCKET ConnectSoc)
	{
		std::string outputBuffer;
		char inputBuffer[1024];
		int bytes;

		_login = dialogue("Enter your login");
		_passwd = dialogue("Enter your passwd");
		_nickName = dialogue("Enter your nickName");

		outputBuffer = Request("reg", _login, _passwd, "0", _nickName).toString();
		send(ConnectSoc, outputBuffer.c_str(), outputBuffer.size() + 1, NULL);
		bytes = recv(ConnectSoc, inputBuffer, sizeof(inputBuffer), NULL);

		return Response(inputBuffer, bytes);
	}

	Response checkAccaut(SOCKET ConnectSoc)
	{
		std::string outputBuffer;
		char inputBuffer[1024];
		int bytes;

		_login = dialogue("Enter your login");
		_passwd = dialogue("Enter your passwd");

		outputBuffer = Request("check", _login, _passwd, "0").toString();
		send(ConnectSoc, outputBuffer.c_str(), outputBuffer.size() + 1, NULL);
		bytes = recv(ConnectSoc, inputBuffer, sizeof(inputBuffer), NULL);

		std::cout << Response(inputBuffer, bytes)._status;

		return Response(inputBuffer, bytes);
	}

	Response post(SOCKET ConnectSoc)
	{
		std::string userInput;
		std::string outputBuffer;
		char inputBuffer[1024];
		int bytes = 0;

		userInput = dialogue("");

		outputBuffer = Request("post", _login, _passwd, "0", "", userInput, findPrivateUserName(userInput)).toString();
		send(ConnectSoc, outputBuffer.c_str(), outputBuffer.size() + 1, NULL);
		bytes = recv(ConnectSoc, inputBuffer, sizeof(inputBuffer), NULL);

		if (bytes > 0)
		{
			return Response(inputBuffer, bytes);
		}

		return Response("connectError", 13);
	}

	Response get(SOCKET ConnectSoc)
	{
		std::string outputBuffer;
		char inputBuffer[1024];
		int bytes;

		outputBuffer = Request("get", _login, _passwd, std::to_string(_lastMessage)).toString();
		send(ConnectSoc, outputBuffer.c_str(), outputBuffer.size() + 1, NULL);
		bytes = recv(ConnectSoc, inputBuffer, sizeof(inputBuffer), NULL);

		if (bytes > 0)
		{
			Response resp(inputBuffer, bytes);

			if (resp._status == "ok")
			{
				_messages.push_back(resp.toMessage());
				_lastMessage = resp._messageCount;
				std::cout << resp._owner << " " << resp._data << std::endl;
			}

			return resp;
		}
		else
			return Response("connectError", 13);
	}

};

void getNewMessages(Client& myClient, SOCKET& ConnectSoc)
{

	while (1)
	{
		if (myClient.get(ConnectSoc)._status != "ok");
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}



int main() {

	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}


	int IP;
	InetPtonW(AF_INET, L"127.0.0.1", &IP);
	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = IP;
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET ConnectSoc = socket(AF_INET, SOCK_STREAM, NULL);
	bind(ConnectSoc, (SOCKADDR*)&addr, sizeof(addr));
	connect(ConnectSoc, (SOCKADDR*)&addr, sizeof(addr));



	Client myClient;


	std::string userChoice;


	while (true)
	{
		std::cout << "Welcome to the C++ Chat \nenter @ before the username for the private message =) \n 1 = Registration \n 2 = Log in to account \n";
		std::getline(std::cin, userChoice);

		if (userChoice == "1")
			if (myClient.registration(ConnectSoc)._status == "ok")
			{
				std::thread(getNewMessages, std::ref(myClient), std::ref(ConnectSoc)).detach();
				while (true)
				{
					myClient.post(ConnectSoc);
				}
			}

		if (userChoice == "2")
			if (myClient.checkAccaut(ConnectSoc)._status == "ok")
			{
				std::thread(getNewMessages, std::ref(myClient), std::ref(ConnectSoc)).detach();
				while (true)
				{
					myClient.post(ConnectSoc);
				}
			}
	}

}