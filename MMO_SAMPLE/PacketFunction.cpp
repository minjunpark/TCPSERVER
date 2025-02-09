#include "PacketFunction.h"
#include "CSerealBuffer.h"
	void MakePacketCreateNewPlayer(CSerealBuffer& packet,unsigned int playerID, BYTE direction, unsigned short x, unsigned short y, BYTE HP)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(HP);
		header.packetType = dfPACKET_SC_CREATE_MY_CHARACTER;
		packet << header.code << header.payloadSize << header.packetType << playerID << direction << x << y << HP;
	}

	void MakePacketCreatePlayer(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y, BYTE HP)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(HP);
		header.packetType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
		packet << header.code << header.payloadSize << header.packetType << playerID << direction << x << y << HP;
	}

	void MakePacketDeletePlayer(CSerealBuffer& packet, unsigned int playerID)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID);
		header.packetType = dfPACKET_SC_DELETE_CHARACTER;
		packet << header.code << header.payloadSize << header.packetType << playerID;
	}

	void MakePacketPlayerMoveStart(CSerealBuffer& packet, unsigned int playerID, BYTE moveDirection, unsigned short x, unsigned short y)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(moveDirection) + sizeof(x) + sizeof(y);
		header.packetType = dfPACKET_SC_MOVE_START;
		packet << header.code << header.payloadSize << header.packetType << playerID << moveDirection << x << y;
	}

	void MakePacketPlayerMoveStop(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize =  sizeof(playerID) + sizeof(direction) + sizeof(x) + sizeof(y);
		header.packetType = dfPACKET_SC_MOVE_STOP;
		packet << header.code << header.payloadSize << header.packetType << playerID << direction << x << y;
	}

	void MakePacketPlayerAttack1(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(direction) + sizeof(x) + sizeof(y);
		header.packetType = dfPACKET_SC_ATTACK1;
		packet << header.code << header.payloadSize << header.packetType << playerID << direction << x << y;
	}

	void MakePacketPlayerAttack2(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(direction) + sizeof(x) + sizeof(y);
		header.packetType = dfPACKET_SC_ATTACK2;
		packet << header.code << header.payloadSize << header.packetType << playerID << direction << x << y;
	}

	void MakePacketPlayerAttack3(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(direction) + sizeof(x) + sizeof(y);
		header.packetType = dfPACKET_SC_ATTACK3;
		packet << header.code << header.payloadSize << header.packetType << playerID << direction << x << y;
	}

	void MakePacketPlayerHitDamage(CSerealBuffer& packet, unsigned int attackPlayerID, unsigned int damagedPlayerID, BYTE damageHP)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(attackPlayerID) + sizeof(damagedPlayerID) + sizeof(damageHP);
		header.packetType = dfPACKET_SC_DAMAGE;
		packet << header.code << header.payloadSize << header.packetType << attackPlayerID << damagedPlayerID << damageHP;
	}

	void MakePacketPositionSync(CSerealBuffer& packet, unsigned int playerID, unsigned short x, unsigned short y)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(playerID) + sizeof(x) + sizeof(y);
		header.packetType = dfPACKET_SC_SYNC;
		packet << header.code << header.payloadSize << header.packetType << playerID << x << y;
	}
	void MakePacketEcho(CSerealBuffer& packet, int time)
	{
		PacketHeader header;
		header.code = 0x89;
		header.payloadSize = sizeof(time);
		header.packetType  = dfPACKET_SC_ECHO;
		packet << header.code << header.payloadSize << header.packetType << time;
	}
