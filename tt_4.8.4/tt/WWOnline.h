#ifndef TT_INCLUDE__WWONLINE_H
#define TT_INCLUDE__WWONLINE_H



#include "engine_string.h"
#include "notify.h"
#include "RefPtr.h"
#include "RefCounted.h"



class WWOnline
{

public:

	enum MessageOfTheDayType;

	class ChannelData;

	
	class ServerError;
	class ConnectionStatus;
	class IRCServerData;
	class ChannelListEvent;
	class UserData;
	class UserEvent;
	class BuddyEvent;
	class ChannelEvent;
	class ChatMessage;
	class PageMessage;
	class PageSendStatus;
	class GameOptionsMessage;
	class GameStartEvent;
	class LadderInfoEvent;
	class LadderData;
	class SquadEvent;
	class NewLoginInfoEvent;
	class AgeCheckEvent;
	class UserIPEvent;

	struct RawPing
	{

		StringClass ipString; // 0000
		uint32 ip; // 0004
		int responseTime; // 0008
		int id; // 000C

	};
	
	struct PingServerData
	{
		PingServerData();
		PingServerData& operator =(const PingServerData&);

		UNKB unk0000[0x0100]; // 0000
		const char* const ipString; // 0100
		UNK unk0104; // 0104
		int ping; // 0108
		// ...
	};

	class Session :
		public RefCounted,
		public Notifier<ServerError>,
		public Notifier<ConnectionStatus>,
		public Notifier<Vector<RefPtr<IRCServerData>>>,
		public Notifier<ChannelListEvent>,
		public Notifier<Vector<RefPtr<UserData>>>,
		public Notifier<UserEvent>,
		public Notifier<BuddyEvent>,
		public Notifier<ChannelEvent>,
		public Notifier<TypedEvent<MessageOfTheDayType, WideStringClass>>,
		public Notifier<ChatMessage>,
		public Notifier<PageMessage>,
		public Notifier<PageSendStatus>,
		public Notifier<GameOptionsMessage>,
		public Notifier<GameStartEvent>,
		public Notifier<RawPing>,
		public Notifier<LadderInfoEvent>,
		public Notifier<Vector<RefPtr<LadderData>>>,
		public Notifier<SquadEvent>,
		public Notifier<NewLoginInfoEvent>,
		public Notifier<AgeCheckEvent>,
		public Notifier<UserIPEvent>
	{

	public:

		UNKB unk0000[0x0204 - 0x01AC];// 01AC  0158
		Vector<RefPtr<WWOnline::PingServerData>> pingServers; // 0204  ____
		UNKB unk0208[0x0330 - 0x0214]; // 0214  ____
		Vector<WWOnline::RawPing> pings; // 0330  ____
		// ...

	public:

		static RefPtr<Session> GetInstance(bool);
		
		virtual ~Session() {}
		void RequestPing(const char*, int);

	};
};



#endif