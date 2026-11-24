#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"

#include "social_bridge.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>

#if defined(__ANDROID__)
#include <jni.h>
#endif

namespace
{
	using Clock = std::chrono::steady_clock;

	struct StoredButton
	{
		std::optional<std::string> label;
		std::optional<std::string> url;

		bool operator==(const StoredButton &other) const
		{
			return label == other.label && url == other.url;
		}
	};

	struct StoredPresence
	{
		int type = 0;
		std::optional<std::string> state;
		std::optional<std::string> details;
		int64_t startTimestamp = 0;
		int64_t endTimestamp = 0;
		std::optional<std::string> largeImageKey;
		std::optional<std::string> largeImageText;
		std::optional<std::string> smallImageKey;
		std::optional<std::string> smallImageText;
		std::optional<std::string> partyId;
		int partySize = 0;
		int partyMax = 0;
		int partyPrivacy = 0;
		StoredButton buttons[2];

		bool operator==(const StoredPresence &other) const
		{
			return type == other.type && state == other.state && details == other.details && startTimestamp == other.startTimestamp &&
				endTimestamp == other.endTimestamp && largeImageKey == other.largeImageKey && largeImageText == other.largeImageText &&
				smallImageKey == other.smallImageKey && smallImageText == other.smallImageText && partyId == other.partyId &&
				partySize == other.partySize && partyMax == other.partyMax && partyPrivacy == other.partyPrivacy && buttons[0] == other.buttons[0] &&
				buttons[1] == other.buttons[1];
		}
	};

	constexpr auto MinBackoff = std::chrono::milliseconds(2000);
	constexpr auto MaxBackoff = std::chrono::milliseconds(60000);
	constexpr auto NetworkSettle = std::chrono::milliseconds(1500);
	constexpr auto RequestTimeout = std::chrono::milliseconds(15000);
	constexpr auto Heartbeat = std::chrono::milliseconds(120000);

	std::recursive_mutex Lock;
	std::unique_ptr<discordpp::Client> Client;
	std::optional<StoredPresence> Last;
	std::optional<std::string> Name;
	hxdiscord_social::EventCallback OnEvent = nullptr;

	uint64_t ApplicationId = 0;
	std::string StoragePath;
	bool Authorizing = false;
	bool AuthorizeDenied = false;
	bool Refreshing = false;
	bool RefreshPending = false;
	int RefreshAttempts = 0;
	Clock::time_point RefreshAt{};
	bool WasReady = false;

	bool Dirty = false;
	bool InFlight = false;
	bool Cleared = false;
	int Attempts = 0;
	uint64_t Generation = 0;
	Clock::time_point NextAttempt{};
	Clock::time_point SentAt{};
	Clock::time_point LastSuccess{};

	std::atomic<bool> IsOnline{true};
	std::atomic<bool> NetworkChanged{false};

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

	Clock::duration Backoff(int attempts)
	{
		static std::minstd_rand engine{std::random_device{}()};

		auto delay = MinBackoff;
		for (int i = 1; i < attempts && delay < MaxBackoff; i++)
			delay *= 2;

		delay = std::min(delay, MaxBackoff);
		const auto jitter = std::chrono::milliseconds(engine() % 750);
		return delay + jitter;
	}

	StoredPresence Store(const hxdiscord_social::Presence &presence)
	{
		StoredPresence stored;
		stored.type = presence.type;
		stored.state = Text(presence.state);
		stored.details = Text(presence.details);
		stored.startTimestamp = presence.startTimestamp;
		stored.endTimestamp = presence.endTimestamp;
		stored.largeImageKey = Text(presence.largeImageKey);
		stored.largeImageText = Text(presence.largeImageText);
		stored.smallImageKey = Text(presence.smallImageKey);
		stored.smallImageText = Text(presence.smallImageText);
		stored.partyId = Text(presence.partyId);
		stored.partySize = presence.partySize;
		stored.partyMax = presence.partyMax;
		stored.partyPrivacy = presence.partyPrivacy;
		for (int i = 0; i < 2; i++)
		{
			stored.buttons[i].label = Text(presence.buttons[i].label);
			stored.buttons[i].url = Text(presence.buttons[i].url);
		}
		return stored;
	}

