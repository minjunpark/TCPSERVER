#pragma once
#include "PacketDefine.h"

	using BYTE = unsigned char;

	class CSerealBuffer;

	void MakePacketCreateNewPlayer(CSerealBuffer& packet,unsigned int playerID, BYTE direction, unsigned short x, unsigned short y, BYTE HP);

	void MakePacketCreatePlayer(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y, BYTE HP);

	void MakePacketDeletePlayer(CSerealBuffer& packet, unsigned int playerID);

	void MakePacketPlayerMoveStart(CSerealBuffer& packet, unsigned int playerID, BYTE moveDirection, unsigned short x, unsigned short y);

	void MakePacketPlayerMoveStop(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerAttack1(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerAttack2(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerAttack3(CSerealBuffer& packet, unsigned int playerID, BYTE direction, unsigned short x, unsigned short y);

	void MakePacketPlayerHitDamage(CSerealBuffer& packet, unsigned int attackPlayerID, unsigned int damagedPlayerID, BYTE damageHP);

	void MakePacketPositionSync(CSerealBuffer& packet, unsigned int playerID, unsigned short x, unsigned short y);

	void MakePacketEcho(CSerealBuffer& packet, int time);