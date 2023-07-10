#pragma once

bool netPacket_ReqLogin(Session* pSession, CSerealBuffer* pPacket);//로그인에대한 반응
void Send_ResLogin(Session* pSession,char Result);//로그인후 반응
void MakePacket_Send_ResLogin(CSerealBuffer* pPacket, int _User_No, char Result);//로그인 후 반응 패킷만들기 

bool netPacket_ReqRoomList(Session* pSession, CSerealBuffer* pPacket);
void Send_ResRoomList(Session* pSession);
void MakePacket_ResRoomList(CSerealBuffer* pPacket);


bool netPacket_ReqRoomCreate(Session* pSession, CSerealBuffer* pPacket);
void Send_ResRoomCreate(Session* pSession, ROOM* pRoom, char Result);
void MakePacket_ResRoomCreate(CSerealBuffer* pPacket, ROOM* pRoom, char Result);

bool netPacket_ReqRoomEnter(Session* pSession, CSerealBuffer* pPacket);
void Send_ResRoomEnter(Session* pSession, ROOM* pRoom, char Result);//로그인후 반응
void MakePacket_Send_RoomEnter(CSerealBuffer* pPacket, ROOM* Room_Info, char Result);//로그인 후 반응 패킷만들기 


bool netPacket_ReqReqChat(Session* pSession, CSerealBuffer* pPacket);
void Send_ResReqChat(Session* pSession, ROOM* pRoom, char Result);//로그인후 반응
void MakePacket_Send_ReqChat(CSerealBuffer* pPacket, ROOM* Room_Info, char Result);//로그인 후 반응 패킷만들기 


bool netPacket_ReqReqRoomLeave(Session* pSession, CSerealBuffer* pPacket);


bool netPacket_StressTest(Session* pSession, CSerealBuffer* pPacket);
void MakePacket_Send_StressTest(CSerealBuffer* pPacket, WORD arr_size, WCHAR* _arr_stress);
void Send_ResStressTest(Session* pSession, WORD arr_size, WCHAR* _arr_stress);

bool netPacket_ReqLogin(Session* pSession, CSerealBuffer* pPacket)
{

	if (pSession->_Login_check == true)//로그인 했는데 또 로그인 할필요가 없다.
	{
		return false;//로그인했는데 왜 또 로그인요청을하냐 답변조차 안한다.
	}
	
	int Result = df_RESULT_LOGIN_OK;
	//기타오류
	WCHAR NICK[dfNICK_MAX_LEN];
	pPacket->GetData((char*)NICK, sizeof(NICK));

	if (_Nick_Check(NICK))//닉네임 확인
	{
		Result = df_RESULT_LOGIN_DNICK;
		wprintf(L"닉네임중복\n");
	}

	if (User_List.size() > dfUSER_MAX_LEN)//닉네임 확인
	{
		Result = df_RESULT_LOGIN_MAX;
	}
	
	wcscpy_s(pSession->_Nick_Name, NICK);//유저 로그인 및 닉네임 세팅
	pSession->_Login_check = true;//로그인이 완료되었다는 뜻이다.
	
	Send_ResLogin(pSession, Result);

	return true;
};

void Send_ResLogin(Session* pSession, char Result)
{
	CSerealBuffer Packet;
	MakePacket_Send_ResLogin(&Packet, pSession->_User_No, Result);
	sendUniCast(pSession, &Packet);
}

void MakePacket_Send_ResLogin(CSerealBuffer* pPacket, int _User_No,char Result)
{
	st_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfPACKET_CODE;
	
	stPacketHeader.wPayloadSize = sizeof(Result) + sizeof(_User_No);
	stPacketHeader.wMsgType = df_RES_LOGIN;

	//체크섬 생성을 위한 부분
	CSerealBuffer Packet;
	Packet.PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
	Packet << Result << _User_No;
	stPacketHeader.byCheckSum = _Ret_Checksum(stPacketHeader.wMsgType, stPacketHeader.wPayloadSize, &Packet);
	
	pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
	*pPacket << Result << _User_No;
	
};


