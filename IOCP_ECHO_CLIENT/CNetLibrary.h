#pragma once
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"winmm.lib")
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <conio.h>
#include <unordered_map>
#include <Winsock2.h>
#include <cstdlib>
#include <cstdio>
#include <process.h>
#include <windows.h>
#include "CRingBuffer.h"
#include "CMemoryPool.h"
#include "CSerealBuffer.h"
//#include "CDump.h"
#include "CLOG.h"
#include "Profile.h"
#include "CSession.h"