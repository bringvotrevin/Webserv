#include "Config.hpp"
#include "Server.hpp"

Config::Config() { }
Config::~Config() { }

std::vector<Server>& Config::get_server_list() { return __v_server_list; }

std::list<std::string> Config::m_next_line(int brace_check = 0)
{
	std::list<std::string> line;

	while (brace_check >= 0)
	{
		if (__l_file.size() > 1)
			__l_file.pop_front();
		line = __l_file.front();
		if (brace_check && line.front() != "{" && line.size() != 1)
			throw std::invalid_argument("Invalid config file : There is a character after braces");
		brace_check--;
	}
	return (line);
}

void		Config::m_parse_listen(Server& new_server, std::list<std::string>& line)
{
	if (line.size() != 2)
		throw std::invalid_argument("Invalid config file : The number of listen arguments is incorrect");
	line.pop_front();
	std::string str = line.front();
	size_t pos = str.rfind(':');
	if (pos != std::string::npos)
	{
		int dot_count = 0;
		for (size_t i = 0; i < pos; i++)
			if (str[i] == '.')
				dot_count++;
		if (dot_count != 3)
			throw std::invalid_argument("Invalid config file : Host IP is incorrect");
		new_server.listen.first = str.substr(0, pos);
		str = str.substr(pos + 1);
	}
	std::stringstream iss(str);
	int port;
	iss >> port;
	if (iss.fail() || !iss.eof() || port < 0 || port > 49151) // atoi 로는 에러가 나서 0이 return 된건지, 원래 값이 0인건지 구분할 수 없음. 그래서 stringstream 사용
		throw std::invalid_argument("Invalid config file : Port number is incorrect");
	new_server.listen.second = port;
}

void		Config::m_parse_server_name(Server& new_server, std::list<std::string>& line)
{
	if (line.size() != 2)
		throw std::invalid_argument("Invalid config file : The number of server name arguments is incorrect");
	line.pop_front();
	new_server.server_name = line.front();
}

void		Config::m_parse_root(std::list<std::string>& line, Location& loc)
{
	if (line.size() != 2)
		throw std::invalid_argument("Invalid config file : The number of root arguments is incorrect");
	if (line.back().front() == '/')
	{
		loc.root = ".";
		loc.root += line.back();
	}
	else
		loc.root = line.back();
	if (loc.root[loc.root.size() - 1] == '/')
		loc.root = loc.root.substr(0, loc.root.size() - 1);
}

void		Config::m_parse_index(std::list<std::string>& line, Location& loc)
{
	line.pop_front();
	if (line.empty())
		throw std::invalid_argument("Invalid config file : The number of index arguments is incorrect");
	loc.v_index.clear();
	while (line.size())
	{
		loc.v_index.push_back(line.front());
		line.pop_front();
	}
}

void		Config::m_parse_allow_methods(std::list<std::string>& line, Location& loc)
{
	const char *method[3] = {"GET", "POST", "DELETE"};
	int bit[3] = {0x01, 0x01 << 1, 0x01 << 2};
	line.pop_front();
	if (line.empty())
		throw std::invalid_argument("Invalid config file : The number of method arguments is incorrect");
	loc.methods = 0;
	while (line.size())
	{
		int i = 0;
		while (i < 3)
		{
			if (line.front() == method[i])
			{
				loc.methods |= bit[i];
				break ;
			}
			i++;
		}
		if (i == 3)
			throw std::invalid_argument("Invalid config file : invalid method type");
		line.pop_front();
	}
}

void		Config::m_parse_error_page(std::list<std::string>& line, Location& loc)
{
	line.pop_front();
	if (line.size() < 2)
		throw std::invalid_argument("Invalid config file : The number of error page arguments is incorrect");

	// std::ifstream fs;
	// 이전에도 얘기 했지만 config file 의 error_page 는 그 error num 의 페이지를 대체하는 것이다.
	// 근데 이게 default error page 를 말하는 건지 모르겠다...
	// 그리고 우리는 error page 를 만든다... 이때 에러처리를 하면 안 될 것 같은데;;
	// check valid error_page
	// fs.open(line.back(), std::ios::in);
	// if (!fs.is_open())
	//	throw std::invalid_argument("cannot find error_page");
	// fs.close();
	loc.p_error_page.second = line.back();
	line.pop_back();

	loc.p_error_page.first.clear();
	while (line.size())
	{
		int code = atoi(line.front().c_str());
		if (code < 300)
			throw std::invalid_argument("Invalid config file : error_page return number is wrong");
		loc.p_error_page.first.push_back(code);
		line.pop_front();
	}
}

void		Config::m_parse_clent_max_body_size(std::list<std::string>& line, Location& loc)
{
	if (line.size() < 2)
		throw std::invalid_argument("Invalid config file : The number of client_max_body_size arguments is incorrect");
	int size = atoi(line.back().c_str());
	if (size < 256 || size > 100000001)
		throw std::invalid_argument("Invalid config file : Too many or too few size");
	loc.client_max_body_size = size;
}

void		Config::m_parse_auto_index(std::list<std::string>& line, Location& loc)
{
	if (line.size() != 2 || (line.back() != "on" && line.back() != "off"))
				throw std::invalid_argument("Invalid config file : Auto index argument is wrong");
	loc.auto_index = line.back() == "on";
}

void		Config::m_parse_return(std::list<std::string>& line, Location& loc)
{
	line.pop_front();
	int code = atoi(line.front().c_str());

	// code 300 번대 아니면 오류
	if (line.size() != 2 || code < 300 || code > 399)
		throw std::invalid_argument("Invalid config file : Return number is wrong");
	loc.p_return.first = code;

	loc.p_return.second = line.back();
}