bool netPacket_ReqRoomList(Session* pSession, CSerealBuffer* pPacket)
{
	if (pSession->_Login_check == false)//로그인도 안됬는데 뭔 놈의 요청이여
	{
		return false;//로그인했는데 왜 또 로그인요청을하냐 답변조차 안한다.
	}
	
	Send_ResRoomList(pSession);

	return true;
};


void Send_ResRoomList(Session* pSession) 
{
	CSerealBuffer Packet(10000);//일단 귀찮으니 10000바이트 잡고 시작해
	MakePacket_ResRoomList(&Packet);

	sendUniCast(pSession, &Packet);
};

void MakePacket_ResRoomList(CSerealBuffer* pPacket)
{
	st_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfPACKET_CODE;
	stPacketHeader.wMsgType = df_RES_ROOM_LIST;
	//stPacketHeader.byCheckSum
	//stPacketHeader.wPayloadSize
	
	short Room_List_size = Room_List.size();

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));//헤더넣기
	*pPacket << Room_List_size;

	int Room_No;
	short Room_Name_Size;
	WCHAR szRoomName[dfROOM_MAX_LEN] = { 0, };
	char UserListSize = 0;

	for (auto Room_it = Room_List.begin(); Room_it != Room_List.end(); ++Room_it)
	{
		//wcscpy_s(szRoomName, Room_it->second->_RoomName);//유저 로그인 및 닉네임 세팅
		Room_Name_Size = wcslen(Room_it->second->_RoomName) * 2;

		*pPacket << Room_it->second->_Room_No;
		*pPacket << Room_Name_Size;
		pPacket->PutData((char*)Room_it->second->_RoomName, Room_Name_Size);
		*pPacket << UserListSize;
		
	}
	stPacketHeader.wPayloadSize = pPacket->GetUseSize() - sizeof(st_PACKET_HEADER);
	*(pPacket->GetBufferPtr() + 4) = stPacketHeader.wPayloadSize;//패킷사이즈위치

	//체크섬 세팅 하기귀찮아서 포인터 직접조정함
	//이따구로 하면안됨
	*(pPacket->GetBufferPtr() + 1) = _Ret_Checksum(stPacketHeader.wMsgType, stPacketHeader.wPayloadSize, pPacket);
};


bool netPacket_ReqRoomCreate(Session* pSession, CSerealBuffer* pPacket)
{
	WORD wTitleSize;
	WCHAR szRoomTilte[dfROOM_MAX_LEN] = {0,};
	int Result = df_RESULT_ROOM_CREATE_OK;
	
	*pPacket >> wTitleSize;
	pPacket->GetData((char*)szRoomTilte, wTitleSize);

	if (pSession->_Login_check == false)
	{
		Disconnect(pSession);
		return false;//로그인도 안되있는데 왜 요청을 계속보내?
	}

	if (pSession->_Connect_Room_No != dfdefaultRoom)
	{
		return false;//방에 들어가있는데 어떻게 방요청을 보내냐?
	}

	if (_Room_Check(szRoomTilte))//닉네임 확인
	{
		Result = df_RESULT_ROOM_CREATE_DNICK;
		return false;
	}

	else if (User_List.size() > dfUSER_MAX_LEN)//닉네임 확인
	{
		Result = df_RESULT_ROOM_CREATE_MAX;
		return false;
	}

	//방생성작업
	ROOM* pRoom = chat_RoomPool.Alloc();
	memset(pRoom->_RoomName,0, sizeof(WCHAR) * 256);

	pRoom->_Room_No = Room_Id_Count;
	Room_Id_Count++;
	wcscpy_s(pRoom->_RoomName, szRoomTilte);
;
	Room_List.emplace(make_pair(pRoom->_Room_No, pRoom));
	wprintf(L"방만들어졌습니다.\n");

	Send_ResRoomCreate(pSession, pRoom,Result);
	
	return true;
};

void Send_ResRoomCreate(Session* pSession, ROOM* pRoom, char Result)
{
	CSerealBuffer Packet;
	MakePacket_ResRoomCreate(&Packet, pRoom, Result);
	//memset(Packet.GetBufferPtr()+1,); ;

	sendBroadCast(nullptr, &Packet);
};

