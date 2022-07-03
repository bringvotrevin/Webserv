#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <dirent.h>

struct Response
{
	std::string	file_path;
	std::string	header;
	std::string body;
	uintptr_t index;

	Response();
	void clear();
};

#endif