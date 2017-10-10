/*
********************************************************************************
*                              COPYRIGHT NOTICE
*                             Copyright (c) 2016
*                             All rights reserved
*
*  @FileName       : tcp_server.cpp
*  @Author         : scm 351721714@qq.com
*  @Create         : 2017/09/21 06:18:04
*  @Last Modified  : 2017/10/10 20:46:24
********************************************************************************
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <list>
#include <vector>
#include <algorithm>

class tcp_server
{
private:
    int m_server_sockfd;                    //服务端套接字描述符
    std::list<int> m_client_sockfd;         //接入连接的客户端的套接字描述符
    struct sockaddr_in m_server_addr;       //本设备的IP地址
    pthread_t m_accept_thd;                 //接收客户端连接的线程
    pthread_t m_recv_thd;                   //接收数据的线程
    int m_epoll_fd;                         //epoll的文件描述符
    std::vector<unsigned char> m_recv_buff; //接收数据的缓冲区
    bool m_err;

    enum {RECV_BUFF_LENGTH = 1024};
    enum {EPOLL_INIT_SIZE = 1024};
    enum {LISTEN_SIZE = 10};
    enum {PRINT_BUFF_LENGTH = 1024};

private:
    void initialize(in_addr_t addr, uint16_t port);
    static void *accept_process(void *arg);
    static void *recv_process(void *arg);
    int add_to_epoll(int cli_fd);

public:
    tcp_server(in_addr_t addr = INADDR_ANY, uint16_t port = -1);
    tcp_server(const char *addr/*  = "0.0.0.0" */, uint16_t port = -1);
    ~tcp_server(void);
    void resize_recv_buff(size_t len);
    size_t recv_buff_size(void);
    virtual void on_receive(int sockfd, void *buff, size_t nbytes);
    void send(int sockfd, void *buff, size_t nbytes);
    void print(int sockfd, const char *format, ...);
    bool is_error(void);
};

tcp_server::tcp_server(in_addr_t addr, uint16_t port)
    :m_server_sockfd(socket(AF_INET, SOCK_STREAM, 0))   //使用IPV4以及TCP协议创建套接字
    ,m_epoll_fd(epoll_create(EPOLL_INIT_SIZE))  //EPOLL_INIT_SIZE不是指定epoll能监听的事件数量，
                                                //而是对内核初始化epoll数据结构的建议。
                                                //linux2.6.8以后这个参数被忽略，只要大于0即可。
    ,m_err(m_server_sockfd == -1 || m_epoll_fd == -1)
{
    initialize(addr, port);
}

tcp_server::tcp_server(const char *addr, uint16_t port)
    :m_server_sockfd(socket(AF_INET, SOCK_STREAM, 0))   //使用IPV4以及TCP协议创建套接字
    ,m_epoll_fd(epoll_create(EPOLL_INIT_SIZE))  //EPOLL_INIT_SIZE不是指定epoll能监听的事件数量，
                                                //而是对内核初始化epoll数据结构的建议。
                                                //linux2.6.8以后这个参数被忽略，只要大于0即可。
    ,m_err(m_server_sockfd == -1 || m_epoll_fd == -1)
{
    initialize(inet_addr(addr), port);
}

void tcp_server::initialize(in_addr_t addr, uint16_t port)
{
    //向一个断开连接的socket写数据会导致系统向程序发送SIGPIPE信号，使程序中止。
    //以下代码使程序忽略SIGPIPE信号。
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, NULL);

    m_recv_buff.resize(RECV_BUFF_LENGTH); //设置接收缓存空间

    if(m_server_sockfd == -1)
    {
        printf("create socket error!\n");
        return;
    }
    printf("socket id:%d\n", m_server_sockfd);

    if(m_epoll_fd == -1)
    {
        printf("create epoll error!\n");
        return;
    }
    printf("epoll fd:%d\n", m_epoll_fd);

    m_server_addr.sin_family = AF_INET;     //使用IPV4
    m_server_addr.sin_port = htons(port);   //设置端口
    m_server_addr.sin_addr.s_addr = addr;   //设置IP地址
    printf("local ip:%s port:%d\n", inet_ntoa(m_server_addr.sin_addr), port);

    //套接字与端口绑定
    if(bind(m_server_sockfd, (struct sockaddr *)&m_server_addr, sizeof(m_server_addr)) == -1)
    {
        printf("bind socket error : %d !\n", errno);
        m_err = false;
        return;
    }
    printf("bind socket successfully.\n");

    //第二个参数不是能连接的客户端的数量，而是指同一时间能接受的连接请求的数量。
    if(listen(m_server_sockfd, LISTEN_SIZE) == -1)
    {
        printf("listen error: %d !\n", errno);
        m_err = false;
        return;
    }
    printf("listenning ...\n");

    if(pthread_create(&m_accept_thd, NULL, accept_process, this) == -1)
    {
        printf("create accept thread error : %d !\n", errno);
        m_err = false;
        return;
    }

    if(pthread_create(&m_recv_thd, NULL, recv_process, this) == -1)
    {
        printf("create receive thread error : %d !\n", errno);
        m_err = false;
        return;
    }
}

