#include "socketbase.h"
#ifdef _UNIX
#else
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
SocketUdp::SocketUdp()
{
#ifdef _UNIX
        socketfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketfd == -1)
        {
                LOG_ERR("%s", "socket init error");
                exit(1);
        }

        socklen_t optlen = sizeof(int);
        int res = getsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen); //获取发送缓冲区大小
        if (res != -1)
        {
                LOG_INFO("default rcv_size:%d", rcv_size);
        }
        else
                LOG_ERR("%s", "get rcv_size error!");
#else
        WSAData data;
        int ret = WSAStartup(MAKEWORD(2, 2), &data);
        FSERVO_CHECK(ret == 0);

        socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        //int timeout=3;//超时时间3ms
        //ret = setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        //if (ret == -1)
        //{
        //    LOG_ERR("set timeout fail");
        //    exit(-1);
        //}

#endif
}

bool SocketUdp::createUdpServer(const std::string &_ip, unsigned short _port)
{
#ifdef _UNIX
        type = Server;
        memset(&server_addr, 0, sizeof(sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(_port);

        int ret = bind(socketfd, (sockaddr *)&server_addr, sizeof(sockaddr_in));
        if (ret == -1)
        {
                LOG_ERR("%s", "bind error!");
                return false;
        }
        return true;
#else
        type = Server;
        memset(&server_addr, 0, sizeof(sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(_ip.c_str());
        server_addr.sin_port = htons(_port);

        int ret = bind(socketfd, (sockaddr *)&server_addr, sizeof(server_addr));
        if (ret == -1)
        {
                LOG_ERR("%s", "bind error!");
                return false;
        }
        return true;
#endif
}

bool SocketUdp::createUdpClient(std::string &_serverIp, unsigned short _serverPort, unsigned short _selfPort)
{
        // type = Client;
        // memset(&server_addr, 0, sizeof(sockaddr_in));
        // server_addr.sin_family = AF_INET;
        // server_addr.sin_addr.s_addr = inet_addr(_serverIp.data());
        // server_addr.sin_port = htons(_serverPort);

        // return true;

        return createUdpClient(_serverIp.c_str(), _serverPort);
}

bool SocketUdp::createUdpClient(const char *_serverIp, unsigned short _serverPort, unsigned short _selfPort)
{
#ifdef _UNIX
        type = Client;
        memset(&server_addr, 0, sizeof(sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(_serverIp);
        server_addr.sin_port = htons(_serverPort);

        if (_selfPort != 0)
        {
                memset(&client_addr, 0, sizeof(sockaddr_in));
                client_addr.sin_family = AF_INET;
                client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                client_addr.sin_port = htons(_selfPort);
                int ret = bind(socketfd, (sockaddr *)&client_addr, sizeof(sockaddr_in));
        }

        return true;
#else
        type = Client;
        memset(&server_addr, 0, sizeof(sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(_serverIp);
        server_addr.sin_port = htons(_serverPort);

        if (_selfPort != 0)
        {
                memset(&client_addr, 0, sizeof(sockaddr_in));
                client_addr.sin_family = AF_INET;
                client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                client_addr.sin_port = htons(_selfPort);
                int ret = bind(socketfd, (sockaddr *)&client_addr, sizeof(sockaddr_in));
        }
#endif
        return true;
}

int SocketUdp::read(char *pRes, int _size) const
{
        if (!pRes || _size <= 0)
        {
                LOG_ERR("%s", "error in read");
                return -1;
        }
        int len = sizeof(sockaddr_in);
        int num = -1;
        if (type == Client)
        {
                num = recvfrom(socketfd, pRes, _size, 0, (sockaddr *)&server_addr, (socklen_t *)&len);
        }
        else
        {
                num = recvfrom(socketfd, pRes, _size, 0, (sockaddr *)&client_addr, (socklen_t *)&len);
        }
        return num;
}

bool SocketUdp::sendTo(const char *_pData, size_t _size) const
{
        int ret = -1;
        if (type == Client)
        {
                ret = sendto(socketfd, _pData, _size, 0, (sockaddr *)&server_addr, sizeof(sockaddr_in));
        }
        else
        {
                ret = sendto(socketfd, _pData, _size, 0, (sockaddr *)&client_addr, sizeof(sockaddr_in));
        }
        if (ret != _size)
        {
                LOG_ERR("send size:%d", ret);
                return false;
        }
        return true;
}

void SocketUdp::setRecvBuffer(int _byteSize)
{
        socklen_t optlen = sizeof(int);

        int ret = setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &_byteSize, optlen);
        assert(ret == 0);
}

void SocketUdp::setSendBuffer(int _byteSize)
{
        socklen_t optlen = sizeof(int);

        int ret = setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &_byteSize, optlen);
        assert(ret == 0);
}

void SocketUdp::closeSocket()
{
#ifdef _UNIX
        close(socketfd);
#else
        closesocket(socketfd);
        WSACleanup();
#endif
}

/***********************************************************************************************

 *                                              Tcp

***********************************************************************************************/

SocketTcp::SocketTcp()
{
#ifdef _UNIX
        socketfd = socket(AF_INET, SOCK_STREAM, 0);

        if (socketfd == -1)
        {
                LOG_ERR("%s", "socket init error");
                exit(1);
        }

        // socklen_t optlen = sizeof(socklen_t);
        // int RecvBuf = 1024 * 1024 * 10;
        //setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &RecvBuf, sizeof(int));

        socklen_t optlen = sizeof(int);
        int res = getsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen); //获取发送缓冲区大小
        if (res != -1)
        {
                LOG_INFO("default rcv_size:%d", rcv_size);
        }
        else
                LOG_ERR("%s", "get rcv_size error!");
#else
        WSAData data;
        int ret = WSAStartup(MAKEWORD(2, 2), &data);
        FSERVO_CHECK(ret == 0);

        socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (socketfd == -1)
        {
                LOG_ERR("%s", "socket init error");
                exit(-1);
        }

        socklen_t optlen = sizeof(rcv_size);
        int res = getsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_size, &optlen);
        if (res != -1)
        {
                LOG_INFO("default rcv_size:%d", rcv_size);
        }
        else
                LOG_ERR("%s", "get rcv_size error!");
#endif
}

SocketTcp::~SocketTcp()
{
}

void SocketTcp::createTcpServer(const char *_ip, unsigned short _port)
{
#ifdef _UNIX
        type = Server;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        inet_aton(_ip, &server_addr.sin_addr);

        int ret = bind(socketfd, (sockaddr *)&server_addr, sizeof(sockaddr_in));
        FSERVO_CHECK(ret == 0);

        ret = listen(socketfd, 1); //only support one connect
        FSERVO_CHECK(ret == 0);
#else
        type = Server;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        inet_pton(AF_INET, _ip, &server_addr.sin_addr);

        int ret = bind(socketfd, (sockaddr *)&server_addr, sizeof(sockaddr_in));
        FSERVO_CHECK(ret == 0);

        ret = listen(socketfd, 1); //only support one connect
        FSERVO_CHECK(ret == 0);
#endif
}

void SocketTcp::createTcpServer(const std::string &_ip, unsigned short _port)
{
        return createTcpServer(_ip.c_str(), _port);
}

void SocketTcp::waitBeConnected(int time_out)
{
#ifdef _UNIX
        close(socketfd_connect);

        socklen_t len = sizeof(sockaddr_in);
        socketfd_connect = accept(socketfd, (sockaddr *)(&client_addr), &len);

        if (socketfd_connect == -1)
        {
                LOG_ERR("server wait be connected timeout!");
                // exit(-1);
        }
        LOG_INFO("connected from %s", inet_ntoa(client_addr.sin_addr));
#else
        // setSocketTimeout(time_out, 0);

        closesocket(socketfd_connect);

        socklen_t len = sizeof(sockaddr_in);
        socketfd_connect = accept(socketfd, (sockaddr *)(&client_addr), &len);

        if (socketfd_connect == -1)
        {
                LOG_ERR("server wait be connected timeout!");
                // exit(-1);
        }
        LOG_INFO("connected from %s", inet_ntoa(client_addr.sin_addr));

// setSocketTimeout(0, 3000);
#endif
}

bool SocketTcp::connetToServer(std::string &_serverIp, unsigned short _serverPort, int time_out)
{
        return connetToServer(_serverIp.c_str(), _serverPort, time_out);
}

bool SocketTcp::connetToServer(const char *_serverIp, unsigned short _serverPort, int time_out)
{
#ifdef _UNIX
        type = Client;
        setSocketTimeout(time_out, 0);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_serverPort);
        inet_aton(_serverIp, &server_addr.sin_addr);

        socklen_t len = sizeof(sockaddr_in);
        int ret = connect(socketfd, (sockaddr *)&server_addr, len);

        if (ret == -1)
        {
                LOG_ERR("connect to %s failed! port:%d", _serverIp, _serverPort);
                exit(-1);
        }

        LOG_INFO("connect to %s success!", _serverIp);

        setSocketTimeout(0, 0);

        return true;
#else
        type = Client;
        setSocketTimeout(time_out, 0);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_serverPort);
        inet_pton(AF_INET, _serverIp, &server_addr.sin_addr);

        socklen_t len = sizeof(sockaddr_in);
        int ret = connect(socketfd, (sockaddr *)&server_addr, len);

        if (ret == -1)
        {
                LOG_ERR("connect to %s failed! port:%d", _serverIp, _serverPort);
                return false;
        }

        LOG_INFO("connect to %s success!", _serverIp);

        setSocketTimeout(0, 0);

        return true;
#endif
}

bool SocketTcp::sendTo(const char *_pData, size_t _size) const
{
#ifdef _UNIX
        int ret;
        if (type == Client)
        {
                ret = write(socketfd, _pData, _size);
        }
        else
        {
                ret = write(socketfd_connect, _pData, _size);
        }
        return ret == _size;
#else
        int ret;
        if (type == Client)
        {
                //ret = setsockopt(socketfd,SOL_SOCKET,SO_SNDTIMEO,(char*)&time_out_msec,sizeof(time_out_msec));
                //assert(ret == 0);
                ret = send(socketfd, _pData, _size, 0);
        }
        else
        {
                //ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_SNDTIMEO, (char*)&time_out_msec, sizeof(time_out_msec));
                //assert(ret == 0);
                ret = send(socketfd_connect, _pData, _size, 0);
        }
        return ret == _size;
#endif
}

int SocketTcp::read(char *pRes, int _size) const
{
#ifdef _UNIX
        int ret;
        if (type == Client)
        {
                ret = recv(socketfd, pRes, _size, 0);
        }
        else
        {
                ret = recv(socketfd_connect, pRes, _size, 0);
        }
        return ret;
#else
        int ret;
        if (type == Client)
        {
                //ret = setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout_msec,sizeof(timeout_msec));
                //assert(ret == 0);
                ret = recv(socketfd, pRes, _size, 0);
        }
        else
        {
                //ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_msec, sizeof(timeout_msec));
                //assert(ret == 0);
                ret = recv(socketfd_connect, pRes, _size, 0);
        }
        return ret;
#endif
}

void SocketTcp::setRecvBuffer(int _byteSize)
{
#ifdef _UNIX
        int ret;
        socklen_t len = sizeof(int);
        if (Server == type)
        {
                ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_RCVBUF, &_byteSize, len);
                assert(ret == 0);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_RCVBUF, &_byteSize, &len);
                assert(ret == 0);
        }
        else
        {
                ret = setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &_byteSize, len);
                assert(ret == 0);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, &_byteSize, &len);
                assert(ret == 0);
        }
        LOG_INFO("current recv buffer len:%d", _byteSize);
