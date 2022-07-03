#include "Client.hpp"
#include "Cgi.hpp"
#include "Server.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "Connect.hpp"
#include "utils.hpp"

void Response::clear()
{
	body.clear();
	file_path.clear();
	header.clear();
}

static std::string make_hyper_link(Request& request, std::string path)
{
	DIR *dir_ptr = NULL;
	struct dirent *file = NULL;
	std::string a_tag;
	std::string dot = ".";
	std::string dotdot = "..";

	dir_ptr = opendir(path.c_str());
	if (!dir_ptr)
	{
		request.status_code = 400;
		return "";
	}
	while(1)
	{
		file = readdir(dir_ptr);
		if (!file)
			break ;
		if (file->d_name == dot)
			continue ;
		else if (file->d_name == dotdot)
		{
			a_tag += "<a href=\"";
			a_tag += request.path;
			if (a_tag[a_tag.size() - 1] != '/')
				a_tag += "/";
			a_tag += "..";
			a_tag += "\">";
			a_tag += "..";
			a_tag += "</a>\r\n";
			continue ;
		}
		a_tag += "<a href=\"";
		a_tag += request.path;
		if (a_tag[a_tag.size() - 1] != '/')
			a_tag += "/";
		a_tag += file->d_name;
		a_tag += "\">";
		a_tag += file->d_name;
		a_tag += "</a>\r\n";
	}
	return a_tag;
}

static std::string get_directory_name(Request& request, std::string path)
{
	if (path == "/")
		path = request.location->root + request.location->route;
	if (path[path.size() - 1] == '/')
		path = path.substr(0, path.size() - 2);
	return (path.substr(path.rfind("/")));
}

static void make_auto_index_page(Client& client, Request& request, Response& response)
{
	std::string	auto_index = "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n	<style>\r\n		body {\r\n			padding: 30px;\r\n		}\r\n		h1, h2 {\r\n			font-weight: 400;\r\n			margin: 0;\r\n		}\r\n		h1 > span {\r\n			font-weight: 900;\r\n		}\r\n		ul > li {\r\n			font-size: 20px;\r\n		}\r\n	</style>\r\n	<meta charset=\"UTF-8\">\r\n	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n	<title>Webserv index of $1</title>\r\n</head>\r\n<body>\r\n	<h1>Webserv index of <span>$1</span></h1>\r\n	<hr>\r\n	<ul>\r\n		<pre>$2</pre>\r\n	</ul>\r\n	<hr>\r\n</body>\r\n</html>\r\n";
	std::string name_dir = get_directory_name(request, request.path); // 해당 directory 이름
	std::string read_dir = make_hyper_link(request, request.location->root + request.path); // readir 함수 사용해서 읽어온 파일 목록

	if (read_dir.empty())
		return ;
	response.body = "";
	std::size_t index = 0;
	index = auto_index.find("$1");
	response.body += auto_index.substr(0, index) + name_dir;
	auto_index = auto_index.substr(index + 2);
	index = auto_index.find("$1");
	response.body += auto_index.substr(0, index) + name_dir;
	auto_index = auto_index.substr(index + 2);
	index = auto_index.find("$2");
	response.body += auto_index.substr(0, index) + read_dir;
	response.body += auto_index.substr(index + 2) + "\r\n";

	response.header = "HTTP/1.1 200 OK\r\nContent-Length: ";
	response.header += ft_itoa(response.body.size());
	client.respond_msg = response.header + "\r\n\r\n" + response.body;
	client._stage = SEND_RESPONSE;
}

