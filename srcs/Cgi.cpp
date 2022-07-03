#include "Cgi.hpp"
#include "utils.hpp"

#define NC "\033[0m"
#define BLCK "\033[30m"
#define RED  "\033[31m"
#define GREN "\033[32m"
#define YLLW "\033[33m"
#define BLUE "\033[34m"
#define PRPL "\033[35m"
#define AQUA "\033[36m"

Cgi::Cgi(Connect& connect, Client& client)
	: __cn(connect), __request(client.rq), __v_envlist(std::vector<std::string>(15, ""))
{
	__cwd = m_get_cwd();
	__filepath = m_get_filepath();
	__requested_uri = __cwd + __filepath;
	__query_string = m_get_query_string();
	__v_envlist[0] = "REQUEST_METHOD=" + m_get_method(__request.method);
	__v_envlist[1] = "CONTENT_LENGTH=" + ft_itoa2(__request.content_length);
	__v_envlist[2] = "CONTENT_TYPE=" +  __request.content_type; //request헤더 content_type 추가하기
	__v_envlist[3] = "GATEWAY_INTERFACE=CGI/1.1";
	__v_envlist[4] = "DOCUMENT_ROOT=" + __cwd;
	__v_envlist[5] = "PATH_TRANSLATED=" + __requested_uri;
	__v_envlist[6] = "QUERY_STRING=" + __query_string;
	__v_envlist[7] = "SCRIPT_NAME=" + __filepath;
	__v_envlist[8] = "SERVER_NAME=" + client.server->listen.first;
	__v_envlist[9] = "SERVER_PORT=" + ft_itoa(client.server->listen.second);
	__v_envlist[10] = "SERVER_PROTOCOL=HTTP/1.1";
	__v_envlist[11] = "SERVER_SOFTWARE=dimteamwebserv";
	__v_envlist[12] = "REQUEST_URI=" + __requested_uri;
	__v_envlist[13] = "PATH_INFO=" + __requested_uri;
	__v_envlist[14] = "SCRIPT_FILENAME=" + __requested_uri;
	client.tmp_buffer = __request.body;
}

Cgi::~Cgi() {}

std::string	Cgi::m_get_cwd()
{
	char cwd[1000];
	getcwd(cwd, 1000);
	return ((std::string)cwd);
}

std::string Cgi::m_get_filepath()
{
	if (m_get_method(__request.method) == "POST")
		return (__request.url);
	std::string root = __request.location->root;
	if (root[0] == '.')
		root = root.substr(1);
	return (root + __request.path);
}

std::string Cgi::m_get_query_string()
{
	if (m_get_method(__request.method) == "POST" || __request.url.find('?') == std::string::npos)
		return ("");
	return (__request.url.substr(__request.url.find_last_of('?') + 1));
}

std::string	Cgi::m_get_method(int method)
{
	switch (method)
	{
		case GET :
			return ("GET");
		case POST :
			return ("POST");
		default :
			return ("");
	}
}

void	Cgi::m_set_env()
{
	__env = new char*[__v_envlist.size() + 1];
	for (unsigned long i = 0; i < __v_envlist.size(); i++)
	{
		__env[i] = new char[__v_envlist[i].size() + 1];
		strcpy(__env[i], __v_envlist[i].c_str());
		__env[i][__v_envlist[i].size()] = 0;
	}
	__env[__v_envlist.size()] = nullptr;
}

void	Cgi::m_set_argv()
{
	std::string cgi_path = __cwd + "/php-cgi"; //cgi exec 변경
	if (__request.location->cgi == ".bla")
		cgi_path = __cwd + "/cgi_tester";
	__argv[0] = new char[cgi_path.size() + 1];
	strcpy(__argv[0], cgi_path.c_str());
	__argv[0][cgi_path.size()] = 0;
	__argv[1] = new char[__requested_uri.size() + 1];
	strcpy(__argv[1], __requested_uri.c_str());
	__argv[1][__requested_uri.size()] = 0;
	__argv[2] = nullptr;
}

void	Cgi::m_delete()
{
	for (unsigned long i = 0; i < __v_envlist.size() + 1; i++)
		delete[] __env[i];
	delete[] __env;
	delete[] __argv[0];
	delete[] __argv[1];
}

int		Cgi::m_cgi_exec()
{
	FILE *fIn = tmpfile();
    FILE *fOut = tmpfile();
	int in_fd = fileno(fIn);
    int out_fd = fileno(fOut);
	pid_t	pid;

	m_set_env();
	m_set_argv();
	if ((pid = fork()) == -1)
		return (500);
	else if (pid == 0)
	{
		dup2(in_fd, STDIN_FILENO);
		dup2(out_fd, STDOUT_FILENO);
		execve(__argv[0], __argv, __env);
		std::cout << "500" << std::endl;
		exit(1);
	}
	change_events(__cn.change_list, in_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL); // write가 더이상 필요한가??
	change_events(__cn.change_list, out_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	Client c1;
	Client c2;
	c1.origin_fd = __cn.curr_event->ident; //현재 클라이언트 socket fd
	c2.origin_fd = __cn.curr_event->ident; //현재 클라이언트 socket fd
	c2.cgi_pid = pid;
	c1._stage = CGI_WRITE;
	c2._stage = CGI_READ;
	__cn.clients.insert(std::make_pair(in_fd, c1));
	__cn.clients.insert(std::make_pair(out_fd, c2));
	__cn.clients[__cn.curr_event->ident]._stage = WAIT;
	m_delete();
	return (0);
}
