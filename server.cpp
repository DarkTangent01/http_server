#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/inotify.h>

#define PORT 8080
#define BUFFER_SIZE 4096

std::map<std::string, std::string> MIME_TYPES = {
	{"html", "text/html"},
	{"css", "text/css"},
	{"js", "text/javascript"}};

void serve_file(int client_socket, const std::string &filename)
{
	std::string extension = filename.substr(filename.rfind(".") + 1);
	std::string mime_type = MIME_TYPES[extension];

	std::ifstream file(filename);
	if (!file)
	{
		std::cerr << "Error: could not open file " << filename << std::endl;
		return;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();

	std::string content = buffer.str();
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: " +
		mime_type + "\r\n"
					"Content-Length: " +
		std::to_string(content.size()) + "\r\n"
										 "\r\n" +
		content;

	send(client_socket, response.c_str(), response.size(), 0);
}

int main()
{
	int server_socket, client_socket, opt = 1;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		std::cerr << "Error: socket creation failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				   &opt, sizeof(opt)))
	{
		std::cerr << "Error: setsockopt failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		std::cerr << "Error: bind failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 10) < 0)
	{
		std::cerr << "Error: listen failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Server started on port " << PORT << std::endl;

	// Create an inotify instance
	int inotify_fd = inotify_init();
	if (inotify_fd == -1)
	{
		std::cerr << "Error: inotify_init failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Add the index.html file to the watch list
	std::string index_file_path = "index.html";
	int wd = inotify_add_watch(inotify_fd, index_file_path.c_str(), IN_MODIFY);
	if (wd == -1)
	{
		std::cerr << "Error: inotify_add_watch failed for " << index_file_path << std::endl;
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(server_socket, &readfds);
		FD_SET(inotify_fd, &readfds);

		int max_fd = std::max(server_socket, inotify_fd);

		if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
		{
			std::cerr << "Error: select failed" << std::endl;
			exit(EXIT_FAILURE);
		}

		// Check if there is a new client connection
		if (FD_ISSET(server_socket, &readfds))
		{
			if ((client_socket = accept(server_socket, (struct sockaddr *)&address,
										&addrlen)) < 0)
			{
				std::cerr << "Error: accept failed" << std::endl;
				exit(EXIT_FAILURE);
			}

			char buffer[BUFFER_SIZE] = {0};
			read(client_socket, buffer, BUFFER_SIZE);

			std::istringstream iss(buffer);
			std::string request;
			iss >> request;

			if (request != "GET")
			{
				std::cerr << "Error: unsupported HTTP method " << request << std::endl;
				close(client_socket);
				continue;
			}

			std::string path;
			iss >> path;

			if (path == "/")
			{
				path = "/index.html";
			}

			path = "." + path;

			if (access(path.c_str(), F_OK) == -1)
			{
				std::cerr << "Error: file " << path << " does not exist" << std::endl;
				close(client_socket);
				continue;
			}

			serve_file(client_socket, path);
			close(client_socket);
		}

		// Check if the index.html file was modified
		if (FD_ISSET(inotify_fd, &readfds))
		{
			char buffer[BUFFER_SIZE];
			ssize_t length = read(inotify_fd, buffer, BUFFER_SIZE);

			if (length == -1)
			{
				std::cerr << "Error: read from inotify fd failed" << std::endl;
				exit(EXIT_FAILURE);
			}

			// Parse the inotify event
			struct inotify_event *event = (struct inotify_event *)buffer;

			if (event->mask & IN_MODIFY)
			{
				std::cout << "index.html was modified" << std::endl;
				serve_file(client_socket, "index.html");
			}
		}
	}

	return 0;
}