#else
        if (type == Server)
        {
                int ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_RCVBUF, (char *)&_byteSize, sizeof(_byteSize));
                assert(ret == 0);

                socklen_t optlen = sizeof(rcv_size);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_size, &optlen);
                assert(ret == 0);
                LOG_INFO("set rcv_size:%d", rcv_size);
        }
        else
        {
                int ret = setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, (char *)&_byteSize, sizeof(_byteSize));
                assert(ret == 0);

                socklen_t optlen = sizeof(rcv_size);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_size, &optlen);
                assert(ret == 0);
                LOG_INFO("set rcv_size:%d", rcv_size);
        }
#endif
}

void SocketTcp::setSendBuffer(int _byteSize)
{
#ifdef _UNIX
        int ret;
        socklen_t len = sizeof(int);
        if (Server == type)
        {
                ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_SNDBUF, &_byteSize, len);
                if (ret != 0)
                {
                        LOG_ERR("%s", strerror(errno));
                        assert(false);
                }

                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_SNDBUF, &_byteSize, &len);
                assert(ret == 0);
        }
        else
        {
                ret = setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &_byteSize, len);
                assert(ret == 0);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &_byteSize, &len);
                assert(ret == 0);
        }
        LOG_INFO("current send buffer len:%d", _byteSize);
