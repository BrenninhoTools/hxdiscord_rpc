package hxdiscord_rpc;

#if !cpp
#error 'Discord RPC supports only C++ target platforms.'
#end
import hxdiscord_rpc.Types;

typedef DiscordUserData =
{
	var userId:String;
	var username:String;
	var globalName:String;
	var discriminator:String;
	var avatar:String;
	var premiumType:Int;
	var bot:Bool;
}

enum DiscordEvent
{
	Ready(user:DiscordUserData);
	Disconnected(errorCode:Int, message:String);
	Errored(errorCode:Int, message:String);
	JoinGame(secret:String);
	SpectateGame(secret:String);
	JoinRequest(user:DiscordUserData);
}

class DiscordPresence
{
	public var type:Int = 0;
	public var state:String = null;
	public var details:String = null;
	public var startTimestamp:Float = 0;
	public var endTimestamp:Float = 0;
	public var largeImageKey:String = null;
	public var largeImageText:String = null;
	public var smallImageKey:String = null;
	public var smallImageText:String = null;
	public var partyId:String = null;
	public var partySize:Int = 0;
	public var partyMax:Int = 0;
	public var partyPrivacy:Int = 0;
	public var button1Label:String = null;
	public var button1Url:String = null;
	public var button2Label:String = null;
	public var button2Url:String = null;
	public var matchSecret:String = null;
	public var joinSecret:String = null;
	public var spectateSecret:String = null;
	public var instance:Bool = false;

	public function new() {}

	public function copy():DiscordPresence
	{
		var presence:DiscordPresence = new DiscordPresence();
		for (field in Reflect.fields(this))
			Reflect.setField(presence, field, Reflect.field(this, field));
		return presence;
	}
}

@:buildXml('<include name="${haxelib:hxdiscord_rpc}/project/Build.xml" />')
@:cppFileCode('#include "discord_rpc_queue.hpp"')
class DiscordEvents
{
	public static var initialized(default, null):Bool = false;

	public static var supported(get, never):Bool;

	public static var ready(get, never):Bool;

	public static var user(get, never):Null<DiscordUserData>;

	public static function init(applicationId:String, autoRegister:Bool = false, ?steamId:String):Bool
	{
		if (!Discord.IsSupported() || applicationId == null || applicationId.length == 0)
			return false;

		if (initialized)
			DiscordQueue.Shutdown();

		DiscordQueue.Initialize(utf8(applicationId), autoRegister, utf8(steamId));
		initialized = true;
		return true;
	}

	public static function shutdown():Void
	{
		if (!initialized)
			return;

		DiscordQueue.Shutdown();
		initialized = false;
	}

	public static function update():Void
	{
		if (initialized)
			DiscordQueue.Update();
	}

	public static function poll():Null<DiscordEvent>
	{
		if (!initialized)
			return null;

		return switch (DiscordQueue.Next())
		{
			case 1: Ready(readUser());
			case 2: Disconnected(DiscordQueue.ErrorCode(), field(0));
			case 3: Errored(DiscordQueue.ErrorCode(), field(0));
			case 4: JoinGame(field(1));
			case 5: SpectateGame(field(1));
			case 6: JoinRequest(readUser());
			default: null;
		}
	}

	public static function pollAll(handler:DiscordEvent->Void):Int
	{
		var count:Int = 0;
		var event:Null<DiscordEvent> = poll();
		while (event != null && count < 64)
		{
			handler(event);
			count++;
			event = poll();
		}
		return count;
	}

	public static function updatePresence(presence:DiscordPresence):Void
	{
		if (!initialized || presence == null)
			return;

		DiscordQueue.UpdatePresence(presence.type, utf8(presence.state), utf8(presence.details), presence.startTimestamp, presence.endTimestamp,
			utf8(presence.largeImageKey), utf8(presence.largeImageText), utf8(presence.smallImageKey), utf8(presence.smallImageText),
			utf8(presence.partyId), presence.partySize, presence.partyMax, presence.partyPrivacy, utf8(presence.button1Label),
			utf8(presence.button1Url), utf8(presence.button2Label), utf8(presence.button2Url), utf8(presence.matchSecret),
			utf8(presence.joinSecret), utf8(presence.spectateSecret), presence.instance);
	}

	public static function clearPresence():Void
	{
		if (initialized)
			Discord.ClearPresence();
	}

	public static function respond(userId:String, accept:Bool):Void
	{
		if (initialized && userId != null)
			Discord.Respond(utf8(userId), accept ? DiscordActivityJoinRequestReply_Yes : DiscordActivityJoinRequestReply_No);
	}

	static function readUser():DiscordUserData
	{
		return {
			userId: field(2),
			username: field(3),
			globalName: field(4),
			discriminator: field(5),
			avatar: field(6),
			premiumType: DiscordQueue.PremiumType(),
			bot: DiscordQueue.Bot()
		};
	}

	static function field(index:Int):String
	{
		return untyped __cpp__('::String::create(DiscordQueue_String({0}))', index);
	}

	static function connected(index:Int):String
	{
		return switch (index)
		{
			case 0: untyped __cpp__('::String::create(DiscordQueue_ConnectedUserId())');
			case 1: untyped __cpp__('::String::create(DiscordQueue_ConnectedUsername())');
			case 2: untyped __cpp__('::String::create(DiscordQueue_ConnectedGlobalName())');
			default: untyped __cpp__('::String::create(DiscordQueue_ConnectedAvatar())');
		}
	}

	static inline function utf8(value:String):cpp.ConstCharStar
	{
		return untyped __cpp__('({0}.__s ? {0}.utf8_str() : (const char *)0)', value);
	}

	static function get_supported():Bool
	{
		return Discord.IsSupported();
	}

	static function get_ready():Bool
	{
		return initialized && Discord.IsReady();
	}

	static function get_user():Null<DiscordUserData>
	{
		if (!initialized || !DiscordQueue.HasUser())
			return null;

		return {
			userId: connected(0),
			username: connected(1),
			globalName: connected(2),
			discriminator: '0',
			avatar: connected(3),
			premiumType: 0,
			bot: false
		};
	}
}
