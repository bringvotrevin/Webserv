# Webserv
42cursus Webserv team project <br>
HTTP server를 C++로 만들어보는 프로젝트 <br>
<br>
Team member : dim, sujikim, saoh, dokkim <br>
How to execute: <br>
` ./webserv [configuration file] ` <br>
기본 conf file은 /config/basic_test.conf로 되어 있음 <br>


### [Mandatory]
<img src="https://user-images.githubusercontent.com/81025416/177487823-e6fb46c4-54e1-4abb-b812-75a238d6dece.png">



### [Requirement]
프로그램은 configuration 파일을 인수로 사용해야 함
클라이언트와 서버 간의 모든 I/O 작업은 poll()만 사용해야 함(poll이라 칭하지만 그와 동등한 select, epoll, kqueue 사용 가능)
polling은 읽기 및 쓰기를 동시에 확인해야 함
poll을 거치지 않고 read/write 금지 (conf 파일 읽는 단계까지는 예외)

자세한 설명은 아래 링크에서 계속 <br>
[Webserv Wiki](https://github.com/dimfrom42/Webserv/wiki)


