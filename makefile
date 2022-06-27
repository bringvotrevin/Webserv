NAME	= webserv
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRC 	= Config.cpp exec_server.cpp Request_parser.cpp Response.cpp\
		  Server.cpp set_html_file.cpp utils.cpp webserv.cpp Cgi.cpp

SRCDIR	= ./srcs/
SRCS	= $(addprefix $(SRCDIR), $(SRC))
OBJS	= $(SRCS:.cpp=.o)
INCDIR	= ./includes/

%.o:	%.cpp
			c++ $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(NAME): $(OBJS)
	c++ $(CXXFLAGS) -I$(INCDIR) -o $(NAME) $(OBJS)

all: $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all