static void method_get(Connect& cn, Request& request, Response& response)
{
	std::ifstream test_fs;
	std::string check_route;
	if (request.is_cgi) // cgi 일때
	{
		Cgi cgi(cn, cn.clients[cn.curr_event->ident]);
		request.status_code = cgi.m_cgi_exec();
	}
	else if (request.is_directory) // directory 일때 (파일이 아닐 때)
	{
		check_route = request.location->root + request.path; // root + (url에서 ? 앞부분) 이 유효한지만 체크 (파일 열때랑 동일)
		if (check_route[check_route.size() - 1] != '/')
			check_route += "/";
		test_fs.open(check_route);
		if (!test_fs.is_open())
			request.status_code = 404;
		else
		{
			test_fs.close();
			for (std::vector<std::string>::iterator iter = request.location->v_index.begin(); iter != request.location->v_index.end(); iter++) // location에 명시되어 있는 index페이지가 유효한지 하나씩 체크, 유효한거 처음 발견하면 file_path 저장 후 break;
			{
				std::string index_check = check_route + *iter;
				test_fs.open(index_check);
				if (test_fs.is_open())
				{
					request.status_code = 200;
					response.file_path = index_check;
					test_fs.close();
					break ;
				}
				test_fs.close();
			}
			if (request.status_code == 0 && request.location->auto_index) // index파일이 전부 유효하지 않은데(그래서 status_code가 0), auto_index는 켜져 있을 때
				make_auto_index_page(cn.clients[cn.curr_event->ident], request, response);
			else if (request.status_code == 0) // index 파일이 전부 유효하지 않은데, auto_index도 꺼져 있을 때
				request.status_code = 404;
		}
	}
	else // 파일일때, root + (url에서 ? 앞부분) 이 유효한지만 체크, 유효하면 file_path에 넣어준다. 유효하지 않으면 404.
	{
		check_route = request.location->root + request.path;
		test_fs.open(check_route);
		if (!test_fs.is_open())
			request.status_code = 404;
		else
		{
			request.status_code = 200;
			response.file_path = check_route;
		}
		test_fs.close();
	}
}

