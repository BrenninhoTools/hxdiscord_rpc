#pragma once

#include <stdint.h>
#include <string>

namespace hxdiscord_social
{
	struct Button
	{
		const char *label;
		const char *url;
	};

	struct Presence
	{
		int type;
		const char *state;
		const char *details;
		int64_t startTimestamp;
		int64_t endTimestamp;
		const char *largeImageKey;
		const char *largeImageText;
		const char *smallImageKey;
		const char *smallImageText;
		const char *partyId;
		int partySize;
		int partyMax;
		int partyPrivacy;
		Button buttons[2];
	};

	struct User
	{
		std::string id;
		std::string username;
		std::string globalName;
		std::string avatar;
	};

	enum class Event
	{
		Ready,
		Disconnected,
		Errored
	};

	typedef void (*EventCallback)(Event event, int code, const char *message);

	bool Start(uint64_t applicationId, EventCallback onEvent);
	void Stop();
	bool Running();
	bool Ready();
	bool CurrentUser(User &user);
	void Tick();
	void Update(const Presence &presence);
	void Clear();
	void SetOnline(bool online);
	bool Online();
	void SetStoragePath(const char *path);
}