	discordpp::Activity Build(const StoredPresence &presence)
	{
		discordpp::Activity activity;
		if (Name)
			activity.SetName(*Name);
		activity.SetType(static_cast<discordpp::ActivityTypes>(presence.type));
		activity.SetState(presence.state);
		activity.SetDetails(presence.details);

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

		if (presence.largeImageKey || presence.smallImageKey)
		{
			discordpp::ActivityAssets assets;
			assets.SetLargeImage(presence.largeImageKey);
			assets.SetLargeText(presence.largeImageKey ? presence.largeImageText : std::optional<std::string>());
			assets.SetSmallImage(presence.smallImageKey);
			assets.SetSmallText(presence.smallImageKey ? presence.smallImageText : std::optional<std::string>());
			activity.SetAssets(assets);
		}

		if (presence.partyId && presence.partyMax > 0)
		{
			discordpp::ActivityParty party;
			party.SetId(*presence.partyId);
			party.SetCurrentSize(std::max(0, std::min(presence.partySize, presence.partyMax)));
			party.SetMaxSize(presence.partyMax);
			party.SetPrivacy(presence.partyPrivacy == 1 ? discordpp::ActivityPartyPrivacy::Public : discordpp::ActivityPartyPrivacy::Private);
			activity.SetParty(party);
		}

		for (const StoredButton &entry : presence.buttons)
		{
			if (!entry.label || !entry.url)
				continue;

			discordpp::ActivityButton button;
			button.SetLabel(entry.label->substr(0, 32));
			button.SetUrl(*entry.url);
			activity.AddButton(button);
		}

		return activity;
	}

	void Report(hxdiscord_social::Event event, int code, const std::string &message)
	{
		if (OnEvent != nullptr)
			OnEvent(event, code, message.c_str());
	}

	std::string TokenFile()
	{
		if (StoragePath.empty() || ApplicationId == 0)
			return std::string();

		return StoragePath + "/hxdiscord_rpc_" + std::to_string(ApplicationId) + ".token";
	}

	std::string LoadRefreshToken()
	{
		const std::string file = TokenFile();
		if (file.empty())
			return std::string();

		std::ifstream input(file);
		std::string token;
		std::getline(input, token);
		return token;
	}

	void SaveRefreshToken(const std::string &token)
	{
		const std::string file = TokenFile();
		if (file.empty() || token.empty())
			return;

		std::ofstream output(file, std::ios::trunc);
		output << token;
	}

	void DeleteRefreshToken()
	{
		const std::string file = TokenFile();
		if (!file.empty())
			std::remove(file.c_str());
	}

	bool IsReady()
	{
		return Client && Client->GetStatus() == discordpp::Client::Status::Ready;
	}

	void Authorize();

	void TokenReceived(discordpp::ClientResult result, std::string accessToken, std::string refreshToken, discordpp::AuthorizationTokenType tokenType,
		int32_t expiresIn, std::string scopes)
	{
		(void)expiresIn;
		(void)scopes;

		std::lock_guard<std::recursive_mutex> guard(Lock);

		Refreshing = false;

		if (!Client)
			return;

		if (!result.Successful())
		{
			if (result.Retryable())
			{
				RefreshAttempts++;
				RefreshPending = true;
				RefreshAt = Clock::now() + Backoff(RefreshAttempts);
				return;
			}

			DeleteRefreshToken();
			Report(hxdiscord_social::Event::Errored, static_cast<int>(result.ErrorCode()), result.Error());
			Authorize();
			return;
		}

		RefreshAttempts = 0;
		SaveRefreshToken(refreshToken);

		Client->UpdateToken(tokenType, accessToken, [](discordpp::ClientResult updated) {
			std::lock_guard<std::recursive_mutex> guard(Lock);

			if (!Client)
				return;

			if (!updated.Successful())
			{
				Report(hxdiscord_social::Event::Errored, static_cast<int>(updated.ErrorCode()), updated.Error());
				return;
			}

			const auto status = Client->GetStatus();
			if (status == discordpp::Client::Status::Disconnected)
				Client->Connect();
		});
	}

	void Refresh()
	{
		if (!Client || Refreshing)
			return;

		const std::string token = LoadRefreshToken();
		if (token.empty())
		{
			Authorize();
			return;
		}

		Refreshing = true;
		RefreshPending = false;
		Client->RefreshToken(ApplicationId, token, TokenReceived);
	}

