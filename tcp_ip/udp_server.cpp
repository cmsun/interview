/*
********************************************************************************
*                              COPYRIGHT NOTICE
*                             Copyright (c) 2016
*                             All rights reserved
*
*  @FileName       : udp_server.cpp
*  @Author         : scm 351721714@qq.com
*  @Create         : 2017/09/21 06:18:04
*  @Last Modified  : 2017/10/10 15:33:43
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

class udp_server
{
private:
    int m_server_sockfd;                    //服务端套接字描述符
    struct sockaddr_in m_server_addr;       //本设备的IP地址
    pthread_t m_recv_thd;                   //接收数据的线程
    std::vector<unsigned char> m_recv_buff; //接收数据的缓冲区
    bool m_err;

    enum {RECV_BUFF_LENGTH = 1024};
    enum {PRINT_BUFF_LENGTH = 1024};

private:
    void initialize(in_addr_t addr, uint16_t port);
    static void *recv_process(void *arg);

public:
    udp_server(in_addr_t addr = INADDR_ANY, uint16_t port = -1);
    udp_server(const char *addr/*  = "0.0.0.0" */, uint16_t port = -1);
    ~udp_server(void);
    void resize_recv_buff(size_t len);
    size_t recv_buff_size(void);
    virtual void on_receive(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes);
    void sendto(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes);
    void print(struct sockaddr *addr, socklen_t addrlen, const char *format, ...);
    bool is_error(void);
};

udp_server::udp_server(in_addr_t addr, uint16_t port)
    :m_server_sockfd(socket(AF_INET, SOCK_DGRAM, 0))   //使用IPV4以及UDP协议创建套接字
    ,m_err(m_server_sockfd == -1)
{
    initialize(addr, port);
}

udp_server::udp_server(const char *addr, uint16_t port)
    :m_server_sockfd(socket(AF_INET, SOCK_DGRAM, 0))   //使用IPV4以及UDP协议创建套接字
    ,m_err(m_server_sockfd == -1)
{
    initialize(inet_addr(addr), port);
}

void udp_server::initialize(in_addr_t addr, uint16_t port)
{
    m_recv_buff.resize(RECV_BUFF_LENGTH); //设置接收缓存空间

    if(m_server_sockfd == -1)
    {
        printf("create socket error!\n");
        return;
    }
    printf("socket id:%d\n", m_server_sockfd);

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

    if(pthread_create(&m_recv_thd, NULL, recv_process, this) == -1)
    {
        printf("create receive thread error : %d !\n", errno);
        m_err = false;
        return;
    }
    pthread_detach(m_recv_thd);
}

udp_server::~udp_server(void)
{
    close(m_server_sockfd);
    pthread_exit(&m_recv_thd);
}

void *udp_server::recv_process(void *arg)
{
    pthread_detach(pthread_self());

    udp_server &server = *(udp_server *)arg;
    std::vector<unsigned char> &buff = server.m_recv_buff;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    while(1)
    {
        int nbytes = recvfrom(server.m_server_sockfd,
                              buff.data(),
                              buff.size(),
                              0,
                              (struct sockaddr *)&addr,
                              &addrlen);
        server.on_receive((struct sockaddr *)&addr, addrlen, buff.data(), nbytes);
    }
}

inline void udp_server::resize_recv_buff(size_t len)
{
    m_recv_buff.resize(len);
}

inline size_t udp_server::recv_buff_size(void)
{
    return m_recv_buff.size();
}

void udp_server::on_receive(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes)
{
    (void)addr;
    (void)addrlen;
    (void)buff;
    printf("from ip %s receive %lu bytes.\n", inet_ntoa(((struct sockaddr_in *)addr)->sin_addr), nbytes);
}

void udp_server::sendto(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes)
{
    if(!m_err) ::sendto(m_server_sockfd, buff, nbytes, 0, addr, addrlen);
}

void udp_server::print(struct sockaddr *addr, socklen_t addrlen, const char *format, ...)
{
    char buff[PRINT_BUFF_LENGTH];
    va_list ap;
    va_start(ap, format);
    int nbytes = vsnprintf(buff, sizeof(buff), format, ap);
    udp_server::sendto(addr, addrlen, buff, nbytes+1);
}

inline bool udp_server::is_error(void)
{
    return m_err;
}

int main(int argc, char *argv[])
{
    class UDP_SERVER : public udp_server
    {
        public:
            UDP_SERVER(const char *addr, uint16_t port) : udp_server(addr, port) {}
            virtual void on_receive(struct sockaddr *addr,
                                    socklen_t addrlen, void *buff, size_t nbytes)
            {
                printf("%s\n", (char *)buff);
                udp_server::print(addr, addrlen, "server send ...");
            }
    };

    UDP_SERVER ser("127.0.0.1", 6666);

    while(1)
        ;
    return 0;
}
