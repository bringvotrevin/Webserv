#include "Client.hpp"

Client::Client() : server(nullptr), file_stream(nullptr) {}

Client::Client(Server *_server) : keep(1), is_io_done(0), origin_fd(0), cgi_pid(0), _stage(GET_REQUEST), server(_server), file_stream(nullptr) {}

void Client::client_clear()
{
	request_msg.clear();
    respond_msg.clear();
    tmp_buffer.clear();
    keep = 1;
	is_io_done = 0;
    origin_fd = 0;
    file_stream = nullptr;
    cgi_pid = 0;
    Request new_rq;
    rq = new_rq;
    rs.clear();
}