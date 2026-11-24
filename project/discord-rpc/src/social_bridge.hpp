#pragma once

#include <stdint.h>

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

	typedef void (*ErrorCallback)(int code, const char *message);

	bool Start(uint64_t applicationId);
	void Stop();
	bool Running();
	void Tick();
	void Update(const Presence &presence, ErrorCallback onError);
	void Clear();
	void SetOnline(bool online);
	bool Online();
}