tcp_server::~tcp_server(void)
{
    close(m_server_sockfd);
    for(std::list<int>::iterator itr = m_client_sockfd.begin();
        itr != m_client_sockfd.end();
        ++itr)
    {
        close(*itr);
    }
    close(m_epoll_fd);
    pthread_cancel(&m_accept_thd);
    pthread_cancel(&m_recv_thd);
}

void *tcp_server::accept_process(void *arg)
{
    pthread_detach(pthread_self());

    int client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    tcp_server &server = *(tcp_server *)arg;
    int server_sockfd = server.m_server_sockfd;
    while(1)
    {
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if(client_sockfd != -1)
        {
            server.m_client_sockfd.push_back(client_sockfd);
            server.add_to_epoll(client_sockfd);
            printf("connect from %s, new socket fd:%d\n",
                   inet_ntoa(client_addr.sin_addr), client_sockfd);
        }
    }
}

void *tcp_server::recv_process(void *arg)
{
    pthread_detach(pthread_self());

    tcp_server &server = *(tcp_server *)arg;
    std::list<int> &client_sockfd = server.m_client_sockfd;
    std::vector<unsigned char> &buff = server.m_recv_buff;
    int epoll_fd = server.m_epoll_fd;
    struct epoll_event event;
    while(1)
    {
        if(epoll_wait(epoll_fd, &event, 2, -1) == -1)
        {
            printf("epoll wait error: %d !\n", errno);
            while(1)
                ;
        }
        size_t nbytes = recv(event.data.fd, buff.data(), buff.size(), 0);
        if(nbytes > 0)
            server.on_receive(event.data.fd, buff.data(), nbytes);
        else //接收到0个字节，表明断开连接
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event.data.fd, &event);  //从epoll监听队列中删除
            close(event.data.fd);                                       //关闭断开连接的客户端的文件描述符
            std::list<int>::iterator iter = 
                std::find(client_sockfd.begin(), client_sockfd.end(), event.data.fd);
            client_sockfd.erase(iter);                                  //删除断开连接的客户端的文件描述符
            printf("disconnet client socket fd %d\n", event.data.fd);
        }
    }
}

int tcp_server::add_to_epoll(int cli_fd)
{
    struct epoll_event event;
    event.data.fd = cli_fd;     //要加入监听队列的文件描述符
    event.events = EPOLLIN;     //要监听的事件为可读事件
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, cli_fd, &event);
    if(ret == 0)
        printf("add client socket %d to epoll successfuly!\n", cli_fd);
    else
        printf("add client socket %d to epoll error!\n", cli_fd);
    return ret;
}

inline void tcp_server::resize_recv_buff(size_t len)
{
    m_recv_buff.resize(len);
}

inline size_t tcp_server::recv_buff_size(void)
{
    return m_recv_buff.size();
}

void tcp_server::on_receive(int sockfd, void *buff, size_t nbytes)
{
    (void)buff;
    printf("from socket %d receive %lu bytes.\n", sockfd, nbytes);
}

void tcp_server::send(int sockfd, void *buff, size_t nbytes)
{
    if(!m_err) ::send(sockfd, buff, nbytes, 0);
}

void tcp_server::print(int sockfd, const char *format, ...)
{
    char buff[PRINT_BUFF_LENGTH];
    va_list ap;
    va_start(ap, format);
    int nbytes = vsnprintf(buff, sizeof(buff), format, ap);
    tcp_server::send(sockfd, buff, nbytes+1);
}

inline bool tcp_server::is_error(void)
{
    return m_err;
}

int main(int argc, char *argv[])
{
    class TCP_SERVER : public tcp_server
    {
        public:
            TCP_SERVER(const char *addr, uint16_t port) : tcp_server(addr, port) {}
            virtual void on_receive(int sockfd, void *buff, size_t nbytes)
            {
                printf("%s\n", (char *)buff);
                tcp_server::print(sockfd, "server send ...");
            }
    };

    TCP_SERVER ser("127.0.0.1", 6666);

    while(1)
        ;
    return 0;
}