static void method_post(Connect& cn, Request& request, Client& client)
{
	struct stat		status;
	std::string path = request.location->root + request.path;

	if (request.is_directory) // 디렉토리라면
		request.status_code = 405;
	else if (!request.content_length)	// POST 요청에는 반드시 body 및 content-length가 필요함, 없을 시 411 length required
		request.status_code = 411;
	else if (request.content_length > request.location->client_max_body_size) // 우리가 허용한 body를 넘을 때
		request.status_code = 413;
	else if (request.is_cgi) // cgi 라면
	{
		Cgi cgi(cn, client);
		request.status_code = cgi.m_cgi_exec();
	}
	else // 아니면 일반 POST
	{
		if (stat(path.c_str(), &status))
			request.status_code = 201; //없어서 CREATED
		else
		{
			request.status_code = 200; //있어서 OK
			client.rs.file_path = cn.first_line[200].second;
		}
		int file_fd = open(path.c_str(), O_RDWR|O_CREAT|O_APPEND, 0644);
		if (file_fd == -1)
		{
			request.status_code = 400;
			client._stage = SET_RESOURCE;
			return ;
		}
		std::cout << file_fd << " : " << path << std::endl;
		change_events(cn.change_list, file_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
		Client c1;
		c1.origin_fd = cn.curr_event->ident;
		c1._stage = FILE_WRITE;
		client.tmp_buffer = client.rq.body;
		cn.clients.insert(std::make_pair(file_fd, c1));
		client._stage = WAIT;
	}
}

static void method_delete(Request& request, Client& client)
{
	struct stat		status;
	std::string path = request.location->root + request.path;

	if (stat(path.c_str(), &status)) // 존재하지 않는 경로
		request.status_code = 404;
	else if (request.is_directory) // 디렉토리
		request.status_code = 405;
	else
	{
		int ret = remove(path.c_str());
		if (ret)
			request.status_code = 405;
		else 
		{
			request.status_code = 200; // 200이 아닌 번호를 선택한 경우, respons skip 하는 부분에서 추가해줘야 함.
			client.rs.body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n	<title>Delete request Done</title>\r\n</head>\r\n<body>\r\n	<h1>Your Delete request Done!</h1>\r\n</body>\r\n</html>\r\n";
			client.rs.header = "HTTP/1.1 200 OK\r\nContent-Length: ";
			client.rs.header += ft_itoa(client.rs.body.size());
			client.respond_msg = client.rs.header + "\r\n\r\n" + client.rs.body;
			client._stage = SEND_RESPONSE;
		}
	}
		/*
		NOTE delete 메서드 처리 후 어떤 응답보낼지 선택가능해보입니다 그냥 204로만 해도 될까요

			응 답
		DELETE 메서드를 성공적으로 적용한 후에 사용할 수 있는 응답 상태 코드는 다음과 같이 몇 가지가 있습니다.
		아마도 명령을 성공적으로 수행할 것 같으나 아직은 실행하지 않은 경우 202 (Accepted) 상태 코드.
		명령을 수행했고 더 이상 제공할 정보가 없는 경우 204 (No Content) 상태 코드.
		명령을 수행했고 응답 메시지가 이후의 상태를 설명하는 경우 200 (OK) 상태 코드.
	*/
}

static void method_exe(Connect& cn, Client& client)
{
	if (client.rq.method == GET)
		method_get(cn, client.rq, client.rs);
	else if (client.rq.method == POST)
		method_post(cn, client.rq, client);
	else if (client.rq.method == DELETE)
		method_delete(client.rq, client);
}

static void set_response(Connect& cn, Request& request, Response& response)
{
	response.header = "HTTP/1.1 ";
	response.header += ft_itoa(request.status_code);
	response.header += " " + cn.first_line[request.status_code].first;
	if (request.status_code == 200)
		return ;
	else if (request.status_code >= 400)
		cn.clients[cn.curr_event->ident].keep = false;
	response.file_path = cn.first_line[request.status_code].second;
	for (std::vector<int>::iterator iter = request.location->p_error_page.first.begin(); iter != request.location->p_error_page.first.end(); iter++)
	{
		if (*iter == request.status_code)
			response.file_path = request.location->p_error_page.second;
	}
}

static void make_redirection(Connect& cn, Client& client)
{
	client.respond_msg = "HTTP/1.1 ";
	client.respond_msg += ft_itoa(client.rq.location->p_return.first);
	client.respond_msg += " " + cn.first_line[client.rq.location->p_return.first].first;
	client.respond_msg += "\r\n";
	client.respond_msg += "Location: " + client.rq.location->p_return.second + "\r\n\r\n";
}

void response(Connect& cn, Client& client, Request& request)
{
	if (cn.clients[cn.curr_event->ident]._stage == SET_RESOURCE)
		std::cout << "STAGE SET_RESOURCE" << std::endl; 
	if (!client.is_io_done)
	{
		if (!request.status_code)
		{
			if (request.location->p_return.first)
			{
				make_redirection(cn, client);
				client._stage = SEND_RESPONSE;
				return ;
			}
			method_exe(cn, client);
		}
		if (request.is_cgi && !request.status_code)
		{
			client.is_io_done = true;
			return ;
		}
		set_response(cn, request, client.rs);
		if(client._stage == SEND_RESPONSE || client._stage == WAIT)
			return ;

		int file_fd = open(client.rs.file_path.c_str(), O_RDONLY);
		if (file_fd == -1)
		{
			client.respond_msg = client.rs.header + "\r\n\r\n" + cn.default_error_page;
			client._stage = SEND_RESPONSE;
			return ;
		}
		struct stat sb;
		stat(client.rs.file_path.c_str(), &sb);
		if (sb.st_size == 0)
		{
			client.is_io_done = true;
			return ;
		}
		std::cout << file_fd << " : " << client.rs.file_path << std::endl;
		change_events(cn.change_list, file_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		Client c1;
		c1.origin_fd = cn.curr_event->ident;
		c1._stage = FILE_READ;
		cn.clients.insert(std::make_pair(file_fd, c1));
		client._stage = WAIT;
		client.is_io_done = true;
	}
	else if (client.is_io_done)
	{
		if (request.is_cgi && request.status_code) // error.html 읽으러 다시 들어오세요
		{
			request.is_cgi = false;
			client.is_io_done = false;
			client.tmp_buffer.clear();
			return ;
		}
		else if (request.is_cgi && request.location->cgi == ".bla" && !request.status_code)
		{
			client.rs.body = client.tmp_buffer.substr(client.tmp_buffer.find("\r\n\r\n") + 4);
			client.respond_msg = "HTTP/1.1 200 OK\r\n";
			client.respond_msg += "Content-length: " + ft_itoa2(client.rs.body.size());
			client.respond_msg += "\r\n\r\n";
			client.respond_msg += client.rs.body;
			client.is_io_done = false;
			client._stage = SEND_RESPONSE;
			return ;
		}
		else if (request.is_cgi && !request.status_code)
		{
			client.respond_msg = "HTTP/1.1 200 OK\r\n";
			client.respond_msg += client.tmp_buffer;
			client.is_io_done = false;
			client._stage = SEND_RESPONSE;
			return ;
		}
		client.rs.body = client.tmp_buffer;
		client.respond_msg = client.rs.header + "\r\n";
		client.respond_msg += "Content-Length: " + ft_itoa2(client.rs.body.size());
		client.respond_msg += "\r\n\r\n";
		client.respond_msg += client.rs.body;
		client.is_io_done = false;
		client._stage = SEND_RESPONSE;
	}
}
