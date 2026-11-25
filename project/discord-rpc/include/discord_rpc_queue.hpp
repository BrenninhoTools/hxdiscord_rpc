#pragma once

#include <stdbool.h>

typedef enum DiscordQueueEventType
{
	DiscordQueueEventType_None = 0,
	DiscordQueueEventType_Ready = 1,
	DiscordQueueEventType_Disconnected = 2,
	DiscordQueueEventType_Errored = 3,
	DiscordQueueEventType_JoinGame = 4,
	DiscordQueueEventType_SpectateGame = 5,
	DiscordQueueEventType_JoinRequest = 6
} DiscordQueueEventType;

typedef enum DiscordQueueField
{
	DiscordQueueField_Message = 0,
	DiscordQueueField_Secret = 1,
	DiscordQueueField_UserId = 2,
	DiscordQueueField_Username = 3,
	DiscordQueueField_GlobalName = 4,
	DiscordQueueField_Discriminator = 5,
	DiscordQueueField_Avatar = 6
} DiscordQueueField;

void DiscordQueue_Initialize(const char *applicationId, bool autoRegister, const char *optionalSteamId);
void DiscordQueue_Shutdown(void);
void DiscordQueue_Update(void);
int DiscordQueue_Next(void);
int DiscordQueue_Pending(void);
void DiscordQueue_Clear(void);
const char *DiscordQueue_String(int field);
int DiscordQueue_ErrorCode(void);
int DiscordQueue_PremiumType(void);
bool DiscordQueue_Bot(void);
const char *DiscordQueue_ConnectedUserId(void);
const char *DiscordQueue_ConnectedUsername(void);
const char *DiscordQueue_ConnectedGlobalName(void);
const char *DiscordQueue_ConnectedAvatar(void);
bool DiscordQueue_HasUser(void);
void DiscordQueue_UpdatePresence(int type, const char *state, const char *details, double startTimestamp, double endTimestamp, const char *largeImageKey,
	const char *largeImageText, const char *smallImageKey, const char *smallImageText, const char *partyId, int partySize, int partyMax, int partyPrivacy,
	const char *button1Label, const char *button1Url, const char *button2Label, const char *button2Url, const char *matchSecret, const char *joinSecret,
	const char *spectateSecret, bool instance);
void DiscordQueue_UpdatePresenceEx(int type, const char *state, const char *details, double startTimestamp, double endTimestamp, const char *largeImageKey,
	const char *largeImageText, const char *smallImageKey, const char *smallImageText, const char *partyId, int partySize, int partyMax, int partyPrivacy,
	const char *button1Label, const char *button1Url, const char *button2Label, const char *button2Url, const char *matchSecret, const char *joinSecret,
	const char *spectateSecret, bool instance, int statusDisplayType, const char *stateUrl, const char *detailsUrl, const char *largeImageUrl,
	const char *smallImageUrl);
void DiscordQueue_SetActivityName(const char *name);
