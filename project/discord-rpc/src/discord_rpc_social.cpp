#include "discord_rpc.hpp"
#include "social_bridge.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

namespace
{
	std::mutex HandlerLock;
	DiscordEventHandlers Handlers{};
	std::atomic<bool> PendingReady{false};
	std::atomic<bool> PendingError{false};
	int LastErrorCode = 0;
	std::string LastError;

	void OnError(int code, const char *message)
	{
		std::lock_guard<std::mutex> guard(HandlerLock);
		LastErrorCode = code;
		LastError = message != nullptr ? message : "";
		PendingError = true;
	}
}

void Discord_Register(const char *applicationId, const char *command)
{
	(void)applicationId;
	(void)command;
}

void Discord_RegisterSteamGame(const char *applicationId, const char *steamId)
{
	(void)applicationId;
	(void)steamId;
}

bool Discord_IsSupported(void)
{
	return true;
}

void Discord_Initialize(const char *applicationId, DiscordEventHandlers *handlers, bool autoRegister, const char *optionalSteamId)
{
	(void)autoRegister;
	(void)optionalSteamId;

	{
		std::lock_guard<std::mutex> guard(HandlerLock);
		if (handlers != nullptr)
			Handlers = *handlers;
		else
			std::memset(&Handlers, 0, sizeof(Handlers));
	}

	const uint64_t id = applicationId != nullptr ? std::strtoull(applicationId, nullptr, 10) : 0;
	PendingReady = hxdiscord_social::Start(id);
}

void Discord_Shutdown(void)
{
	hxdiscord_social::Stop();

	std::lock_guard<std::mutex> guard(HandlerLock);
	std::memset(&Handlers, 0, sizeof(Handlers));
	PendingReady = false;
	PendingError = false;
}

void Discord_RunCallbacks(void)
{
	hxdiscord_social::Tick();

	DiscordEventHandlers handlers;
	int errorCode = 0;
	std::string error;
	bool ready = false;
	bool errored = false;

	{
		std::lock_guard<std::mutex> guard(HandlerLock);
		handlers = Handlers;
		ready = PendingReady.exchange(false);
		errored = PendingError.exchange(false);
		errorCode = LastErrorCode;
		error = LastError;
	}

	if (ready && handlers.ready != nullptr)
	{
		DiscordUser user{};
		user.userId = "0";
		user.username = "";
		user.globalName = "";
		user.discriminator = "0";
		user.avatar = "";
		user.premiumType = DiscordPremiumType_None;
		user.bot = false;
		handlers.ready(&user);
	}

	if (errored && handlers.errored != nullptr)
		handlers.errored(errorCode, error.c_str());
}

void Discord_UpdateConnection(void)
{
	hxdiscord_social::Tick();
}

void Discord_UpdatePresence(const DiscordRichPresence *presence)
{
	if (presence == nullptr)
	{
		hxdiscord_social::Clear();
		return;
	}

	hxdiscord_social::Presence data{};
	data.type = static_cast<int>(presence->type);
	data.state = presence->state;
	data.details = presence->details;
	data.startTimestamp = presence->startTimestamp;
	data.endTimestamp = presence->endTimestamp;
	data.largeImageKey = presence->largeImageKey;
	data.largeImageText = presence->largeImageText;
	data.smallImageKey = presence->smallImageKey;
	data.smallImageText = presence->smallImageText;
	data.partyId = presence->partyId;
	data.partySize = presence->partySize;
	data.partyMax = presence->partyMax;
	data.partyPrivacy = static_cast<int>(presence->partyPrivacy);
	for (int i = 0; i < 2; i++)
	{
		data.buttons[i].label = presence->buttons[i].label;
		data.buttons[i].url = presence->buttons[i].url;
	}

	hxdiscord_social::Update(data, OnError);
}

void Discord_ClearPresence(void)
{
	hxdiscord_social::Clear();
}

void Discord_Respond(const char *userid, DiscordActivityJoinRequestReply reply)
{
	(void)userid;
	(void)reply;
}

void Discord_UpdateHandlers(DiscordEventHandlers *handlers)
{
	std::lock_guard<std::mutex> guard(HandlerLock);
	if (handlers != nullptr)
		Handlers = *handlers;
}
