// © Grumboz World Capture the Flag System © 
// © By slp13at420 of EmuDevs.com © 
// © an EmuDevs NomSoft - Only - release © 
// © http://emudevs.com/showthread.php/5993-CPP-Grumbo-z-Capture-the-Flag-System?p=39857#post39857

// © Language:CPP © 
// © Platform:TrinityCore © 
// © Start:10-05-2016 © 
// © Finish:10-07-2016 © 
// © Release:10-07-2016 © 
// © Primary Programmer:slp13at420 © 
// © Secondary Programmers:none © 

// © My latest version of my beloved blood shed system ;) © 
// ©  Do NOT remove any credits © 
// ©  Don't share/rerelease on any other site other than EmuDevs.com © 
// © Dont attempt to claim as your own work ... © 

#include "chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.cpp"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "Custom/World_CTF.h"
#include "Language.h"
#include "Log.h"
#include "Maps/MapManager.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QueryResult.h"
#include "Random.h"
#include "RBAC.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include <unordered_map>
#include "World.h"
#include "WorldSession.h"

GCTF::GCTF() { }

GCTF::~GCTF()
{
    if (sGCTF->test) { TC_LOG_INFO("server.loading", ">>    <[{DEFINE + CLEAR TABLES}]>    <<"); }

    for (std::unordered_map<uint32, WorldFlags_Elements>::iterator itr = WorldFlags.begin(); itr != WorldFlags.end(); ++itr)
		delete &itr->second;
    for (std::unordered_map<uint32, FlagList_Elements>::iterator itr = FlagList.begin(); itr != FlagList.end(); ++itr)
        delete& itr->second;
    for (std::unordered_map<uint32, WorldPlayerData_Elements>::iterator itr = WorldPlayerData.begin(); itr != WorldPlayerData.end(); ++itr)
		delete &itr->second;
    for (std::unordered_map<uint32, PlayerLeaderBoard_Elements>::iterator itr = PlayerLeaderBoard.begin(); itr != PlayerLeaderBoard.end(); ++itr)
        delete& itr->second;

    WorldFlags.clear();
    FlagList.clear();
    WorldPlayerData.clear();
    PlayerLeaderBoard.clear();
}

GCTF* GCTF::instance()
{
	static GCTF instance;
	return &instance;
}

std::string GCTF::ConvertNumberToString(uint64 numberX)
{
	auto number = numberX;
	std::stringstream convert;
	std::string number32_to_string;
	convert << number;
	number32_to_string = convert.str();

	return number32_to_string;
};

void GCTF::LoadWorldFlags()
{
	// Loading prestored World Flag's info from db

	uint32 flag_count = 1;

	QueryResult WorldFlagGps_Query = WorldDatabase.PQuery("SELECT `guid`, `map`, `zoneId`, `areaId`, `position_x`, `position_y`, `position_z`, `orientation` FROM gameobject WHERE `id` = %u;", sGCTF->GetDefaultWorldFlagID()); // id, guid, name, map_id, area_id, zone_id, x, y, z, o

	if (WorldFlagGps_Query)
	{
		do
		{
			Field* fields = WorldFlagGps_Query->Fetch();
			uint32 guid = fields[0].GetUInt32();
			uint32 map_id = fields[1].GetUInt32();
			uint32 zone_id = fields[2].GetUInt32();
			uint32 area_id = fields[3].GetUInt32();
            float x = fields[4].GetFloat();
            float y = fields[5].GetFloat();
            float z = fields[6].GetFloat();
            float o = fields[7].GetFloat();

			WorldFlags_Elements& data = sGCTF->WorldFlags[guid];
			// Save the DB values to the MyData object
			data.id = flag_count;
			data.guid = guid;
			data.map_id = map_id;
			data.area_id = area_id;
			data.zone_id = zone_id;
            data.x = x;
            data.y = y;
            data.z = z;
            data.o = o;

            FlagList_Elements& data1 = sGCTF->FlagList[flag_count];
            // Save the DB values to the MyData object
            data1.id = flag_count;
            data1.guid = guid;

            flag_count += 1;

		} while (WorldFlagGps_Query->NextRow());
	}
}

