/*
********************************************************************************
*                              COPYRIGHT NOTICE
*                             Copyright (c) 2016
*                             All rights reserved
*
*  @FileName       : udp_client.cpp
*  @Author         : scm 351721714@qq.com
*  @Create         : 2017/09/21 20:53:12
*  @Last Modified  : 2017/09/23 18:16:36
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

class udp_client
{
private:
    int m_client_sockfd;
    sockaddr_in m_server_addr;
    pthread_t m_recv_thd;
    std::vector<unsigned char> m_recv_buff;
    bool m_err;

    enum {PRINT_BUFF_LENGTH = 1024};
    enum {RECV_BUFF_LENGTH = 1024};

private:
    bool initialize(in_addr_t addr, uint16_t port);
    static void *recv_process(void *arg);

public:
    udp_client(in_addr_t addr, uint16_t port);
    udp_client(const char *addr, uint16_t port);
    ~udp_client(void);
    bool setaddr(in_addr_t addr, uint16_t port);
    bool setaddr(const char *addr, uint16_t port);
    void send(void *buff, size_t nbytes);
    void print(const char *format, ...);
    void sendto(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes);
    void printto(struct sockaddr *addr, socklen_t addrlen, const char *format, ...);
    virtual void on_receive(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes);
    bool is_error(void);
};

udp_client::udp_client(in_addr_t addr, uint16_t port)
{
    m_err = initialize(addr, port);
    m_recv_buff.resize(RECV_BUFF_LENGTH); //设置接收缓存空间
    if(pthread_create(&m_recv_thd, NULL, recv_process, this) == -1)
    {
        printf("create receive thread error : %d !\n", errno);
        return;
    }
    pthread_detach(m_recv_thd);
}

udp_client::udp_client(const char *addr, uint16_t port)
{
    m_err = initialize(inet_addr(addr), port);
    m_recv_buff.resize(RECV_BUFF_LENGTH); //设置接收缓存空间
    if(pthread_create(&m_recv_thd, NULL, recv_process, this) == -1)
    {
        printf("create receive thread error : %d !\n", errno);
        return;
    }
    pthread_detach(m_recv_thd);
}

udp_client::~udp_client(void)
{
    close(m_client_sockfd);
    pthread_exit(&m_recv_thd);
}

bool udp_client::setaddr(in_addr_t addr, uint16_t port)
{
    close(m_client_sockfd);
    return m_err = initialize(addr, port);
}

bool udp_client::setaddr(const char *addr, uint16_t port)
{
    close(m_client_sockfd);
    return m_err = initialize(inet_addr(addr), port);
}

bool udp_client::initialize(in_addr_t addr, uint16_t port)
{
    if((m_client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("create socket error!\n");
        return false;
    }
    printf("socket id:%d\n", m_client_sockfd);
    m_server_addr.sin_family = AF_INET;     //使用IPV4
    m_server_addr.sin_port = htons(port);   //设置端口
    m_server_addr.sin_addr.s_addr = addr;   //设置IP地址
    printf("local ip:%s port:%d\n", inet_ntoa(m_server_addr.sin_addr), port);
    return true;
}

void *udp_client::recv_process(void *arg)
{
    udp_client &client = *((udp_client *)arg);
    std::vector<unsigned char> &buff = client.m_recv_buff;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    while(1)
    {
        size_t nbytes = recvfrom(client.m_client_sockfd,
                                 buff.data(),
                                 buff.size(),
                                 0,
                                 (struct sockaddr *)&addr,
                                 &addrlen);
        client.on_receive((struct sockaddr *)&addr, addrlen, buff.data(), nbytes);
    } 
}

inline void udp_client::send(void *buff, size_t nbytes)
{
    ::sendto(m_client_sockfd, buff, nbytes, 0,
            (struct sockaddr *)&m_server_addr, sizeof(m_server_addr));
}

void udp_client::print(const char *format, ...)
{
    char buff[PRINT_BUFF_LENGTH];
    va_list ap;
    va_start(ap, format);
    int nbytes = vsnprintf(buff, sizeof(buff), format, ap);
    udp_client::send(buff, nbytes+1);
}

inline void udp_client::sendto(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes)
{
    ::sendto(m_client_sockfd, buff, nbytes, 0, addr, addrlen);
}

void udp_client::printto(struct sockaddr *addr, socklen_t addrlen, const char *format, ...)
{
    char buff[PRINT_BUFF_LENGTH];
    va_list ap;
    va_start(ap, format);
    int nbytes = vsnprintf(buff, sizeof(buff), format, ap);
    udp_client::sendto(addr, addrlen, buff, nbytes+1);
}

void udp_client::on_receive(struct sockaddr *addr, socklen_t addrlen, void *buff, size_t nbytes)
{
    (void)addr;
    (void)addrlen;
    (void)buff;
    printf("receive %lu bytes form %s\n", nbytes, inet_ntoa(m_server_addr.sin_addr));
    printf("%s\n", (char *)buff);
}

bool udp_client::is_error(void)
{
    return m_err;
}

int main(int argc, char *argv[])
{
    udp_client client("127.0.0.1", 6666);
    int i = 0;
    while(1)
    {
        sleep(1);
        client.print("client send...%d", ++i);
    }
    return 0;
}
