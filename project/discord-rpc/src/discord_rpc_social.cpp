#include "discord_rpc.hpp"
#include "social_bridge.hpp"

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
	struct PendingEvent
	{
		hxdiscord_social::Event event;
		int code;
		std::string message;
	};

	std::mutex HandlerLock;
	DiscordEventHandlers Handlers{};
	std::vector<PendingEvent> Pending;

	void OnEvent(hxdiscord_social::Event event, int code, const char *message)
	{
		std::lock_guard<std::mutex> guard(HandlerLock);
		Pending.push_back({event, code, message != nullptr ? message : ""});
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

bool Discord_IsReady(void)
{
	return hxdiscord_social::Ready();
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
		Pending.clear();
	}

	const uint64_t id = applicationId != nullptr ? std::strtoull(applicationId, nullptr, 10) : 0;

	if (!hxdiscord_social::Start(id, OnEvent))
		OnEvent(hxdiscord_social::Event::Errored, -1, "Invalid application id");
}

void Discord_Shutdown(void)
{
	hxdiscord_social::Stop();

	std::lock_guard<std::mutex> guard(HandlerLock);
	std::memset(&Handlers, 0, sizeof(Handlers));
	Pending.clear();
}

void Discord_RunCallbacks(void)
{
	hxdiscord_social::Tick();

	DiscordEventHandlers handlers;
	std::vector<PendingEvent> events;

	{
		std::lock_guard<std::mutex> guard(HandlerLock);
		handlers = Handlers;
		events.swap(Pending);
	}

	for (const PendingEvent &entry : events)
	{
		switch (entry.event)
		{
			case hxdiscord_social::Event::Ready:
			{
				if (handlers.ready == nullptr)
					break;

				hxdiscord_social::User current;
				if (!hxdiscord_social::CurrentUser(current))
					break;

				DiscordUser user{};
				user.userId = current.id.c_str();
				user.username = current.username.c_str();
				user.globalName = current.globalName.c_str();
				user.discriminator = "0";
				user.avatar = current.avatar.c_str();
				user.premiumType = DiscordPremiumType_None;
				user.bot = false;
				handlers.ready(&user);
				break;
			}
			case hxdiscord_social::Event::Disconnected:
				if (handlers.disconnected != nullptr)
					handlers.disconnected(entry.code, entry.message.c_str());
				break;
			case hxdiscord_social::Event::Errored:
				if (handlers.errored != nullptr)
					handlers.errored(entry.code, entry.message.c_str());
				break;
		}
	}
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

	hxdiscord_social::Update(data);
}

void Discord_ClearPresence(void)
{
	hxdiscord_social::Clear();
}

void Discord_SetActivityName(const char *name)
{
	hxdiscord_social::SetName(name);
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
