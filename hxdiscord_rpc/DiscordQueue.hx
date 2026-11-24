package hxdiscord_rpc;

#if !cpp
#error 'Discord RPC supports only C++ target platforms.'
#end

@:buildXml('<include name="${haxelib:hxdiscord_rpc}/project/Build.xml" />')
@:include('discord_rpc_queue.hpp')
@:unreflective
extern class DiscordQueue
{
	@:native('DiscordQueue_Initialize')
	static function Initialize(applicationId:cpp.ConstCharStar, autoRegister:Bool, optionalSteamId:cpp.ConstCharStar):Void;

	@:native('DiscordQueue_Shutdown')
	static function Shutdown():Void;

	@:native('DiscordQueue_Update')
	static function Update():Void;

	@:native('DiscordQueue_Next')
	static function Next():Int;

	@:native('DiscordQueue_Pending')
	static function Pending():Int;

	@:native('DiscordQueue_Clear')
	static function Clear():Void;

	@:native('DiscordQueue_ErrorCode')
	static function ErrorCode():Int;

	@:native('DiscordQueue_PremiumType')
	static function PremiumType():Int;

	@:native('DiscordQueue_Bot')
	static function Bot():Bool;

	@:native('DiscordQueue_HasUser')
	static function HasUser():Bool;

	@:native('DiscordQueue_UpdatePresence')
	static function UpdatePresence(type:Int, state:cpp.ConstCharStar, details:cpp.ConstCharStar, startTimestamp:Float, endTimestamp:Float,
		largeImageKey:cpp.ConstCharStar, largeImageText:cpp.ConstCharStar, smallImageKey:cpp.ConstCharStar, smallImageText:cpp.ConstCharStar,
		partyId:cpp.ConstCharStar, partySize:Int, partyMax:Int, partyPrivacy:Int, button1Label:cpp.ConstCharStar, button1Url:cpp.ConstCharStar,
		button2Label:cpp.ConstCharStar, button2Url:cpp.ConstCharStar, matchSecret:cpp.ConstCharStar, joinSecret:cpp.ConstCharStar,
		spectateSecret:cpp.ConstCharStar, instance:Bool):Void;
}
