#ifndef TT_INCLUDE__NETWORKOBJECTCLASSID_H
#define TT_INCLUDE__NETWORKOBJECTCLASSID_H



enum NetworkObjectClassId
{
	
	// Renegade network class ids
	
	NET_BaseGameObj                      = 1000, // Server to Client
	NET_cScTextObj                       = 1001, // Server to Client
	NET_cPlayerKill                      = 1002, // Server to Client
	NET_cWinEvent                        = 1003, // Server to Client
	NET_cPurchaseResponseEvent           = 1004, // Server to Client
	NET_cConsoleCommandEvent             = 1005, // Server to Client; Disabled
//	NET_                                 = 1006, // Unused; NETCLASSID_RESETWINSEVENT
	NET_cSvrGoodbyeEvent                 = 1007, // Server to Client
	NET_cGameOptionsEvent                = 1008, // Server to Client
	NET_cEvictionEvent                   = 1009, // Server to Client
	NET_cTeam                            = 1010, // Server to Client
	NET_cPlayer                          = 1011, // Server to Client
	NET_cGameDataUpdateEvent             = 1012, // Server to Client
	NET_cScPingResponseEvent             = 1013, // Server to Client
	NET_cScExplosionEvent                = 1014, // Server to Client
//	NET_                                 = 1015, // Unused; NETCLASSID_SCOBELISKEVENT
	NET_SCAnnouncement                   = 1016, // Server to Client
	NET_cGameSpyScChallengeEvent         = 1017, // Server to Client
	NET_CClientControl                   = 1018, // Client to Server
	NET_cCsTextObj                       = 1019, // Client to Server
	NET_cSuicideEvent                    = 1020, // Client to Server
	NET_cChangeTeamEvent                 = 1021, // Client to Server
	NET_cMoneyEvent                      = 1022, // Server to Client
	NET_cWarpEvent                       = 1023, // Server to Client
	NET_cPurchaseRequestEvent            = 1024, // Client to Server
	NET_cClientGoodbyeEvent              = 1025, // Client to Server
	NET_cBioEvent                        = 1026, // Client to Server
	NET_cLoadingEvent                    = 1027, // Client to Server
	NET_cGodModeEvent                    = 1028, // Server to Client
	NET_cVipModeEvent                    = 1029, // Server to Client
	NET_cScoreEvent                      = 1030, // Server to Client
	NET_cClientBboEvent                  = 1031, // Client to Server
	NET_CClientFps                       = 1032, // Client to Server
	NET_cCsPingRequestEvent              = 1033, // Client to Server
	NET_cCsDamageEvent                   = 1034, // Client to Server; Replaced by NewDamageEvent
	NET_cRequestKillEvent                = 1035, // Server to Client
	NET_cCsConsoleCommandEvent           = 1036, // Server to Client; Disabled
	NET_cCsHint                          = 1037, // Client to Server
	NET_CSAnnouncement                   = 1038, // Client to Server
	NET_cDonateEvent                     = 1039, // Server to Client
	NET_cGameSpyCsChallengeResponseEvent = 1040, // Client to Server
	
	
	// TT network class ids
	
	NET_FileHashEvent                    = 2000, // Client to Server
	NET_NewDamageEvent                   = 2001, // Client to Server
	NET_GameHintEvent					 = 2002, // Server to Client
	NET_WeaponEvent                      = 2003, // Server to Client

};



#endif
