#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"

#include "social_bridge.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace
{
	std::recursive_mutex Lock;
	std::unique_ptr<discordpp::Client> Client;

	std::optional<std::string> Text(const char *value)
	{
		if (value == nullptr || value[0] == '\0')
			return std::nullopt;

		return std::string(value);
	}

	uint64_t Millis(int64_t value)
	{
		if (value <= 0)
			return 0;

		return value < 100000000000LL ? (uint64_t)value * 1000ULL : (uint64_t)value;
	}
}

namespace hxdiscord_social
{
	bool Start(uint64_t applicationId)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (applicationId == 0)
			return false;

		if (!Client)
			Client = std::make_unique<discordpp::Client>();

		Client->SetApplicationId(applicationId);
		return true;
	}

	void Stop()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!Client)
			return;

		Client->ClearRichPresence();
		discordpp::RunCallbacks();
		Client.reset();
	}

	bool Running()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);
		return Client != nullptr;
	}

	void Tick()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (Client)
			discordpp::RunCallbacks();
	}

	void Update(const Presence &presence, ErrorCallback onError)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!Client)
			return;

		discordpp::Activity activity;
		activity.SetType(static_cast<discordpp::ActivityTypes>(presence.type));
		activity.SetState(Text(presence.state));
		activity.SetDetails(Text(presence.details));

		const uint64_t start = Millis(presence.startTimestamp);
		const uint64_t end = Millis(presence.endTimestamp);
		if (start > 0 || end > 0)
		{
			discordpp::ActivityTimestamps timestamps;
			if (start > 0)
				timestamps.SetStart(start);
			if (end > 0)
				timestamps.SetEnd(end);
			activity.SetTimestamps(timestamps);
		}

		if (Text(presence.largeImageKey) || Text(presence.smallImageKey))
		{
			discordpp::ActivityAssets assets;
			assets.SetLargeImage(Text(presence.largeImageKey));
			assets.SetLargeText(Text(presence.largeImageText));
			assets.SetSmallImage(Text(presence.smallImageKey));
			assets.SetSmallText(Text(presence.smallImageText));
			activity.SetAssets(assets);
		}

		if (Text(presence.partyId) && presence.partyMax > 0)
		{
			discordpp::ActivityParty party;
			party.SetId(presence.partyId);
			party.SetCurrentSize(presence.partySize);
			party.SetMaxSize(presence.partyMax);
			party.SetPrivacy(presence.partyPrivacy == 1 ? discordpp::ActivityPartyPrivacy::Public : discordpp::ActivityPartyPrivacy::Private);
			activity.SetParty(party);
		}

		for (const Button &entry : presence.buttons)
		{
			if (!Text(entry.label) || !Text(entry.url))
				continue;

			discordpp::ActivityButton button;
			button.SetLabel(entry.label);
			button.SetUrl(entry.url);
			activity.AddButton(button);
		}

		Client->UpdateRichPresence(activity, [onError](discordpp::ClientResult result) {
			if (!result.Successful() && onError != nullptr)
				onError(static_cast<int>(result.ErrorCode()), result.Error().c_str());
		});
	}

	void Clear()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (Client)
			Client->ClearRichPresence();
	}
}