#else
        if (type == Server)
        {
                int ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_SNDBUF, (char *)&_byteSize, sizeof(_byteSize));
                assert(ret == 0);

                socklen_t optlen = sizeof(rcv_size);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_SNDBUF, (char *)&rcv_size, &optlen);
                assert(ret == 0);
                LOG_INFO("set snd_size:%d", rcv_size);
        }
        else
        {
                int ret = setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, (char *)&_byteSize, sizeof(_byteSize));
                assert(ret == 0);

                socklen_t optlen = sizeof(rcv_size);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, (char *)&rcv_size, &optlen);
                assert(ret == 0);
                LOG_INFO("set snd_size:%d", rcv_size);
        }
#endif
}

void SocketTcp::setSendTimeout(int _timeSec, int _timeMsec)
{
#ifdef _UNIX
        int ret;
        struct timeval timeout = {_timeSec, _timeMsec * 1000};
        socklen_t len = sizeof(timeval);
        if (Server == type)
        {
                ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
                assert(ret == 0);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_SNDTIMEO, &timeout, &len);
                assert(ret == 0);
        }
        else
        {
                ret = setsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
                assert(ret == 0);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, &len);
                assert(ret == 0);
        }
        LOG_INFO("current send time out:%d", timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
        int ret;
        int len = sizeof(_timeMsec);
        if (type == Client)
        {
                ret = setsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&_timeMsec, sizeof(_timeMsec));
                assert(ret == 0);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&_timeMsec, &len);
                assert(ret == 0);
        }
        else
        {
                ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_SNDTIMEO, (char *)&_timeMsec, sizeof(_timeMsec));
                assert(ret == 0);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_SNDTIMEO, (char *)&_timeMsec, &len);
                assert(ret == 0);
        }
        LOG_INFO("current send time out:%d", _timeMsec);