	void Authorize()
	{
		if (!Client || Authorizing || AuthorizeDenied)
			return;

		Authorizing = true;

		auto verifier = Client->CreateAuthorizationCodeVerifier();

		discordpp::AuthorizationArgs args{};
		args.SetClientId(ApplicationId);
		args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
		args.SetCodeChallenge(verifier.Challenge());

		Client->Authorize(args, [verifier](discordpp::ClientResult result, std::string code, std::string redirectUri) {
			std::lock_guard<std::recursive_mutex> guard(Lock);

			Authorizing = false;

			if (!Client)
				return;

			if (!result.Successful())
			{
				AuthorizeDenied = true;
				Report(hxdiscord_social::Event::Errored, static_cast<int>(result.ErrorCode()), result.Error());
				return;
			}

			Refreshing = true;
			Client->GetToken(ApplicationId, code, verifier.Verifier(), redirectUri, TokenReceived);
		});
	}

	void StatusChanged(discordpp::Client::Status status, discordpp::Client::Error error, int32_t detail)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (status == discordpp::Client::Status::Ready)
		{
			WasReady = true;

			if (Last)
			{
				Dirty = true;
				InFlight = false;
				Attempts = 0;
				NextAttempt = Clock::time_point{};
			}

			Report(hxdiscord_social::Event::Ready, 0, std::string());
			return;
		}

		if (status != discordpp::Client::Status::Disconnected)
			return;

		InFlight = false;

		if (WasReady || error != discordpp::Client::Error::None)
		{
			std::string message = discordpp::Client::ErrorToString(error);
			if (error == discordpp::Client::Error::None)
				message = discordpp::Client::StatusToString(status);

			Report(hxdiscord_social::Event::Disconnected, static_cast<int>(error) * 10000 + detail, message);
		}

		WasReady = false;

		if (error != discordpp::Client::Error::None)
		{
			RefreshPending = true;
			RefreshAttempts++;
			RefreshAt = Clock::now() + Backoff(RefreshAttempts);
		}
	}

	void Send(Clock::time_point now)
	{
		if (!IsReady() || !Last || !Dirty || InFlight || !IsOnline || now < NextAttempt)
			return;

		const uint64_t generation = Generation;
		Dirty = false;
		InFlight = true;
		SentAt = now;

		Client->UpdateRichPresence(Build(*Last), [generation](discordpp::ClientResult result) {
			std::lock_guard<std::recursive_mutex> guard(Lock);

			if (generation != Generation)
				return;

			InFlight = false;
			const auto finished = Clock::now();

			if (result.Successful())
			{
				Attempts = 0;
				LastSuccess = finished;
				return;
			}

			Attempts++;
			Dirty = true;
			NextAttempt = finished + Backoff(Attempts);

			if (Attempts == 1)
				Report(hxdiscord_social::Event::Errored, static_cast<int>(result.ErrorCode()), result.Error());
		});
	}

	void Reset()
	{
		Generation++;
		Last.reset();
		Dirty = false;
		InFlight = false;
		Cleared = false;
		Attempts = 0;
		NextAttempt = Clock::time_point{};
		LastSuccess = Clock::time_point{};
	}
}

namespace hxdiscord_social
{
	bool Start(uint64_t applicationId, EventCallback onEvent)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		OnEvent = onEvent;

		if (applicationId == 0)
			return false;

		if (Client && ApplicationId != applicationId)
		{
			Client.reset();
			Reset();
		}

		ApplicationId = applicationId;

		if (!Client)
		{
			Client = std::make_unique<discordpp::Client>();
			Reset();
			Authorizing = false;
			AuthorizeDenied = false;
			Refreshing = false;
			RefreshPending = false;
			RefreshAttempts = 0;
			WasReady = false;

			Client->SetApplicationId(applicationId);
			Client->SetStatusChangedCallback(StatusChanged);
			Client->SetTokenExpirationCallback([]() {
				std::lock_guard<std::recursive_mutex> guard(Lock);
				Refresh();
			});

			Refresh();
		}
		else if (IsReady())
			Report(Event::Ready, 0, std::string());

		if (Last)
		{
			Dirty = true;
			InFlight = false;
			Attempts = 0;
			NextAttempt = Clock::time_point{};
		}

