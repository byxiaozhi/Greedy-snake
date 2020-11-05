/*
贪吃蛇联机版服务端，55190630朱志放
*/
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <map>
#include <vector>
#include <Windows.h>
#include <iomanip>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
const short BufLen = 2048;
const short BODY_MAX = 500;
char RecvBuf[BufLen];
char SendBuf[BufLen];

class Room {
public:
	map<int, short[BODY_MAX]> bodys;
	map<int, short> body_lens;
	map<int, time_t> times;
	short food = -1;
};
map<int, Room> rooms;

int main()
{
	system("title 贪吃蛇联机版，作者：55190630朱志放");
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in RecvAddr, SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(4000);
	InetPton(PF_INET, TEXT("0.0.0.0"), &RecvAddr.sin_addr.s_addr);
	bind(Socket, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
	cout << "开始监听...";
	while (true) {
		int size = recvfrom(Socket, RecvBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
		int playerId, roomId;
		memcpy(&playerId, RecvBuf + 0 * sizeof(int), sizeof(int));//读取玩家ID
		memcpy(&roomId, RecvBuf + 1 * sizeof(int), sizeof(int));//读取房间ID
		Room& room = rooms[roomId]; //找到对应的房间对象
		if (size == 2 * sizeof(int) + sizeof(short)) { //更新食物的位置
			memcpy(&room.food, RecvBuf + 2 * sizeof(int), sizeof(short));
		}
		else if (size == 2 * sizeof(int) + sizeof(short) + sizeof(short) * BODY_MAX) { //更新蛇的位置
			memcpy(&room.body_lens[playerId], RecvBuf + 2 * sizeof(int), sizeof(short));
			memcpy(&room.bodys[playerId], RecvBuf + 2 * sizeof(int) + sizeof(short), sizeof(short) * BODY_MAX);
		}
		room.times[playerId] = time(NULL);
		map<int, short[BODY_MAX]>::iterator iter;
		for (iter = room.bodys.begin(); iter != room.bodys.end();)//给玩家发送其他玩家的位置
		{
			if (time(NULL) - room.times[iter->first] > 2) {
				room.bodys.erase(iter++);
				continue;
			}
			memcpy(SendBuf, &iter->first, sizeof(int));
			memcpy(SendBuf + sizeof(int), &room.food, sizeof(short));
			memcpy(SendBuf + sizeof(int) + sizeof(short), &room.body_lens[iter->first], sizeof(short));
			memcpy(SendBuf + sizeof(int) + 2 * sizeof(short), &iter->second, sizeof(short) * BODY_MAX);
			sendto(Socket, SendBuf, sizeof(int) + 2 * sizeof(short) + sizeof(short) * BODY_MAX, 0, (SOCKADDR*)&SenderAddr, SenderAddrSize);
			iter++;
		}
	}
	return 0;
}

