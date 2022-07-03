#ifndef UTILS_HPP
#define UTILS_HPP

#include "Client.hpp"
#include "Connect.hpp"


void request_msg_parsing(Client& client);
std::string ft_itoa(int len);
std::string ft_itoa2(int len);
void response(Connect& cn, Client& client, Request& request);
void change_events(std::vector<struct kevent>& change_list, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);

#endif