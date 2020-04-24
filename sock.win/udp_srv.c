
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    WORD sockVersion = MAKEWORD(2, 2);
    
    if(WSAStartup(sockVersion, &wsaData) != 0)
    {
        return 0;
    }

    SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if(serSocket == INVALID_SOCKET)
    {
        printf("socket error !");
        return 0;
    }

    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(8888);
    serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    if(bind(serSocket, (SOCKADDR *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        printf("bind error !");
        closesocket(serSocket);
        return 0;
    }

    struct sockaddr_in remoteAddr;
    int nAddrLen = sizeof(remoteAddr); 
    while (1)
    {
        char recvData[255];  
        int ret = recvfrom(serSocket, recvData, 255, 0, (SOCKADDR *)&remoteAddr, &nAddrLen);
        if (ret > 0)
        {
            recvData[ret] = 0x00;
            printf("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));
            printf(recvData);           
        }

        char * sendData = "一个来自服务端的UDP数据包\n";
        sendto(serSocket, sendData, strlen(sendData), 0, (SOCKADDR *)&remoteAddr, nAddrLen);    

    }
    
    closesocket(serSocket); 
    WSACleanup();
    return 0;
}