void GCTF::GenerateNewRandomFlagGps()
{
    int id = 1;

    if (sGCTF->FlagList.size() > 0)
    {
        id = urand(1, sGCTF->FlagList.size());

        if (!sGCTF->FlagList[id].guid)
        {
            sGCTF->GenerateNewRandomFlagGps();
        }
    }

    sGCTF->SetActiveGO_ID(id);

    Map* map = sMapMgr->FindMap(sGCTF->WorldFlags[sGCTF->FlagList[id].guid].map_id, 0);

    std::string map_name = map->GetMapName();
    std::string message = "new world flag spawned at " + map_name;

    sGCTF->SendWorldMsg(1, message);

    if (sGCTF->test) { TC_LOG_INFO("server.loading", "GENERATE_NEW_ACTIVE_FLAG ID:%u of %u", id, sGCTF->FlagList.size()); }
}

void GCTF::AddFlag(GameObject* go)
{
    uint32 flag_count = (sGCTF->WorldFlags.size()) + 1;
    uint32 guid = go->GetSpawnId();

    WorldFlags_Elements& data = sGCTF->WorldFlags[guid];
    // Save the DB values to the MyData object
    data.id = flag_count;
    data.guid = guid;
    data.map_id = go->GetMapId();
    data.area_id = go->GetAreaId();
    data.zone_id = go->GetZoneId();
    data.x = go->GetPositionX();
    data.y = go->GetPositionY();
    data.z = go->GetPositionZ();
    data.o = go->GetOrientation();

    FlagList_Elements& data1 = sGCTF->FlagList[flag_count];
    // Save the DB values to the MyData object
    data1.id = flag_count;
    data1.guid = guid;

    if (sGCTF->test) { TC_LOG_INFO("server.loading", "ADD_NEW__FLAG ID:%u", flag_count); }
}

void GCTF::LoadPlayerData()
{
    uint32 player_count = 1;

    QueryResult WorldPlayerData_Query = WorldDatabase.PQuery("SELECT `acct_id`, `guid`, `name`, `captures` FROM grumboz_ctf;"); // id, guid, name, map_id, area_id, zone_id, x, y, z, o

    if (WorldPlayerData_Query)
    {
        do
        {
            Field* fields = WorldPlayerData_Query->Fetch();
            uint32 acct_id = fields[0].GetUInt32();
            uint32 guid = fields[1].GetUInt32();
            std::string name = fields[2].GetString();
            uint32 captures = fields[3].GetUInt32();

            WorldPlayerData_Elements& data = sGCTF->WorldPlayerData[guid];
            // Save the DB values to the MyData object
            data.acct_id = acct_id;
            data.guid = guid;
            data.name = name;
            data.captures = captures;

            player_count += 1;

        } while (WorldPlayerData_Query->NextRow());
    }

}

void GCTF::AddCharacter(Player* player)
{
    std::string WorldPlayerData_Query;
    uint32 player_count = (sGCTF->WorldPlayerData.size());

    uint32 acct_id = player->GetSession()->GetAccountId();
    uint32 guid = player->GetGUID();
    std::string name = player->GetName();
    uint32 captures = 0;

//    WorldPlayerData_Query = ;
    
    WorldDatabase.PExecute("INSERT INTO grumboz_ctf VALUES('%u', '%u', '%s', '%u');", acct_id, guid, name.c_str(), captures);

    WorldPlayerData_Elements& data = sGCTF->WorldPlayerData[guid];
    // Save the DB values to the MyData object
    data.acct_id = acct_id;
    data.guid = guid;
    data.name = name;
    data.captures = captures;
}

void GCTF::UpdatePlayerLeaderBoard()
{
    PlayerLeaderBoard.clear();

    uint32 rank = 1;

    QueryResult RankQry = WorldDatabase.Query("SELECT `name`,`captures`  FROM grumboz_ctf  ORDER BY `captures` DESC;");
    if (RankQry)
    {
        do
        {
            Field* fields = RankQry->Fetch();
            // Save the DB values to the LocData object
            std::string name = fields[0].GetString();
            uint32 captures = fields[1].GetUInt32();

            PlayerLeaderBoard[rank].name = name;
            PlayerLeaderBoard[rank].captures = captures;

            rank = rank + 1;

        } while (RankQry->NextRow());
    }
}

void GCTF::PlayerAddWin(Player* player, uint32 value)
{
    uint32 guid = player->GetGUID();
    uint32 captures = sGCTF->WorldPlayerData[guid].captures + value;

    WorldDatabase.PExecute("UPDATE grumboz_ctf SET `captures` = %u WHERE `guid` = %u;", captures, guid); // id, guid, name, map_id, area_id, zone_id, x, y, z, o

    sGCTF->WorldPlayerData[guid].captures = captures;
}

class CTF_Load_Conf  : public WorldScript
{
public: CTF_Load_Conf() : WorldScript("CTF_Load_Conf") { };

