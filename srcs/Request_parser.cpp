#include "Request_parser.hpp"
#include "Connect.hpp"

Request_parser::Request_parser(std::string request_msg)
	: __request_msg(request_msg) { }
Request_parser::~Request_parser() { }

static int		parse_method(std::string str)
{
	int	method = 0;

	const char *methods[9] = {"GET", "POST", "DELETE", "HEAD", "PUT", "OPTIONS", "PATCH", "CONNECT", "TRACE"};
	int bit = 0x01;
	for(int i = 0; i < 9; i++)
	{
		if (methods[i] == str)
			method |= bit;
		bit <<= 1;
	}
	if (method & 0x01 << 4)
		method |= 0x01 << 1;
	return (method);
}

static Location* search_location_in_list(std::string& str, std::vector<Location> &list)
{
	std::vector<Location>::iterator it = list.begin();
	if (str.empty())
		str = "/";

	for (; it != list.end(); it++)
	{
		if (it->route == str) {
			return &(*it);
		}
	}
	return NULL;
}

Location* Request_parser::search_location(std::string str, std::vector<Location> &list)
{
	// 로케이션 완전일치 확인
	Location *location = search_location_in_list(str, list);
	std::string tmp_str(str);

	// 최상위 루트까지 상위 디렉토리 하나씩 일치 확인	
	while (!location && tmp_str != "/")
	{
		tmp_str = tmp_str.substr(0, tmp_str.rfind("/"));
		location = search_location_in_list(tmp_str, list);
	}

	// cgi 확인
	size_t pos = str.find('.');
	if (location->cgi.size() && pos != std::string::npos && str.substr(pos) == location->cgi)
		request.is_cgi = true;
	return location;
}

std::list<std::string>		split_word(std::string str)
{
	std::stringstream	iss(str);
	std::string			buf;
	std::list<std::string> list;

	while(iss >> buf)
		list.push_back(buf);
	return list;
}

void	Request_parser::split_request_msg()
{
	request.body = "";
	size_t pos = __request_msg.find("\r\n\r\n");
	if (pos != std::string::npos)
		request.body = __request_msg.substr(pos + 4);

	std::string header = __request_msg.substr(0, pos);

	pos = header.find("\r\n");
	std::string line;
	while (pos != std::string::npos)
	{
		line = header.substr(0, pos);
		header = header.substr(pos + 2);

		__l_file.push_back(split_word(line));
		pos = header.find("\r\n");
	}
	__l_file.push_back(split_word(header));
}


std::list<std::string> Request_parser::m_next_line()
{
	if (__l_file.size() > 1)
		__l_file.pop_front();
	return (__l_file.front());
}

std::list<std::string>		split_line(std::string	origin)
{
	std::list<std::string>		list;
	size_t			pos = origin.find("\r\n");
	while (pos != std::string::npos)
	{
		std::string		line = origin.substr(0, pos);
		origin = origin.substr(pos + 2);
		list.push_back(line);
		pos = origin.find("\r\n");
	}
	return list;
}

size_t		unchunk_data(std::list<std::string> list, std::string& body, int &status_code)
{
	size_t			length_size;
	while(!list.empty())
	{
		std::stringstream	iss(list.front());
		list.pop_front();
		iss >> std::hex >> length_size;
		if (iss.fail() || length_size != list.front().size())
			status_code = 400;
		body.append(list.front());
		list.pop_front();
	}
	return length_size;
}

void		Request_parser::m_unchunk_after_body_clear(bool& is_enough_body_length)
{
	std::list<std::string>		list = split_line(request.body);
	request.body.clear();
	size_t last_length = unchunk_data(list, request.body, request.status_code);
	
	if (last_length != 0)
		is_enough_body_length = false;
}

void	Request_parser::parse_request(Client& client)
{
	size_t pos = __request_msg.find("\r\n\r\n");
	if (pos == std::string::npos)
		return ;
	m_parse_request_header(client);
}


std::string		transform_tolower(std::string str)
{
	std::string::iterator	it = str.begin();

	for(; it != str.end(); it++)
		*it = std::tolower(*it);
	return str;
}

