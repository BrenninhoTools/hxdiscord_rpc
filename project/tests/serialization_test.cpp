#include "discord_rpc.hpp"
#include "serialization.hpp"
#include <cstdio>
#include <cstring>

static int failures = 0;

static void expect(const char *json, const char *needle, bool present)
{
	const bool found = std::strstr(json, needle) != nullptr;

	if (found != present)
	{
		std::printf("FAIL: %s \"%s\"\n%s\n", present ? "missing" : "unexpected", needle, json);
		failures++;
	}
}

int main()
{
	char buffer[16 * 1024];

	DiscordRichPresence presence = {};
	presence.state = "Playing";
	presence.details = "Week 1";
	presence.startTimestamp = 1700000000;
	presence.largeImageKey = "icon";
	presence.largeImageText = "Funkin";
	presence.largeImageUrl = "https://example.com/large";
	presence.partyId = "party";
	presence.partySize = 1;
	presence.partyMax = 4;
	presence.buttons[0].label = "Site";
	presence.buttons[0].url = "https://example.com";
	presence.joinSecret = "secret";
	presence.statusDisplayType = 2;
	presence.stateUrl = "https://example.com/state";
	presence.detailsUrl = "https://example.com/details";

	size_t length = JsonWriteRichPresenceObj(buffer, sizeof(buffer), 1, 42, &presence);

	if (length == 0)
	{
		std::printf("FAIL: empty payload\n");
		return 1;
	}

	expect(buffer, "\"status_display_type\":2", true);
	expect(buffer, "\"state_url\":\"https://example.com/state\"", true);
	expect(buffer, "\"details_url\":\"https://example.com/details\"", true);
	expect(buffer, "\"large_url\":\"https://example.com/large\"", true);
	expect(buffer, "\"buttons\":[", true);
	expect(buffer, "\"party\":{", true);
	expect(buffer, "\"size\":[1,4]", true);
	expect(buffer, "\"secrets\"", false);

	presence.buttons[0].label = nullptr;
	presence.buttons[0].url = nullptr;
	presence.statusDisplayType = 0;
	presence.stateUrl = nullptr;
	length = JsonWriteRichPresenceObj(buffer, sizeof(buffer), 2, 42, &presence);

	expect(buffer, "\"buttons\"", false);
	expect(buffer, "\"secrets\":{", true);
	expect(buffer, "\"status_display_type\"", false);
	expect(buffer, "\"state_url\"", false);

	if (failures == 0)
		std::printf("ok\n");

	return failures == 0 ? 0 : 1;
}
