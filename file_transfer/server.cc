#include <map>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll/poller.h>
/* According to POSIX.1-2001, POSIX.1-2008 */

/* According to earlier standards */

#include "metainfo.h"
#include "../d_tcp/d_server_socket.h"
#include "fd_wrap.h"
#include "../d_tcp/d_socket.h"

using namespace std;

#define TIME 1
#define MAX_CLIENTS 100
#define BUF_SIZE 256*1024*1024

int num;
d_server_socket *real_socket;
char *buf;

long get_time(){
	timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000000 + tp.tv_usec;
}

void args_check(int argc){

	if(argc >= 2) return;

	cerr << "Port number not found. Stopped." << endl
		 << "Usage: ./server [Port number]" << endl;

	exit(1); //Code for error: "Port number not found";
}

void error(const char *msg, int code){
	perror(msg);
	cout << "s" << endl;
	exit(code); 
}

void activate_socket(char **argv){
	int port_num = atoi(argv[1]);

	try {
		real_socket = new d_server_socket((uint16_t) port_num);
	} catch (net_exception) {
		error("server fail", -1);
	}
}

struct client_info{

    fd_wrap *file;
	
	string file_name;

	bool transfer_inited = false;
	file_metainfo fmi;
	
	unsigned long long full_readed = 0;
};

int sel(fd_set *sock_set) {
	timeval waiting_time;
	waiting_time.tv_sec = TIME;
	waiting_time.tv_usec = 0;
	
	int result = select(MAX_CLIENTS + 2, sock_set, NULL, NULL, &waiting_time);

	return result;
}

void try_add_sockets(map <d_socket *, client_info> &sock_map, fd_set *sock_set) {
	if (!FD_ISSET(real_socket->get_fd(), sock_set))
		return;

    auto new_sock = dynamic_cast<d_socket *>(real_socket->accept());

	client_info i;
	sock_map.insert(pair<d_socket *, client_info>(new_sock, i));
}

fd_set generate_set(map<d_socket *, client_info> &sock_map){
	fd_set new_set;
	FD_ZERO(&new_set);
	FD_SET(real_socket->get_fd(), &new_set);
	for (auto it=sock_map.begin(); it!=sock_map.end(); ++it)
		FD_SET(it->first->get_fd(), &new_set);
	
	return new_set;
}

void _write(pollable *_pollable, const void* _buf, unsigned int size){
	int writed;
	const char *buf = (const char*)_buf;
	while(size > 0){
		auto __poller = new poller(-1);
		__poller->add_untimed(_pollable->set_actions(POLL_WR));
		__poller->poll();
		delete __poller;
        writed = (int) _pollable->write(buf, size);
		if(writed == 0) error("_write: Whooopsie", 6);
		size -= writed;
		buf += writed;
	}
}


string get_name(pollable *sock, file_metainfo &fmi){
	unsigned int name_size = fmi.get_name_size();
	char* file_name = new char[name_size];

	cout << "fmi.get_name_size(name_size): " << name_size << endl;

	auto __poller = new poller(-1);
	__poller->add_untimed(sock->set_actions(POLL_RE));
	__poller->poll();
	delete __poller;

    sock->read(file_name, name_size);

	cout << "Name): " << file_name << endl;

	string path = file_name;
	string base_filename = path.substr(path.find_last_of("/") + 1);

	cout << "fmi.get_file_size(name_size): " << fmi.get_file_size() << endl;

	delete[] file_name;

	return base_filename;
}

void init_transfer(map<d_socket *, client_info>::iterator it) {
	auto __poller = new poller(-1);
	__poller->add_untimed(it->first->set_actions(POLL_RE));
	__poller->poll();
	delete __poller;

    cerr << it->first->read(&(it->second.fmi), sizeof(file_metainfo)) << endl;
	it->second.file_name = get_name(it->first, it->second.fmi);

	int fd = open(it->second.file_name.c_str(), O_RDWR | O_CREAT| O_EXCL);
	if (fd == -1) {
		cout << "FILE? " << endl;
		it->second.transfer_inited = false;
		_write(it->first, &fd, sizeof(int));
		return;
	}

    it->second.file = new fd_wrap(fd);

	_write(it->first, &fd, sizeof(int));
	it->second.transfer_inited = true;
}

map<d_socket *, client_info>::iterator delete_node(map<d_socket *, client_info> &sock_map,
                                                   map<d_socket *, client_info>::iterator to_del){
    cout << "Summary downloaded: " << to_del->second.full_readed << "Bytes" << endl;
	cout << "Downloading status: failed" << endl;
	delete to_del->second.file;
	delete to_del->first;
	return sock_map.erase(to_del);
}

void init_map(map<d_socket *, client_info> &sock_map, fd_set *set) {
	for (auto it = sock_map.begin(); it != sock_map.end();) {
		if (!FD_ISSET(it->first->get_fd(), set) || it->second.transfer_inited) {
			it++;
			continue;
		}
    	
		init_transfer(it);
		if (!it->second.transfer_inited) {
    		it = delete_node(sock_map, it);
		    cout << "Client disconnected. " << endl;	
    		continue;
		} else {
			++it;
		}
    }
}

void read_data(map<d_socket *, client_info> &sock_map, fd_set *set) {
	for (auto it = sock_map.begin(); it != sock_map.end();) {
		if (!FD_ISSET(it->first->get_fd(), set) && !it->second.transfer_inited) {
			it++;
			continue;
		}

		auto __poller = new poller(-1);
		__poller->add_untimed(it->first->set_actions(POLL_RE));
		__poller->poll();
		delete __poller;

		ssize_t readed = it->first->read(buf, BUF_SIZE);
		if (readed < 1) {
    		it = delete_node(sock_map, it);
			continue;
		};

		_write(it->second.file, buf, (unsigned int) readed);
		it->second.full_readed += readed;
		
		cout << "Progress from " << it->first << ": " << 100*(double)(it->second.full_readed)/it->second.fmi.get_file_size()  << " %" << endl;

		if (it->second.full_readed == it->second.fmi.get_file_size()) {
    		cout << "File " << it->second.file_name << " from " << it->first << " downloaded!" << endl;	
    		it = delete_node(sock_map, it);
		    cout << "Client disconnected. " << endl;
		} else {
			++it;
		}
    }
}

int main(int argc, char **argv){

	buf = new char[BUF_SIZE];
	
	map<d_socket *, client_info> sock_map;
	args_check(argc);
	activate_socket(argv);

	while(true){
		fd_set socks = generate_set(sock_map);
//		cout << "select wait" << endl;
		num = sel(&socks);
//		cout << "Init" << endl;
		init_map(sock_map, &socks);
//		cout << "try_add" << endl;
		try_add_sockets(sock_map, &socks);
//		cout << "read" << endl;
		read_data(sock_map, &socks);
		//cout << "generate" << endl;
		//cout << "NEW ROUND" << endl;
		
	}

	delete[] buf;
	return 0;
}