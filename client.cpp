/*
贪吃蛇联机版客户端，55190630朱志放
*/
#define HAVE_STRUCT_TIMESPEC
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <fstream>
#include <thread>
#include <Windows.h>
#include <conio.h>
#include <cmath>
#include <iomanip>
#include <string>
#include <mutex>
#include<map>
#include"data.h"
#pragma comment(lib,"ws2_32.lib")
using namespace std;
HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
const short W = 60, H = 30;
const string SNAKE = "■", FOOD = "●", EMPTY = "  ";
const short BufLen = 8192;
const short BODY_MAX = 500;
char SendBuf[BufLen];
short body_len = 0, body[BODY_MAX] = { 0 };
short food = -1, direction = 1;
int playerId, roomId;
bool online = true;
size_t n = 1, m[W * H] = { 0 };
map<int, short[BODY_MAX]> bodys;
map<int, short> body_lens;
map<int, time_t> times;
SOCKET Socket;
sockaddr_in RecvAddr;
mutex rmt, dmt;

void render(int i, string content, WORD color) { //渲染像素点
	rmt.lock();
	SetConsoleTextAttribute(handle, color);
	SetConsoleCursorPosition(handle, { (SHORT)2 * (i % W), (SHORT)i / W });
	if (food >= 0 && food == i && content == EMPTY)
		content = FOOD;
	cout << content;
	rmt.unlock();
}

bool haveBody(int p) {
	for (int i = 0; i < body_len; i++) {
		if (body[i] == p)
			return true;
	}
	dmt.lock();
	bool t = m[p] == n;
	dmt.unlock();
	return t;
}

void show_bodys() {
	dmt.lock();
	n++;
	for (map<int, short[BODY_MAX]>::iterator iter = bodys.begin(); iter != bodys.end();)
	{
		if (time(NULL) - times[iter->first] > 2) {//玩家掉线
			bodys.erase(iter++);
		}
		else {
			for (int i = 0; i < body_lens[iter->first]; i++) {
				short p = bodys[iter->first][i];
				if (m[p] < n - 1)
					render(p, SNAKE, 1);
				m[p] = n;
			}
			iter++;
		}
	}
	for (int i = 0; i < W * H; i++) {
		if (m[i] == n - 1)
			render(i, EMPTY, 1);
	}
	dmt.unlock();
}
void show(string data, WORD color) { //打印文件内容到屏幕
	SetConsoleTextAttribute(handle, color);
	SetConsoleCursorPosition(handle, { 0, 0 });
	cout << data << endl;
}

void makeFood() { //生成一个食物
	render(food, EMPTY, 4);
	short temp_food = rand() % (W * H);
	for (int i = 0; i < body_len; i++) {
		if (haveBody(temp_food) || body[0] + direction == temp_food) {
			temp_food = rand() / (W * H);
			i = -1;
		}
	}
	if (online) {
		memcpy(SendBuf + 0 * sizeof(int), &playerId, sizeof(int));
		memcpy(SendBuf + 1 * sizeof(int), &roomId, sizeof(int));
		memcpy(SendBuf + 2 * sizeof(int), &temp_food, sizeof(short));
		sendto(Socket, SendBuf, 2 * sizeof(int) + sizeof(short), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));//发送食物位置到服务端
	}
	else {
		food = temp_food;
		render(food, FOOD, 4);
	}
}

void sendt() { // 发送数据线程
	while (online) {
		memcpy(SendBuf + 0 * sizeof(int), &playerId, sizeof(int));
		memcpy(SendBuf + 1 * sizeof(int), &roomId, sizeof(int));
		memcpy(SendBuf + 2 * sizeof(int), &body_len, sizeof(short));
		memcpy(SendBuf + 2 * sizeof(int) + sizeof(short), body, sizeof(body));
		sendto(Socket, SendBuf, 2 * sizeof(int) + sizeof(short) + sizeof(body), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
		Sleep(10);
	}
	closesocket(Socket);
}

void recvt() { // 接受数据线程
	sockaddr_in SenderAddr;
	char RecvBuf[BufLen];
	int SenderAddrSize = sizeof(SenderAddr);
	while (online) {
		int size = recvfrom(Socket, RecvBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
		if (size != sizeof(int) + 2 * sizeof(short) + sizeof(body))
			continue;
		int peerId;
		short peer_body[W * H], peer_len, temp_food;
		memcpy(&peerId, RecvBuf, sizeof(int));
		memcpy(&temp_food, RecvBuf + sizeof(int), sizeof(short));
		memcpy(&peer_len, RecvBuf + sizeof(int) + sizeof(short), sizeof(short));
		memcpy(&peer_body, RecvBuf + sizeof(int) + 2 * sizeof(short), sizeof(body));
		if (temp_food < 0 || temp_food >= W * H) {
			makeFood();
		}
		if (temp_food != food) {
			if (!haveBody(food))
				render(food, EMPTY, 0);
			render(temp_food, FOOD, 4);
			food = temp_food;
		}
		if (peerId != playerId) {
			memcpy(&bodys[peerId], peer_body, sizeof(body));
			body_lens[peerId] = peer_len;
			times[peerId] = time(NULL);
		}
		show_bodys();
	}
}

void climb() { //蛇爬
	srand((int)time(NULL));
	body[body_len++] = H / 2 * W + W / 2;
	if (!online)
		makeFood();
	while (true) {
		int next = body[0] + direction;
		if (haveBody(next) || next < 0 || next > W * H || abs(body[0] % W - next % W) == W - 1) //判断
			break;
		if (next == food) { //判断是否吃到食物
			body_len++;
			makeFood();
		}
		else if (body_len > 0) {
			render(body[body_len - 1], EMPTY, 0); //渲染
		}
		for (int i = body_len - 1; i > 0; i--)
			body[i] = body[i - 1]; // 蛇整体向前移动
		body[0] = next;
		render(next, SNAKE, 2); //渲染
		Sleep(100);
	}
	body_len = 0;
	show(gameover, 2);
}

int main()
{
	system("title 贪吃蛇联机版，作者：55190630朱志放");
	show(ready, 2); //载入欢迎界面
	cout << setw(68) << "请输入房间号（0为单人房）：";
	cin >> roomId;
	playerId = (int)time(NULL);
	if (roomId == 0)
		online = false;
	system("cls");
	SetConsoleCursorInfo(handle, new CONSOLE_CURSOR_INFO({ 1, false })); //隐藏光标
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);//初始化Socket
	Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(4000);
	InetPton(PF_INET, TEXT("101.132.253.220"), &RecvAddr.sin_addr.s_addr);	//监听地址
	thread c(climb); //启动蛇爬行线程
	thread s(sendt); //启动数据发送线程
	thread r(recvt);//启动数据接收线程
	while (true) { //持续监控输入
		char ch = _getch();
		if (ch == 'w' && direction != W) direction = -W;
		else if (ch == 's' && direction != -W) direction = W;
		else if (ch == 'a' && direction != 1) direction = -1;
		else if (ch == 'd' && direction != -1) direction = 1;
	}
}