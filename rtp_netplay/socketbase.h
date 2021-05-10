#pragma once

#include "byteArray.h"
#include "log.h"
#include <string>
#ifdef _UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <Ws2tcpip.h>
#include <winsock2.h>
#endif
typedef int SOCK_FD;

enum SocketType
{
        Server,
        Client,
};

class SocketBase
{
protected:
        SocketType type;
        bool isConnected = false;

public:
        SocketBase() {}
        virtual ~SocketBase() {}

public:
        virtual int read(char *pRes, int _size) const = 0;
        //virtual ByteArray read(size_t _readSize) const =0;
        //virtual ByteArray readAll() const=0;
        virtual bool sendTo(const char *_pData, size_t _size) const = 0;
        //virtual bool status() const=0;
        virtual void closeSocket() = 0;
};

class SocketUdp : public SocketBase
{
public:
        SOCK_FD socketfd = -1;

        sockaddr_in server_addr;
        sockaddr_in client_addr;

private:
        std::string ip;
        unsigned short port = 0;
        int rcv_size = 0;
        const int waitConMax = 20;

public:
        SocketUdp();
        ~SocketUdp() {}

public:
        bool createUdpServer(const std::string &_ip, unsigned short _port);
        bool createUdpClient(std::string &_serverIp, unsigned short _serverPort, unsigned short _selfPort = 0);
        bool createUdpClient(const char *_serverIp, unsigned short _serverPort, unsigned short _selfPort = 0);
        bool connetToServer(std::string &_serverIp, unsigned short _serverPort) = delete;
        bool sendTo(const char *_pData, size_t _size) const;
        void setRecvBuffer(int _byteSize);
        void setSendBuffer(int _byteSize);
        ByteArray read(size_t _readSize) const = delete;
        ByteArray readAll() const = delete;
        int read(char *pRes, int _size) const;
        void closeSocket();
};

class SocketTcp : public SocketBase
{
public:
        enum STATUS
        {
                CONNECTED,
                DISCONNECTED,
        };
        SOCK_FD socketfd = -1;
        SOCK_FD socketfd_connect = -1;

        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;

private:
        std::string ip;
        unsigned short port = 0;
        int rcv_size = 0;
        const int waitConMax = 20;

public:
        SocketTcp();
        ~SocketTcp();

public:
        void createTcpServer(const std::string &_ip, unsigned short _port);
        void createTcpServer(const char *_ip, unsigned short _port);
        bool connetToServer(std::string &_serverIp, unsigned short _serverPort, int time_out = 10);
        bool connetToServer(const char *_serverIp, unsigned short _serverPort, int time_out = 10);
        void waitBeConnected(int time_out = 10 * 60);
        bool sendTo(const char *_pData, size_t _size) const;
        void setRecvBuffer(int _byteSize);
        void setSendBuffer(int _byteSize);
        void setSendTimeout(int _timeSec, int _timeMsec);
        void setRecvTimeout(int _timeSec, int _timeMsec);
        bool getConnectStatus() const;
        ByteArray read(size_t _readSize) const = delete;
        ByteArray readAll() const = delete;
        int read(char *pRes, int _size) const;
        void closeSocket();

private:
        inline void setSocketTimeout(int time_sec, int time_usec)
        {
#ifdef _UNIX
                struct timeval timeout;
                timeout.tv_sec = time_sec;
                timeout.tv_usec = time_usec;
                if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
                {
                        LOG_ERR("set socket error!");
                        FSERVO_CHECK(false);
                }
#else
                int msec = time_usec / 1000;
                if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&msec, sizeof(msec)) == -1) //���ý��ճ�ʱ
                {
                        LOG_ERR("set socket error!");
                        FSERVO_CHECK(false);
                }
#endif
        }
};