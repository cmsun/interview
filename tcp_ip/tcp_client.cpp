/*
********************************************************************************
*                              COPYRIGHT NOTICE
*                             Copyright (c) 2016
*                             All rights reserved
*
*  @FileName       : tcp_client.cpp
*  @Author         : scm 351721714@qq.com
*  @Create         : 2017/09/21 20:53:12
*  @Last Modified  : 2017/09/22 22:35:06
********************************************************************************
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>

#include <vector>

class tcp_client
{
private:
    int m_client_sockfd;
    sockaddr_in m_server_addr;
    pthread_t m_recv_thd;
    std::vector<unsigned char> m_recv_buff;
    bool m_is_connect;

    enum {PRINT_BUFF_LENGTH = 1024};
    enum {RECV_BUFF_LENGTH = 1024};

private:
    static void *recv_process(void *arg);

public:
    tcp_client(void);
    ~tcp_client(void);
    bool connect(in_addr_t addr, uint16_t port);
    bool connect(const char *addr, uint16_t port);
    void send(void *buff, size_t nbytes);
    void print(const char *format, ...);
    virtual void on_receive(unsigned char *buff, size_t nbytes);
};

tcp_client::tcp_client(void)
    :m_client_sockfd(socket(AF_INET, SOCK_STREAM, 0))
    ,m_is_connect(false)
{
    //向一个断开连接的socket写数据会导致系统向程序发送SIGPIPE信号，使程序中止。
    //以下代码使程序忽略SIGPIPE信号。
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, NULL);

    m_recv_buff.resize(RECV_BUFF_LENGTH);

    if(m_client_sockfd == -1)
    {
        printf("create socket error : %d\n", errno);
        return;
    }
    printf("create socket successfully, socket fd : %d\n", m_client_sockfd);

    if(pthread_create(&m_recv_thd, NULL, recv_process, this) == -1)
    {
        printf("create receive thread error : %d !\n", errno);
        return;
    }
    pthread_detach(m_recv_thd);
}

tcp_client::~tcp_client(void)
{
    close(m_client_sockfd);
    pthread_exit(&m_recv_thd);
}

void *tcp_client::recv_process(void *arg)
{
    tcp_client &client = *((tcp_client *)arg);
    std::vector<unsigned char> &buff = client.m_recv_buff;
    struct pollfd pfd;
    pfd.fd = client.m_client_sockfd;
    pfd.events = POLLIN;
    while(1)
    {
        while(!client.m_is_connect) //必须处于连接状态才能读socket，否则会读出大量随机数。
            ;
        if(poll(&pfd, 1, -1) == -1)
            printf("poll error : %d !\n", errno);
        else if(pfd.events & POLLIN)
        {
            size_t nbytes = recv(client.m_client_sockfd, buff.data(), buff.size(), 0);
            if(nbytes > 0)
                client.on_receive(buff.data(), nbytes);
            else    //收到数据为0则说明连接已经断开
            {
                client.m_is_connect = false;
                printf("disconnect form %s\n", inet_ntoa(client.m_server_addr.sin_addr));
                while(!client.connect(client.m_server_addr.sin_addr.s_addr,
                                      client.m_server_addr.sin_port))
                { sleep(2); }
            }
        }
    } 
}

bool tcp_client::connect(in_addr_t addr, uint16_t port)
{
    m_server_addr.sin_family = AF_INET; 
    m_server_addr.sin_port = htons(port);
    m_server_addr.sin_addr.s_addr = addr;
    if(::connect(m_client_sockfd, (struct sockaddr *)&m_server_addr, sizeof(struct sockaddr)) == -1)
    {
        m_is_connect = false;
        printf("connect to %s error : %d !\n", inet_ntoa(m_server_addr.sin_addr), errno);
    }
    else
    {
        m_is_connect = true;
        printf("connect to %s successfully!\n", inet_ntoa(m_server_addr.sin_addr));
    }
    return m_is_connect;
}

inline bool tcp_client::connect(const char *addr, uint16_t port)
{
    return tcp_client::connect(inet_addr(addr), port);
}

inline void tcp_client::send(void *buff, size_t nbytes)
{
    ::send(m_client_sockfd, buff, nbytes, 0);
}

void tcp_client::print(const char *format, ...)
{
    char buff[PRINT_BUFF_LENGTH];
    va_list ap;
    va_start(ap, format);
    int nbytes = vsnprintf(buff, sizeof(buff), format, ap);
    tcp_client::send(buff, nbytes+1);
}

void tcp_client::on_receive(unsigned char *buff, size_t nbytes)
{
    (void)buff;
    printf("receive %lu bytes form %s\n", nbytes, inet_ntoa(m_server_addr.sin_addr));
    printf("%s\n", (char *)buff);
}

int main(int argc, char *argv[])
{
    tcp_client client;
    uint16_t port = 6666;
    client.connect(inet_addr("127.0.0.1"), port);
    int i = 0;
    while(1)
    {
        sleep(1);
        client.print("client send...%d", ++i);
    }
    return 0;
}