void MakePacket_ResRoomCreate(CSerealBuffer* pPacket, ROOM* pRoom, char Result)
{

	short title_size = (wcslen(pRoom->_RoomName)*2);

	st_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfPACKET_CODE;
	stPacketHeader.wMsgType = df_RES_ROOM_CREATE;

	//체크섬 생성을 위한 부분
	CSerealBuffer Packet;
	Packet.PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
	Packet << Result << pRoom->_Room_No << title_size;
	Packet.PutData((char*)pRoom->_RoomName, title_size);

	stPacketHeader.wPayloadSize = Packet.GetUseSize() - sizeof(st_PACKET_HEADER);

	
	pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));//헤더넣기
	*pPacket << Result << pRoom->_Room_No << title_size;//이름세팅
	pPacket->PutData((char*)pRoom->_RoomName, title_size);

	*(pPacket->GetBufferPtr() + 1) = _Ret_Checksum(stPacketHeader.wMsgType, stPacketHeader.wPayloadSize, &Packet);
};

bool netPacket_ReqRoomEnter(Session* pSession, CSerealBuffer* pPacket)
{
	int Result = df_RESULT_ROOM_ENTER_OK;
	
	if (pSession->_Login_check == false)
	{
		Disconnect(pSession);
		return false;//로그인도 안되있는데 왜 요청을 계속보내?
	}

	if (pSession->_Connect_Room_No != dfdefaultRoom)
	{
		return false;//방에 들어가있는데 어떻게 방요청을 보내냐?
	}

	//if (_Room_Check(szRoomTilte))//같은방에 있으면 다시 못들어간다.
	//{
	//	Result = df_RESULT_ROOM_CREATE_DNICK;
	//	return false;
	//}

	

	DWORD Room_NUM;
	*pPacket >> Room_NUM;
	
	ROOM* pRoom = Find_Room(Room_NUM);
	
	if (pRoom == nullptr)
	{
		Result = df_RESULT_ROOM_ENTER_NOT;
	}

	if (Room_List.size() >= dfROOM_MAX_LEN)//방최대치 이상이면
	{
		Result = df_RESULT_ROOM_ENTER_MAX;
	}
	pRoom->_Room_UserList.push_back(pSession);//만든방에 들어간 유저정보를 넣는다.
	pSession->_Connect_Room_No = pRoom->_Room_No;//들어간방을 업데이트한다.
	Send_ResRoomEnter(pSession, pRoom, Result);

	return true;
};

void Send_ResRoomEnter(Session* pSession, ROOM* pRoom,char Result) 
{	
	CSerealBuffer Packet;
	MakePacket_Send_RoomEnter(&Packet, pRoom, Result);
	sendUniCast(pSession, &Packet);

	//다른사용자가 입장했을때다른유저도 알필요가 있다.
	//CSerealBuffer UserPacket;

	//sendBroadCast(pSession, &UserPacket);//나를제외하고 다른사람에게도 알린다.

};//로그인후 반응

void MakePacket_Send_RoomEnter(CSerealBuffer* pPacket, ROOM* Room_Info, char Result)
{
	st_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfPACKET_CODE;
	stPacketHeader.wMsgType = df_RES_ROOM_ENTER;
	stPacketHeader.byCheckSum = (BYTE)((stPacketHeader.wMsgType + Result) % 256);
	stPacketHeader.wPayloadSize = sizeof(Result);
	
	pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));//헤더넣기
	*pPacket << Result;
	
	if (Result != df_RESULT_ROOM_ENTER_OK)
	{
		return;
	}

	int Room_No;
	short Room_Name_Size;
	WCHAR szRoomName[dfROOM_MAX_LEN] = { 0, };
	WCHAR szNickName[dfNICK_MAX_LEN] = { 0, };
	char UserListSize = 0;

	Room_Name_Size = wcslen(Room_Info->_RoomName) * 2;
	*pPacket << Room_Info->_Room_No;
	*pPacket << Room_Name_Size;
	pPacket->PutData((char*)Room_Info->_RoomName, Room_Name_Size);

	UserListSize = Room_Info->_Room_UserList.size();
	*pPacket << UserListSize;

	int session_User_No = 0;
	for (auto session_it = Room_Info->_Room_UserList.begin(); session_it != Room_Info->_Room_UserList.end(); ++session_it)
	{
		wcscpy_s(szNickName, (*session_it)->_Nick_Name);
		session_User_No = (*session_it)->_User_No;

		pPacket->PutData((char*)szNickName, sizeof(WCHAR) * dfNICK_MAX_LEN);
		*pPacket << session_User_No;

		memset(szNickName,0,sizeof(szNickName));
	}

	stPacketHeader.wPayloadSize = pPacket->GetUseSize() - sizeof(st_PACKET_HEADER);
	*(pPacket->GetBufferPtr() + 4) = stPacketHeader.wPayloadSize;//패킷사이즈위치

	//체크섬 세팅 하기귀찮아서 포인터 직접조정함
	//이따구로 하면안됨
	*(pPacket->GetBufferPtr() + 1) = _Ret_Checksum(stPacketHeader.wMsgType, stPacketHeader.wPayloadSize, pPacket);

};//로그인 후 반응 패킷만들기 