#endif
}

void SocketTcp::setRecvTimeout(int _timeSec, int _timeMsec)
{
#ifdef _UNIX
        int ret;
        struct timeval timeout = {_timeSec, _timeMsec * 1000};
        socklen_t len = sizeof(timeval);
        // socklen_t len2 = sizeof(int);
        if (Server == type)
        {
                ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
                assert(ret == 0);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_RCVTIMEO, &timeout, &len);
                assert(ret == 0);
        }
        else
        {
                ret = setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
                assert(ret == 0);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, &len);
                assert(ret == 0);
        }
        LOG_INFO("current recv time out:%d", timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
        int ret;
        int len = sizeof(_timeMsec);
        if (type == Client)
        {
                ret = setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&_timeMsec, sizeof(_timeMsec));
                assert(ret == 0);
                ret = getsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&_timeMsec, &len);
                assert(ret == 0);
        }
        else
        {
                ret = setsockopt(socketfd_connect, SOL_SOCKET, SO_RCVTIMEO, (char *)&_timeMsec, sizeof(_timeMsec));
                assert(ret == 0);
                ret = getsockopt(socketfd_connect, SOL_SOCKET, SO_RCVTIMEO, (char *)&_timeMsec, &len);
                assert(ret == 0);
        }
        LOG_INFO("current recv time out:%d", _timeMsec);
#endif
}

bool SocketTcp::getConnectStatus() const
{
#ifdef _UNIX
        struct tcp_info info;
        int len = sizeof(info);

        if (type == Client)
        {
                getsockopt(socketfd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
        }
        else
        {
                getsockopt(socketfd_connect, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
        }

        if (TCP_ESTABLISHED == info.tcpi_state)
        {
                return CONNECTED;
        }
        else
                return DISCONNECTED;
#else
        char buffer[1];
        if (Server == type)
        {
                recv(socketfd_connect, buffer, 1, MSG_PEEK);
        }
        else
        {
                recv(socketfd, buffer, 1, MSG_PEEK);
        }
        return (WSAECONNRESET != WSAGetLastError());
#endif
}

void SocketTcp::closeSocket()
{
#ifdef _UNIX
        close(socketfd);
        close(socketfd_connect);
#else

        closesocket(socketfd);
        closesocket(socketfd_connect);
#endif
}