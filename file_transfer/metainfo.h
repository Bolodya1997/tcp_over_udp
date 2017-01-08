#include <endian.h>
#include <arpa/inet.h>

struct file_metainfo{
	unsigned int name_size;
	unsigned long long file_size;

	file_metainfo();
	void set_file_size(unsigned long long file_size);
	void set_name_size(unsigned long long name_size);
	unsigned long long get_file_size();
	unsigned int get_name_size();
};
