#include <map>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <poll/poller.h>

#include "metainfo.h"
#include "../d_tcp/d_socket.h"
#include "../system/system.h"

using namespace std;

#define PART_SIZE 2*1024*1024
#define BUF_SIZE 2*1024*1024

d_socket *server_socket;
int file_descriptor;

void error(const char *msg, int code){
	perror(msg);
	exit(code); 
}

void args_check(int argc){

	if(argc >= 4) return;

	cerr << "Port number or host IP not found. Stopped." << endl
		 << "Usage: ./client [host IP][Port number][File Path]" << endl;

	exit(1); //Code for error: "Port number not found";
}

void activate_socket(char **argv){
 
    struct sockaddr_in serv_addr;

	//Init socket for connection
    try {
        server_socket = new d_socket(argv[1], atoi(argv[2]));

		auto __poller = new poller(-1);
		__poller->add_untimed(server_socket->set_actions(POLL_AC));
		__poller->poll();
		delete __poller;

        server_socket->accept();	//	TODO: connect

	} catch (net_exception) {
        error("d_socket", -1);
    }

    int flags = fcntl(server_socket->get_fd(), F_GETFL) & ~O_NONBLOCK;
    fcntl(server_socket->get_fd(), F_SETFL, flags);
}

unsigned long long open_file(string &path){	
	file_descriptor = open(path.c_str(), O_RDONLY);
	if(file_descriptor == -1) error("open_file", 5);

	unsigned long long file_size = lseek(file_descriptor, 0L, SEEK_END);
	lseek(file_descriptor, 0L, SEEK_SET);

	return file_size;
}


void _write(pollable *_pollable, const void* _buf, unsigned int size){
	int writed;
	const char *buf = (const char*)_buf;
	while(size > 0){
		auto __poller = new poller(-1);
		__poller->add_untimed(_pollable->set_actions(POLL_WR));
		__poller->poll();
		delete __poller;

        writed = _pollable->write(buf, size);
		if(writed == -1) error("_write: Whooopsie", 6);
		size -= writed;
		buf += writed;
	}
}

int prepare_send(string path){
	string base =  path.substr(path.find_last_of("/") + 1);
	const char *file_name = base.c_str(); // ???
	int lenght = strlen(file_name)+1;
	cout << "path: " << base << endl; 

	unsigned long long file_size = open_file(path);

	file_metainfo fmi;
	fmi.set_file_size(file_size);
	fmi.set_name_size(lenght); // +1 for terminating 0
	
	//Debug output
	cout << "file size: " << file_size  << endl;
	cout << "name: " << file_name << " Name size: " << strlen(file_name)+1 << endl;
	
	_write(server_socket, &fmi, sizeof(fmi));
	_write(server_socket, file_name, lenght);

	int answer;

	auto __poller = new poller(-1);
	__poller->add_untimed(server_socket->set_actions(POLL_RE));
	__poller->poll();
	delete __poller;

    server_socket->read(&answer, sizeof(int));
	answer = ntohl(answer);
	if(answer == -1) error("Error on a server. Please rename a file", 3);

	return file_size;
}

void send_file(pollable *socket, int file, unsigned long long file_size){
	char *buf = new char[BUF_SIZE];
	while(file_size > 0){
		int readed = read(file, buf, BUF_SIZE);
		cout << "Part size: " << readed << endl;
		_write(socket, buf, readed);
		file_size -= readed;
	}

	cout << "Sended";
	
	delete[] buf;
}

int main(int argc, char **argv){

	args_check(argc);
	activate_socket(argv);
	unsigned long long file_size = prepare_send(string(argv[3]));
    send_file(server_socket, file_descriptor, file_size);

    delete server_socket;
    system::get_instance()->wait();
	return 0;
}