		virtual void OnConfigLoad(bool /*reload*/)
		{
			TC_LOG_INFO("server.loading", "___________________________________");
			TC_LOG_INFO("server.loading", "-        Grumboz World CTF        -");
			TC_LOG_INFO("server.loading", "___________________________________");

			// Storing flag carrier aura ids by teamId
			// Load and Store the World conf entries
			sGCTF->SetDefaultWorldFlagID(sConfigMgr->GetIntDefault("CTF.DEFAULT_WORLD_FLAG_ID", 600002));
			sGCTF->SetDefaultWorldFlagScale(sConfigMgr->GetFloatDefault("CTF.DEFAULT_WORLD_FLAG_SCALE", 30.00));
			sGCTF->SetHintSystem(sConfigMgr->GetIntDefault("CTF.HINT_SYSTEM", 0));
			sGCTF->SetRequiredGMMinimumRank(sConfigMgr->GetIntDefault("CTF.GM_RANK", 3));
			sGCTF->SetTest(sConfigMgr->GetBoolDefault("CTF.TEST", false));

            sGCTF->LoadWorldFlags();

			uint32 flag_count = sGCTF->WorldFlags.size();

			TC_LOG_INFO("server.loading", "- %u flag locations loaded", flag_count);

            sGCTF->LoadPlayerData();

            uint32 player_count = sGCTF->WorldPlayerData.size();

            TC_LOG_INFO("server.loading", "- %u characters loaded.", player_count);

            // Post Settings to console
				if (sGCTF->GetHintSystem() == 0) { TC_LOG_INFO("server.loading", "- Hint System:Idle."); }
				if (sGCTF->GetHintSystem() == 1) { TC_LOG_INFO("server.loading", "- Hint System:Active."); }

				TC_LOG_INFO("server.loading", "- World Flag Scale Size :%.2f.", sGCTF->GetDefaultWorldFlagScale());
				TC_LOG_INFO("server.loading", "- Minimum required GM rank:%u.", sGCTF->GetRequiredGMMinimumRank());

				if (sGCTF->test) { TC_LOG_INFO("server.loading", ">>    <[{Test Mode Active}]>    <<"); }

    			TC_LOG_INFO("server.loading", "___________________________________");

				if (flag_count >= 1) { sGCTF->GenerateNewRandomFlagGps(); }
		}
};

void GCTF::SendWorldMsg(uint8 type, std::string message)
{ // type [ 1 = global via hint system // 2 = bypass hint and announce to all]
	SessionMap sessions = sWorld->GetAllSessions();

	for (SessionMap::iterator itr = sessions.begin(); itr != sessions.end(); ++itr)
	{

		if (!itr->second)
			continue;

		Player *player = itr->second->GetPlayer();

			ChatHandler(player->GetSession()).PSendSysMessage(message.c_str());
	}

};

class CTF_Flag : public GameObjectScript
{
public: CTF_Flag() : GameObjectScript("CTF_Flag") { };

			struct World_Flag : public GameObjectAI
			{

				World_Flag(GameObject* go) : GameObjectAI(go) { }

				bool GossipHello(Player* player/*, GameObject* go*/) // override // virtual
				{
                    if (!sGCTF->WorldFlags[me->GetSpawnId()].id == sGCTF->GetActiveGO_ID())
                    {
                        me->SetPhaseMask(0, true);
                    }
                    else {
                        if (player->IsGameMaster())
                        {
                            ChatHandler(player->GetSession()).PSendSysMessage("You are in GM mode. Exit GM mode to enjoy.|r");

                            return true;
                        }
                        else
                        {
                            uint32 guid = player->GetGUID();

                            me->SetPhaseMask(0, true);

                            sGCTF->SetActiveGO_ID(0);

                            std::string msg1 = player->GetName() + " has claimed the World flag.";

                            sGCTF->SendWorldMsg(2, msg1);

                            sGCTF->GenerateNewRandomFlagGps();

                            ChatHandler(player->GetSession()).PSendSysMessage("Captures:%u", sGCTF->WorldPlayerData[guid].captures + 1);

                            sGCTF->PlayerAddWin(player, 1);
                        }
                    }
					return true;
				}