bool netPacket_ReqReqChat(Session* pSession, CSerealBuffer* pPacket)
{
	//int Result = df_RESULT_ROOM_ENTER_OK;

	if (pSession->_Login_check == false)
	{
		Disconnect(pSession);
		return false;//로그인도 안되있는데 왜 요청을 계속보내?
	}

	if (pSession->_Connect_Room_No == dfdefaultRoom)
	{
		Disconnect(pSession);
		return false;//방에 안들어가있는데 메세지를 보내는것이므로 false
	}
	
	//방에 있는지도 확인해야된다.
	//아직안했다.



	//Send_ResReqChat(pSession,);

	//WORD msgSize;
	//WCHAR msg[512] = {0,};

	//CSerealBuffer Packet;
	//DWORD User_Num;
	//WORD msg_Num;

	//*pPacket >> msgSize;
	//pPacket->GetData((char*)msg, msgSize);


	//User_Num = pSession->_User_No;

	//Packet << User_Num;

	//sendBroadCast();
	return true;
};

void Send_ResReqChat(Session* pSession, ROOM* pRoom, char Result)
{
	CSerealBuffer Packet;
	MakePacket_Send_ReqChat(&Packet, pRoom, Result);


	sendUniCast(pSession, &Packet);
};
void MakePacket_Send_ReqChat(CSerealBuffer* pPacket, ROOM* Room_Info, char Result)
{
	
};

bool netPacket_ReqReqRoomLeave(Session* pSession, CSerealBuffer* pPacket)
{
	return true;
};


bool netPacket_StressTest(Session* pSession, CSerealBuffer* pPacket)
{
	//int Result = df_RESULT_ROOM_ENTER_OK;

	//if (pSession->_Login_check == false)
	//{
	//	Disconnect(pSession);
	//	return false;//로그인도 안되있는데 왜 요청을 계속보내?
	//}

	//if (pSession->_Connect_Room_No != dfdefaultRoom)
	//{
	//	return false;//방에 들어가있는데 어떻게 방요청을 보내냐?
	//}
	WORD arr_size;
	WCHAR arr_stress[1024] = {0,};

	*pPacket >> arr_size;
	pPacket->GetData((char*)arr_stress, arr_size);

	Send_ResStressTest(pSession, arr_size, arr_stress);
	
	return true;
};

void Send_ResStressTest(Session* pSession,WORD arr_size, WCHAR* _arr_stress)
{
	CSerealBuffer Packet;
	MakePacket_Send_StressTest(&Packet, arr_size, _arr_stress);

	sendUniCast(pSession, &Packet);
};
void MakePacket_Send_StressTest(CSerealBuffer* pPacket, WORD arr_size, WCHAR* _arr_stress)
{
	st_PACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfPACKET_CODE;
	stPacketHeader.wPayloadSize = sizeof(WORD) + arr_size;
	stPacketHeader.wMsgType = df_RES_STRESS_ECHO;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
	*pPacket << arr_size;
	pPacket->PutData((char*)_arr_stress, arr_size);

	//체크섬 세팅 하기귀찮아서 포인터 직접조정함
	//이따구로 하면안됨
	*(pPacket->GetBufferPtr() + 1) = _Ret_Checksum(stPacketHeader.wMsgType, stPacketHeader.wPayloadSize, pPacket);

};
