#include "IOCP_Echo_Server.h"

int main()
{
	unsigned short serverPort = 6000;
	int backlogQueueSize = SOMAXCONN;
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	int threadPoolSize = info.dwNumberOfProcessors * 2;
	int runningThread = info.dwNumberOfProcessors;
	threadPoolSize = 12;
	runningThread = 8;
	//printf("ThreadPool Size : ");
	//scanf_s("%d", &threadPoolSize);
	//printf("Num of Running Thread : ");
	//scanf_s("%d", &runningThread);
	//IOCP_ECHO_SERVER server(serverPort, SOMAXCONN, threadPoolSize, runningThread, false, 10000);Z
	IOCP_ECHO_SERVER server(0, serverPort, threadPoolSize, runningThread, false, 5000, 200);
	while (1)
	{
		server.All_Moniter();

		int key = _getch();
		if (key == 'w')
		{
			ProfileDataOutText(L"ABC");
		}
		//if (key == 's')
		//{
		//	printf("process Stop START\n");
		//	closesocket(listen_socket);
		//	g_Shutdown = true;
		//	for (int i = 0; i < Thread_Count; i++)
		//		PostQueuedCompletionStatus(IOCP_WORKER_POOL, 0, 0, 0);
		//	break;
		//	//Ư�� Űī ������ SaveThread �� �����.
		//}
		Sleep(0);
	}

	//WaitForMultipleObjects(Thread_Count - 1, _CreateThread, true, INFINITE);//��� �����尡 �����ɶ����� ���Ѵ���ϸ鼭 Ȯ��
	printf("process Stop END\n");
	//���� ����
	WSACleanup();
	printf("MainThread STOP!\n");


	return 0;
}