				void UpdateAI(uint32 diff) // override // This function updates every 1000 (I believe) and is used for the timers, etc
				{
//                    if (sGCTF->test) { TC_LOG_INFO("server.loading", "[FLAG] UPDATE_AI"); }

					uint32 guid = me->GetSpawnId();
                    uint32 phasemask = me->GetPhaseMask();
                    uint32 activeGuid = sGCTF->FlagList[sGCTF->GetActiveGO_ID()].guid;
					uint32 defaultflagid = sGCTF->GetDefaultWorldFlagID();

                    if (sGCTF->WorldFlags[guid].guid != guid) { sGCTF->AddFlag(me); }

                    if (guid == activeGuid && phasemask == 0)
					{
						me->SetPhaseMask(1, true); // PHASEMASK_ANYWHERE -1
                        if (sGCTF->test) { TC_LOG_INFO("server.loading", "[FLAG] UPDATE_AI PHASEMASK 1 %u %u", guid, activeGuid); }
                    }

                    if (guid != activeGuid && phasemask == 1)
                    {
                        me->SetPhaseMask(0, true); // PHASEMASK_ANYWHERE -1

                    if (sGCTF->test) { TC_LOG_INFO("server.loading", "[FLAG] UPDATE_AI PHASEMASK 0 %u %u", guid, activeGuid); }
                    }
				}
		};

		GameObjectAI* GetAI(GameObject* go) const override
		{
			return new World_Flag(go);
		}
};

class CTF_Player_Actions : public PlayerScript
{
public: CTF_Player_Actions() : PlayerScript("CTF_Player_Actions") { };

		virtual void OnLogout(Player* player)
		{ 
		}

		virtual void OnLogin(Player* player, bool firstLogin)
		{
            uint32 guid = player->GetGUID();

            if (!sGCTF->WorldPlayerData[guid].guid) { sGCTF->AddCharacter(player); }
		}
};

class CTF_commands : public CommandScript
{
public: CTF_commands() : CommandScript("CTF_commands") { };

	std::vector<ChatCommand> GetCommands() const
	{

		static std::vector<ChatCommand> CTFCommandTable =
		{
			{ "setup",		rbac::RBAC_IN_GRANTED_LIST, true, &HandleCTFSetupCommand, "world capture-the-flag command to display current settings for players." },
            { "list",		rbac::RBAC_IN_GRANTED_LIST, true, &HandleCTFPlayerLeaderBoard, "Player Leaderboard." },
            { "tele",		rbac::RBAC_PERM_COMMAND_SERVER, true, &HandleCTFTeleCommand, "use .tele x where x is the id or null to tele to current active flag." },
			{ "cycle",		rbac::RBAC_PERM_COMMAND_SERVER, true, &HandleCTFCycleCommand, "use to cycle a new current active flag ." },
            { "add",		rbac::RBAC_PERM_COMMAND_SERVER, true, &HandleCTFAddCommand, "use to add a new flag ." },
        };

		static std::vector<ChatCommand> commandTable =
		{
			{ "ctf", rbac::RBAC_IN_GRANTED_LIST, true, NULL, "custom world capture the flag commands by Grumbo.", CTFCommandTable },
		};

		return commandTable;
	}

	static bool HandleCTFSetupCommand(ChatHandler* handler, const char* args)
	{
		Player* player = handler->GetSession()->GetPlayer();
		uint32 guid = player->GetGUID();

		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");
		ChatHandler(player->GetSession()).PSendSysMessage("           Capture the Flag settings           ");
		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");

		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");
		ChatHandler(player->GetSession()).PSendSysMessage("                  Global data                  ");
		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");

		if (sGCTF->GetHintSystem() == 0) { ChatHandler(player->GetSession()).PSendSysMessage("- Hint System:Idle."); }
		if (sGCTF->GetHintSystem() == 2) { ChatHandler(player->GetSession()).PSendSysMessage("- Hint System:Active."); }


		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");
		ChatHandler(player->GetSession()).PSendSysMessage("                 Player data                   ");
		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");

		ChatHandler(player->GetSession()).PSendSysMessage("- Captures:%u", sGCTF->WorldPlayerData[guid].captures);
		ChatHandler(player->GetSession()).PSendSysMessage("- Total Active Flags:%u", sGCTF->WorldFlags.size());

		if (handler->GetSession()->GetSecurity() >= sGCTF->GetRequiredGMMinimumRank())
		{ 
			ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");
			ChatHandler(player->GetSession()).PSendSysMessage("                    GM data                    ");
			ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");

			ChatHandler(player->GetSession()).PSendSysMessage("- Minimum required GM rank:%u.", sGCTF->GetRequiredGMMinimumRank());
		}

		ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");

		return true;
	}