void	Request_parser::m_parse_request_header (Client& client)
{
	Server *server = client.server;
	// url 입력값 없어도 괜찮도록 디폴트 로케이션 설정
	Location	*location = &(server->default_location);
	request.url = "/";
	request.path = "/";
	request.location = location;

	split_request_msg();
	if (request.status_code)
		return ;
	__l_line = __l_file.front();
	if (!__l_line.size())
	{
		// 첫줄이 빈 줄이면 에러 response 최소 메소드는 들어와야 함 400 Bad Request
		request.status_code = 400;
		return ;
	}
	// 처음 등장하는 단어(=method) 저장, 로케이션 확정 후 메소드 해석할 예정
	std::string		method_str = __l_line.front();
	__l_line.pop_front();

	// parse location part
	if (__l_line.size())
	{
		// set location & url & path
		request.url = __l_line.front();
		if (request.url.find(":") != std::string::npos)
		{
			request.status_code = 400;
			return ;
		}
		request.path = request.url.substr(0,request.url.find("?"));
		location = search_location(request.path, server->location);
		__l_line.pop_front();	
	}
	request.location = location;

	// 디렉토리 여부 확인 
	struct stat		status;
	std::string		path = request.location->root + request.path;
	stat(path.c_str(), &status);
	if (S_ISDIR(status.st_mode))
		request.is_directory = true;
	
	// 본격적인 메소드 해석
	int method = parse_method(method_str);
	if (!method)
	{
		request.status_code = 400;
		return ;
	}
	if (!(method & location->methods))
	{
		// 405 not allowed method
		request.status_code = 405;
		return ;
	}
	request.method = method;
	// 프로토콜 들어오지 않아도 이상없음
	// 하지만 들어왔다면 HTTP/1.1과 정확히 일치해야 아니면 505 HTTP Version Not Supported
	if (!__l_line.empty() && !(__l_line.front() == "HTTP/1.1"))
	{
		request.status_code = 505;
		return ;
	}
	// post 메소드라면 반드시 content-length 헤더를 가져야 함
	bool post_must_have_content_length = !(request.method & POST);
	// post 메소드가 아니면 항상 true
	bool is_enough_body_length = true;
	
	while (__l_file.size() > 1)
	{
		__l_line = m_next_line();
		if (transform_tolower(__l_line.front()) == "content-length:" && __l_line.size() == 2)
		{
			post_must_have_content_length = true;
			// GET 일때 Content-Length가 들어와도 nginx 200 확인함
			request.content_length = std::atoi(__l_line.back().c_str());
			if(request.content_length > static_cast<int>(request.body.size()))
				is_enough_body_length = false;
			if (request.content_length < static_cast<int>(request.body.size()))
				request.body = request.body.substr(0, request.content_length);
		}
		if (transform_tolower(__l_line.front()) == "connection:" && __l_line.size() == 2 && __l_line.back() == "close")
			client.keep = false;
		if (transform_tolower(__l_line.front()) == "content-type:")
		{
			__l_line.pop_front();
			while (__l_line.size())
			{
				request.content_type += __l_line.front();
				if (__l_line.size() > 2)
					request.content_type += " ";
				__l_line.pop_front();
			}
		}
		if (transform_tolower(__l_line.front()) == "transfer-encoding:" && __l_line.size() == 2 && __l_line.back() == "chunked")
		{
			m_unchunk_after_body_clear(is_enough_body_length);
			request.is_chunk_body = true;
			post_must_have_content_length = true;
			request.content_length = request.body.size();
		}
	}

	if (!post_must_have_content_length)
		request.status_code = 411;
	if (is_enough_body_length)
		client._stage = SET_RESOURCE; // FLAG처리 이상이 있으면 stage그대로 없으면 변경 3번째거로로 SET_RESOURCE
}

void request_msg_parsing(Client& client)
{
	if(client.rq.content_length)
	{
		size_t			body_start_pos = client.request_msg.find("\r\n\r\n") + 4;
		std::string		body = client.request_msg.substr(body_start_pos);
		
		if (client.rq.is_chunk_body)
		{
			std::list<std::string> list = split_line(body);
			client.rq.body.clear();
			size_t last_length = unchunk_data(list, client.rq.body, client.rq.status_code);
			if (client.rq.status_code || last_length == 0)
				client._stage = SET_RESOURCE;
			return ;
		}
		client.rq.body = body;
		if (body.size() >= static_cast<size_t>(client.rq.content_length))
			client._stage = SET_RESOURCE;
		if (body.size() > static_cast<size_t>(client.rq.content_length))
			client.rq.body = body.substr(0, client.rq.content_length);
		return ;
	}
	// 초기 파싱
	Request_parser		parser(client.request_msg);
	parser.parse_request(client);
	client.rq = parser.request;
	if (client.rq.status_code)
		client._stage = SET_RESOURCE;
}
