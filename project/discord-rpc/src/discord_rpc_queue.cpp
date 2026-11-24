#include "discord_rpc.hpp"
#include "discord_rpc_queue.hpp"

#include <deque>
#include <mutex>
#include <string>

namespace
{
	struct QueuedEvent
	{
		int type = DiscordQueueEventType_None;
		int errorCode = 0;
		int premiumType = 0;
		bool bot = false;
		std::string message;
		std::string secret;
		std::string userId;
		std::string username;
		std::string globalName;
		std::string discriminator;
		std::string avatar;
	};

	const size_t MAX_EVENTS = 64;

	std::mutex queueMutex;
	std::deque<QueuedEvent> queue;
	QueuedEvent current;
	QueuedEvent connectedUser;
	bool hasUser = false;
	std::string returned;

	std::string copy(const char *value)
	{
		return value != nullptr ? std::string(value) : std::string();
	}

	void push(QueuedEvent &&event)
	{
		std::lock_guard<std::mutex> guard(queueMutex);
		if (queue.size() >= MAX_EVENTS)
			queue.pop_front();
		queue.push_back(std::move(event));
	}

	QueuedEvent fromUser(int type, const DiscordUser *user)
	{
		QueuedEvent event;
		event.type = type;
		if (user != nullptr)
		{
			event.userId = copy(user->userId);
			event.username = copy(user->username);
			event.globalName = copy(user->globalName);
			event.discriminator = copy(user->discriminator);
			event.avatar = copy(user->avatar);
			event.premiumType = (int)user->premiumType;
			event.bot = user->bot;
		}
		return event;
	}

	void onReady(const DiscordUser *user)
	{
		QueuedEvent event = fromUser(DiscordQueueEventType_Ready, user);
		{
			std::lock_guard<std::mutex> guard(queueMutex);
			connectedUser = event;
			hasUser = true;
		}
		push(std::move(event));
	}

	void onStatus(int type, int errorCode, const char *message)
	{
		QueuedEvent event;
		event.type = type;
		event.errorCode = errorCode;
		event.message = copy(message);
		if (type == DiscordQueueEventType_Disconnected)
		{
			std::lock_guard<std::mutex> guard(queueMutex);
			hasUser = false;
			connectedUser = QueuedEvent();
		}
		push(std::move(event));
	}

	void onDisconnected(int errorCode, const char *message)
	{
		onStatus(DiscordQueueEventType_Disconnected, errorCode, message);
	}

	void onErrored(int errorCode, const char *message)
	{
		onStatus(DiscordQueueEventType_Errored, errorCode, message);
	}

	void onSecret(int type, const char *secret)
	{
		QueuedEvent event;
		event.type = type;
		event.secret = copy(secret);
		push(std::move(event));
	}

	void onJoinGame(const char *secret)
	{
		onSecret(DiscordQueueEventType_JoinGame, secret);
	}

	void onSpectateGame(const char *secret)
	{
		onSecret(DiscordQueueEventType_SpectateGame, secret);
	}

	void onJoinRequest(const DiscordUser *user)
	{
		push(fromUser(DiscordQueueEventType_JoinRequest, user));
	}

	const char *give(const std::string &value)
	{
		returned = value;
		return returned.c_str();
	}
}

void DiscordQueue_Initialize(const char *applicationId, bool autoRegister, const char *optionalSteamId)
{
	DiscordEventHandlers handlers = {};
	handlers.ready = onReady;
	handlers.disconnected = onDisconnected;
	handlers.errored = onErrored;
	handlers.joinGame = onJoinGame;
	handlers.spectateGame = onSpectateGame;
	handlers.joinRequest = onJoinRequest;
	Discord_Initialize(applicationId, &handlers, autoRegister, optionalSteamId);
}

void DiscordQueue_Shutdown(void)
{
	Discord_Shutdown();
	std::lock_guard<std::mutex> guard(queueMutex);
	queue.clear();
	current = QueuedEvent();
	connectedUser = QueuedEvent();
	hasUser = false;
}

void DiscordQueue_Update(void)
{
#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif
	Discord_RunCallbacks();
}

int DiscordQueue_Next(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	if (queue.empty())
	{
		current = QueuedEvent();
		return DiscordQueueEventType_None;
	}
	current = std::move(queue.front());
	queue.pop_front();
	return current.type;
}

int DiscordQueue_Pending(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return (int)queue.size();
}

void DiscordQueue_Clear(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	queue.clear();
}

const char *DiscordQueue_String(int field)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	switch (field)
	{
		case DiscordQueueField_Message: return give(current.message);
		case DiscordQueueField_Secret: return give(current.secret);
		case DiscordQueueField_UserId: return give(current.userId);
		case DiscordQueueField_Username: return give(current.username);
		case DiscordQueueField_GlobalName: return give(current.globalName);
		case DiscordQueueField_Discriminator: return give(current.discriminator);
		case DiscordQueueField_Avatar: return give(current.avatar);
	}
	return give(std::string());
}

int DiscordQueue_ErrorCode(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return current.errorCode;
}

int DiscordQueue_PremiumType(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return current.premiumType;
}

bool DiscordQueue_Bot(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return current.bot;
}

const char *DiscordQueue_ConnectedUserId(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return give(connectedUser.userId);
}

const char *DiscordQueue_ConnectedUsername(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return give(connectedUser.username);
}

const char *DiscordQueue_ConnectedGlobalName(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return give(connectedUser.globalName);
}

const char *DiscordQueue_ConnectedAvatar(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return give(connectedUser.avatar);
}

bool DiscordQueue_HasUser(void)
{
	std::lock_guard<std::mutex> guard(queueMutex);
	return hasUser;
}

static const char *nullIfEmpty(const char *value)
{
	return value != nullptr && value[0] != '\0' ? value : nullptr;
}

void DiscordQueue_UpdatePresence(int type, const char *state, const char *details, double startTimestamp, double endTimestamp, const char *largeImageKey,
	const char *largeImageText, const char *smallImageKey, const char *smallImageText, const char *partyId, int partySize, int partyMax, int partyPrivacy,
	const char *button1Label, const char *button1Url, const char *button2Label, const char *button2Url, const char *matchSecret, const char *joinSecret,
	const char *spectateSecret, bool instance)
{
	DiscordRichPresence presence = {};
	presence.type = (DiscordActivityType)type;
	presence.state = nullIfEmpty(state);
	presence.details = nullIfEmpty(details);
	presence.startTimestamp = (int64_t)startTimestamp;
	presence.endTimestamp = (int64_t)endTimestamp;
	presence.largeImageKey = nullIfEmpty(largeImageKey);
	presence.largeImageText = nullIfEmpty(largeImageText);
	presence.smallImageKey = nullIfEmpty(smallImageKey);
	presence.smallImageText = nullIfEmpty(smallImageText);
	presence.partyId = nullIfEmpty(partyId);
	presence.partySize = partySize;
	presence.partyMax = partyMax;
	presence.partyPrivacy = (DiscordActivityPartyPrivacy)partyPrivacy;
	presence.buttons[0].label = nullIfEmpty(button1Label);
	presence.buttons[0].url = nullIfEmpty(button1Url);
	presence.buttons[1].label = nullIfEmpty(button2Label);
	presence.buttons[1].url = nullIfEmpty(button2Url);
	presence.matchSecret = nullIfEmpty(matchSecret);
	presence.joinSecret = nullIfEmpty(joinSecret);
	presence.spectateSecret = nullIfEmpty(spectateSecret);
	presence.instance = instance;
	Discord_UpdatePresence(&presence);
}