    static bool HandleCTFPlayerLeaderBoard(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();

        sGCTF->UpdatePlayerLeaderBoard();

        uint8 id;

        ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");
        ChatHandler(player->GetSession()).PSendSysMessage("               Player LeaderBoard              ");
        ChatHandler(player->GetSession()).PSendSysMessage("-----------------------------------------------");

        for (id = 1; id <= 10; id++)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("%u %s %u", id, sGCTF->PlayerLeaderBoard[id].name, sGCTF->PlayerLeaderBoard[id].captures);
        }
        return true;
    }

	static bool HandleCTFTeleCommand(ChatHandler* handler, const char* args)
	{
		Player* player = handler->GetSession()->GetPlayer();

		if (!player->IsGameMaster())
		{
			ChatHandler(player->GetSession()).PSendSysMessage("You need to be in GM mode.");
		}
		else
		{
			if (handler->GetSession()->GetSecurity() < sGCTF->GetRequiredGMMinimumRank())
			{
				ChatHandler(player->GetSession()).PSendSysMessage("You need to be GM with rank:%u.", sGCTF->GetRequiredGMMinimumRank());
			}
			else
			{
				uint32 id = 1;

				if (*args)
					id = (uint32)atoi((char*)args);

				if (sGCTF->WorldFlags[id].id == id)
				{
					player->TeleportTo(sGCTF->WorldFlags[id].map_id, sGCTF->WorldFlags[id].x, sGCTF->WorldFlags[id].y, sGCTF->WorldFlags[id].z, sGCTF->WorldFlags[id].o);
				}
				else
				{
					ChatHandler(player->GetSession()).PSendSysMessage("Bad flag id:%u.", id);
				}
			}
		}
		return true;
	}

	static bool HandleCTFCycleCommand(ChatHandler* handler, const char* args)
	{
		Player* player = handler->GetSession()->GetPlayer();

		if (!player->IsGameMaster())
		{
			ChatHandler(player->GetSession()).PSendSysMessage("You need to be in GM mode.");
		}
		else
		{
			if (handler->GetSession()->GetSecurity() < sGCTF->GetRequiredGMMinimumRank())
			{
				ChatHandler(player->GetSession()).PSendSysMessage("You need to be GM with rank:%u.", sGCTF->GetRequiredGMMinimumRank());
			}
			else
			{
				sGCTF->GenerateNewRandomFlagGps();
			}
		}
		return true;
	}

    static bool HandleCTFAddCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        Map* map = player->GetMap();

        if (!player->IsGameMaster())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You need to be in GM mode.");
        }
        else
        {
            if (handler->GetSession()->GetSecurity() < sGCTF->GetRequiredGMMinimumRank())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("You need to be GM with rank:%u.", sGCTF->GetRequiredGMMinimumRank());
            }
            else
            {
                uint32 objectId = sGCTF->GetDefaultWorldFlagID();

                GameObjectTemplate const* objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);
                if (!objectInfo)
                {
                    handler->PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, objectId);
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
                {
                    // report to DB errors log as in loading case
                    TC_LOG_ERROR("sql.sql", "Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.", objectId, objectInfo->type, objectInfo->displayId);
                    handler->PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA, objectId);
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                GameObject* object = new GameObject();
                ObjectGuid::LowType guidLow = map->GenerateLowGuid<HighGuid::GameObject>();
                QuaternionData rot = QuaternionData::fromEulerAnglesZYX(player->GetOrientation(), 0.f, 0.f);

                if (!object->Create(guidLow, objectInfo->entry, map, player->GetPhaseMaskForSpawn(), *player, rot, 255, GO_STATE_READY))
                {
                    delete object;
                    return false;
                }

                // fill the gameobject data and save to the db
                object->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), player->GetPhaseMaskForSpawn());
                guidLow = object->GetSpawnId();

                // delete the old object and do a clean load from DB with a fresh new GameObject instance.
                // this is required to avoid weird behavior and memory leaks
                delete object;

                object = new GameObject();
                // this will generate a new guid if the object is in an instance
                if (!object->LoadFromDB(guidLow, map, true))
                {
                    delete object;
                    return false;
                }

                /// @todo is it really necessary to add both the real and DB table guid here ?
                sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGameObjectData(guidLow));

                handler->PSendSysMessage(LANG_GAMEOBJECT_ADD, objectId, objectInfo->name.c_str(), guidLow, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
//                return true;

                sGCTF->AddFlag(object);
            }
        }
    return true;
    }
};

void AddSC_Grumboz_CTF()
{
	new CTF_Load_Conf();
	new CTF_Flag();
	new CTF_Player_Actions();
	new CTF_commands();
}