void		Config::m_parse_cgi_extension(std::list<std::string>& line, Location& loc)
{
	if (line.size() != 2)
		throw std::invalid_argument("Invalid config file : The number of cgi arguments is incorrect");
	if (line.back().front() != '.')
		throw std::invalid_argument("Invalid config file : Invalid cgi_extension");
	loc.cgi = line.back();
}

Location	Config::m_parse_location(std::list<std::string>& line, Location& loc, Server& new_server)
{
	while (line.front() != "}" && line.front() != "location")
	{
		if (line.front() == "listen" && __s_brace.size() == 1)
			m_parse_listen(new_server, line);
		else if (line.front() == "server_name" && __s_brace.size() == 1)
			m_parse_server_name(new_server, line);
		else if (line.front() == "root")
			m_parse_root(line, loc);
		else if (line.front() == "index")
			m_parse_index(line, loc);
		else if (line.front() == "allow_methods")
			m_parse_allow_methods(line, loc);
		else if (line.front() == "error_page")
			m_parse_error_page(line, loc);
		else if (line.front() == "client_max_body_size")
			m_parse_clent_max_body_size(line, loc);
		else if (line.front() == "auto_index")
			m_parse_auto_index(line, loc);
		else if (line.front() == "cgi_extension" && __s_brace.size() == 2)
			m_parse_cgi_extension(line, loc);
		else if (line.front() == "return" && __s_brace.size() == 2)
			m_parse_return(line, loc);
		else
			throw std::invalid_argument("Invalid config file : This argument not allowed '" + line.front() + "'");
		line = m_next_line(0);
	}
	return loc;
}

void		Config::m_check_location_and_get_method(Server& new_server)
{
	if (new_server.location.empty())
		throw std::invalid_argument("Invalid config file : Default location does not exist");
	for (std::vector<struct Location>::iterator start = new_server.location.begin(); start != new_server.location.end(); start++)
	{
		if (!((*start).methods & 0x01))
			throw std::invalid_argument("Invalid config file : Get method does not exist");
	}
}


void		Config::m_check_route(std::vector<Location>& locs, std::string route)
{
	if (locs.empty())
		return ;
	for (std::vector<Location>::iterator start = locs.begin(); start != locs.end(); start++)
	{
		if ((*start).route == route)
			throw std::invalid_argument("Invalid config file : Duplicate route");
	}
}

void		Config::m_check_default_location(Location& loc)
{
	if (loc.p_error_page.first.front() == -1)
		throw std::invalid_argument("Invalid config file : Error page not found");
	std::vector<int>::iterator end = loc.p_error_page.first.end();
	for (std::vector<int>::iterator begin = loc.p_error_page.first.begin(); begin != end; begin++)
	{
		if (*begin < 400 || *begin > 500)
			throw std::invalid_argument("Invalid config file : Error_page number is incorrect");
	}
	if (loc.v_index.empty())
		throw std::invalid_argument("Invalid config file : Index not found");
	if (loc.root.empty())
		throw std::invalid_argument("Invalid config file : Root not found");
}

void		Config::m_parse_server(std::list<std::string> &line)
{
	Server	new_server;
	line = m_next_line(1);
	Location	default_location;
	new_server.default_location = m_parse_location(line, default_location, new_server);
	m_check_default_location(default_location);
	while (line.front() != "}" && __l_file.size())
	{
		if (line.front() != "location" || line.size() != 2)
			throw std::invalid_argument("Invalid config file : Line is not location");
		Location	new_location(new_server.default_location);
		__s_brace.push("location");
		new_location.route = line.back();
		m_check_route(new_server.location, new_location.route);
		line = m_next_line(1);
		new_server.location.push_back(m_parse_location(line, new_location, new_server));
		if (line.size() != 1)
			throw std::invalid_argument("Invalid config file : There is a character after braces, location part");
		line = m_next_line(0);
		__s_brace.pop();
	}
	m_check_location_and_get_method(new_server);
	__v_server_list.push_back(new_server);
	if (!__l_file.size())
		throw std::invalid_argument("Invalid config file : The file is already over");
	if (line.size() != 1)
		throw std::invalid_argument("Invalid config file : There is a character after braces, server part");
	line = m_next_line(0); // server } 닫는 부분
	__s_brace.pop();
}

void		Config::parse_file()
{
	std::list<std::string> line = __l_file.front();
	while (__l_file.size() > 1)
	{
		if (line.size() != 1 || line.front() != "server")
			throw std::invalid_argument("Invalid config file : Line is not server");
		if (__s_brace.size())
			throw std::invalid_argument("Invalid config file : The braces are not closed.");
		__s_brace.push("server");
		m_parse_server(line);
	}
}

static std::list<std::string> split_line(std::string str)
{
	std::list<std::string>	list;
	std::istringstream		iss(str);
	std::string				buf;

	while(iss >> buf)
		list.push_back(buf);
	return (list);
}

void		Config::open_file(std::string& conf_name)
{
	std::ifstream	fs;

	fs.open(conf_name, std::ios::in);
	if (!fs.is_open())
	{
		std::string conf_root = getcwd(nullptr, 1000);
		conf_root += "/config/" + conf_name;
		fs.open(conf_root);
		if (!fs.is_open())
			throw std::invalid_argument("Conf file does not exist");
	}
	while (!fs.eof())
	{
		std::string str;
		getline(fs >> std::ws, str);
		str = str.substr(0, str.find('#'));
		std::list<std::string> list;
		if (str.size())
		{
			list = split_line(str);
			__l_file.push_back(list);
		}
	}
	fs.close();
}
