/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TcpServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ecarlier <ecarlier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/30 15:38:05 by ecarlier          #+#    #+#             */
/*   Updated: 2024/10/02 14:20:57 by ecarlier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/TcpServer.hpp"

TcpServer::TcpServer()
{
	std::cout << "\033[33m" << "Default constructor called" << "\033[0m" << std::endl;
}

TcpServer::~TcpServer()
{
	std::cout << "\033[32m" << "Destructor called" << "\033[0m" << std::endl;
}

// void TcpServer::closeServer()
// {
// 		close(m_socket);
// 		exit(0);
// }

/*
AF_INET -> IPv4
SOCK_STREAM -> TCP
0 -> default protocol -> TCP
*/
int TcpServer::startServer()
{
	std::cout << "Starting server..." << std::endl;
	
	m_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (m_socket == -1)
		throw TcpServer::SocketCreationFailed();
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);  // Port 8080
	server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface (including localhost)

	if (bind(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
		throw TcpServer::SocketBindingFailed();  //binding failed, could be replace by throw std::runtime_error("Binding failed");

	if (listen(m_socket, SOMAXCONN) < 0)
		throw TcpServer::SocketlisteningFailed();

	std::cout << "Server is listening on port 8080..." << std::endl;

	socklen_t client_addr_len = sizeof(client_addr);

	while (true)
	{
		client_socket = accept(m_socket, (struct sockaddr*)&client_addr, &client_addr_len);

		if (client_socket < 0)
			throw SocketAcceptFailed();

		char buffer[1024] = {0};
		int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
		if (bytes_read < 0)
			throw SocketReadFailed();

		std::cout << "Received: " << buffer << std::endl;


		std::string response = "HTTP/1.1 200 OK\r\n"
							"Content-Type: text/plain\r\n"
							"Content-Length: 13\r\n"
							"\r\n"
							"Hello, World!";
		send(client_socket, response.c_str(), response.size(), 0);


		close(client_socket);
	}


	return 0;
}

/* --------------- Expections handling --------------- */

const char* TcpServer::SocketCreationFailed::what() const throw ()
{
	return ("Trowing exception : creating server socket");
}

const char* TcpServer::SocketBindingFailed::what() const throw ()
{
	return ("Trowing exception : socket binding failed");
}

const char* TcpServer::SocketlisteningFailed::what() const throw ()
{
	return ("Trowing exception : socket listening failed");
}

const char* TcpServer::SocketAcceptFailed::what() const throw ()
{
	return ("Trowing exception : Failed to accept connection");
}

const char* TcpServer::SocketReadFailed::what() const throw ()
{
	return ("Trowing exception : Failed to read from client");
}