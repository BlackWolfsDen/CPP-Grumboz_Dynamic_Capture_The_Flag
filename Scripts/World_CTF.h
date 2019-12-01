#ifndef GRUMBOZ_WORLD_CTF_H
#define GRUMBOZ_WORLD_CTF_H

#include <unordered_map>

struct WorldFlags_Elements
{
	uint32 id;
	uint32 guid;
	uint32 map_id;
	uint32 area_id;
	uint32 zone_id;
    float x;
    float y;
    float z;
    float o;
};

struct FlagList_Elements
{
    uint32 id;
    uint32 guid;
};

struct WorldPlayerData_Elements
{
    uint32 guid;
    uint32 acct_id;
    std::string name;
    uint32 captures;
};

struct PlayerLeaderBoard_Elements
{
    std::string name;
    uint32 captures;
};

class TC_GAME_API GCTF
{

private:
	GCTF();
	~GCTF();

public:
	static GCTF* instance();

	// public methods
		// Getters
		uint32 GetActiveGO_ID() { return ActiveGO_ID; }
		uint32 GetDefaultWorldFlagID() { return Default_World_Flag_ID; }
		float GetDefaultWorldFlagScale() { return Default_World_Flag_Scale; }
		uint8 GetHintSystem() { return Default_Hint_System; }
        uint8 GetRequiredGMMinimumRank() { return Default_Required_GM_Rank; }
        uint32 GetFlagCtfID(uint32 guid) { return WorldFlags[guid].id; }


		// Setters
		void SetActiveGO_ID(uint32 v) { ActiveGO_ID = v; }
		void SetDefaultWorldFlagID(uint32 v) { Default_World_Flag_ID = v; }
		void SetDefaultWorldFlagScale(float v) { Default_World_Flag_Scale = v; }
		void SetHintSystem(uint8 v) { Default_Hint_System = v; }
		void SetRequiredGMMinimumRank(uint8 v) { Default_Required_GM_Rank = v; }
		void SetTest(bool v) { test = v; }
		// Tools
		std::string ConvertNumberToString(uint64 numberX);
		void SendWorldMsg(uint8 type, std::string message);
        void AddFlag(GameObject* go);
		void LoadWorldFlags();
		void GenerateNewRandomFlagGps();
        void LoadPlayerData();
        void AddCharacter(Player* player);
        void UpdatePlayerLeaderBoard();
        void PlayerAddWin(Player* player, uint32 value);

	// Public Tables
	std::unordered_map<uint32, WorldFlags_Elements> WorldFlags;
    std::unordered_map<uint32, FlagList_Elements> FlagList;
    std::unordered_map<uint32, WorldPlayerData_Elements> WorldPlayerData;
    std::unordered_map<uint32, PlayerLeaderBoard_Elements> PlayerLeaderBoard;

	// Public Variables
//	std::default_random_engine generator;
	bool test;

private:
	// Methods
	// Local Variables
	uint32 ActiveGO_ID;
	uint32 Default_World_Flag_ID;
	float Default_World_Flag_Scale;
	uint8 Default_Hint_System;
	uint8 Default_Required_GM_Rank;
};

#define sGCTF GCTF::instance()
#endif
