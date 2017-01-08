#include "metainfo.h"

//for struct file_metainfo
file_metainfo::file_metainfo(){file_size = 0; name_size = 0;}
void file_metainfo::set_file_size(unsigned long long file_size){this->file_size = htobe64(file_size);}
void file_metainfo::set_name_size(unsigned long long name_size){this->name_size = htonl(name_size);}
unsigned long long file_metainfo::get_file_size(){return be64toh(file_size);}
unsigned int file_metainfo::get_name_size(){return ntohl(name_size);};
