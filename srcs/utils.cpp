#include "Connect.hpp"

std::string ft_itoa(int len)
{
    std::string result;

    if (!len)
        return ("");
    while (len)
    {
        result += len % 10 + '0';
        len /= 10;
    }
    std::reverse(result.begin(), result.end());
    return (result);
}

std::string ft_itoa2(int len)
{
    std::string result;

    if (!len)
        return ("0");
    while (len)
    {
        result += len % 10 + '0';
        len /= 10;
    }
    std::reverse(result.begin(), result.end());
    return (result);
}

void change_events(std::vector<struct kevent>& change_list, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata)
{
    struct kevent temp_event;

    EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
    change_list.push_back(temp_event);
}