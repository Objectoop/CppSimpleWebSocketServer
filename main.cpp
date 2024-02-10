#include <iostream>
#include <thread>
#include "SimpleWebSocket.h"



void handleMessage(SOCKET clientSocket, std::string message);

int main() 
{	
	SimpleWebSocket server;  //Default initialization

	server.handleClientMessage = handleMessage;
	server.startWebSocketServer().detach();
	while (true)
	{
		//Loop sending message
		std::cout << "Press any key to send message to client.." << std::endl;
		getchar();
		std::string msg = "Hello";

		for (int i = 0; i < server.socketCliensts.size(); i++) {

			//std::cout << server.charHolder[i].recvbuf << std::endl;
			server.sendSocketMessageString(server.socketCliensts[i], msg + " socket " + std::to_string(i));
		}
		
	}
	
	return 1;
	
	
}


void handleMessage(SOCKET clientSocket, std::string message)
{
	std::cout << "this guy said " << message << std::endl;

}