		return true;
	}

	void Stop()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!Client)
			return;

		if (IsReady())
			Client->ClearRichPresence();

		Client->SetStatusChangedCallback([](discordpp::Client::Status, discordpp::Client::Error, int32_t) {});
		Client->Disconnect();
		discordpp::RunCallbacks();
		Client.reset();
		Reset();
		OnEvent = nullptr;
		WasReady = false;
	}

	bool Ready()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);
		return IsReady();
	}

	bool CurrentUser(User &user)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!IsReady())
			return false;

		auto handle = Client->GetCurrentUserV2();
		if (!handle || handle->Id() == 0)
			return false;

		user.id = std::to_string(handle->Id());
		user.username = handle->Username();
		user.globalName = handle->GlobalName().value_or(std::string());
		user.avatar = handle->Avatar().value_or(std::string());
		return true;
	}

	void SetStoragePath(const char *path)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);
		StoragePath = path != nullptr ? path : "";
	}

	bool Running()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);
		return Client != nullptr;
	}

	void Tick()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!Client)
			return;

		discordpp::RunCallbacks();

		const auto now = Clock::now();

		const bool networkBack = NetworkChanged.exchange(false) && IsOnline;

		if (networkBack && Last)
		{
			Dirty = true;
			Attempts = 0;
			NextAttempt = now + NetworkSettle;
		}

		if (networkBack && RefreshPending)
			RefreshAt = now + NetworkSettle;

		if (RefreshPending && IsOnline && !Refreshing && !Authorizing && now >= RefreshAt && Client->GetStatus() == discordpp::Client::Status::Disconnected)
			Refresh();

		if (InFlight && now - SentAt > RequestTimeout)
		{
			InFlight = false;
			Dirty = true;
			Attempts++;
			NextAttempt = now + Backoff(Attempts);
		}

		if (Last && !Dirty && !InFlight && LastSuccess != Clock::time_point{} && now - LastSuccess > Heartbeat)
			Dirty = true;

		Send(now);
	}

	void Update(const Presence &presence)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!Client)
			return;

		StoredPresence stored = Store(presence);
		if (Last && *Last == stored && !Cleared)
			return;

		Last = std::move(stored);
		Cleared = false;
		Dirty = true;
		Attempts = 0;
		NextAttempt = Clock::time_point{};

		if (InFlight)
		{
			Generation++;
			InFlight = false;
		}

		Send(Clock::now());
	}

	void Clear()
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		if (!Client)
			return;

		Generation++;
		Last.reset();
		Dirty = false;
		InFlight = false;
		Attempts = 0;
		Cleared = true;

		if (IsReady())
			Client->ClearRichPresence();
	}

	void SetName(const char *name)
	{
		std::lock_guard<std::recursive_mutex> guard(Lock);

		std::optional<std::string> value = Text(name);
		if (value == Name)
			return;

		Name = std::move(value);
		if (Last)
		{
			Dirty = true;
			Attempts = 0;
			NextAttempt = Clock::time_point{};
		}
	}

	void SetOnline(bool online)
	{
		const bool previous = IsOnline.exchange(online);
		if (online && !previous)
			NetworkChanged = true;
	}

	bool Online()
	{
		return IsOnline;
	}
}

#if defined(__ANDROID__)
extern "C" JNIEXPORT void JNICALL Java_org_haxe_extension_DiscordSocialInit_nativeSetStoragePath(JNIEnv *env, jclass type, jstring path)
{
	(void)type;

	if (path == nullptr)
		return;

	const char *chars = env->GetStringUTFChars(path, nullptr);
	if (chars == nullptr)
		return;

	hxdiscord_social::SetStoragePath(chars);
	env->ReleaseStringUTFChars(path, chars);
}

extern "C" JNIEXPORT void JNICALL Java_org_haxe_extension_DiscordSocialInit_nativeNetworkChanged(JNIEnv *env, jclass type, jboolean online)
{
	(void)env;
	(void)type;
	hxdiscord_social::SetOnline(online == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL Java_org_haxe_extension_DiscordSocialInit_nativeResumed(JNIEnv *env, jclass type)
{
	(void)env;
	(void)type;
	std::lock_guard<std::recursive_mutex> guard(Lock);
	if (Last && !InFlight)
	{
		Dirty = true;
		NextAttempt = Clock::now() + NetworkSettle;
	}
}
#endif
