# hxdiscord_rpc

![](https://img.shields.io/github/repo-size/MAJigsaw77/hxdiscord_rpc) ![](https://badgen.net/github/open-issues/MAJigsaw77/hxdiscord_rpc) ![](https://badgen.net/badge/license/MIT/green)

Haxe/hxcpp @:native bindings for [Discord RPC](https://github.com/discord/discord-rpc).

> [!NOTE]
> These bindings are using a fork of Discord RPC, you can read more about it [here](https://github.com/MAJigsaw77/hxdiscord_rpc/tree/main/project/discord-rpc).

### Installation

You can install it through `Haxelib`
```bash
haxelib install hxdiscord_rpc
```
Or through `Git`, if you want the latest updates
```bash
haxelib git hxdiscord_rpc https://github.com/MAJigsaw77/hxdiscord_rpc.git
```

### Usage

The recommended way is `hxdiscord_rpc.DiscordEvents`. The Discord callbacks are installed on the C++ side and every event is stored in a native queue, so no Haxe function is ever passed to Discord RPC. It works on scriptable (cppia) builds, where `cpp.Function.fromStaticFunction` fails to compile, and on every compiler.

```haxe
import hxdiscord_rpc.DiscordEvents;

DiscordEvents.init('YOUR_APPLICATION_ID');

function update():Void
{
	DiscordEvents.update();
	DiscordEvents.pollAll(function(event)
	{
		switch (event)
		{
			case Ready(user):
				var presence = new DiscordPresence();
				presence.details = 'Playing a song';
				presence.state = 'Ação';
				presence.statusDisplayType = 2;
				presence.setButton(0, 'Download', 'https://example.com').setParty('lobby', 1, 4).startNow();
				DiscordEvents.updatePresence(presence);
			case Disconnected(code, message):
			case Errored(code, message):
			default:
		}
	});
}
```

`DiscordPresence` keeps plain Haxe strings and sends them as UTF-8, so accents and emoji show correctly. The raw `Discord` and `Types` bindings are still available.

`statusDisplayType` picks what the member list shows (0 name, 1 state, 2 details), `stateUrl`, `detailsUrl`, `largeImageUrl` and `smallImageUrl` make those fields clickable, and `startNow`, `countdown` and `progress` fill the timestamps. Party info is now sent together with buttons; only secrets are dropped when buttons are set, as Discord requires.

### Usage Example

Check out the [Sample Folder](sample/) for an example on how to use this library.

### Licensing

**hxdiscord_rpc** is made available under the **MIT License**. Check [LICENSE](./LICENSE) for more information.

![](https://raw.githubusercontent.com/github/explore/2a3ce46f963399611d8e2054bb0ce9a4b539296a/topics/discord/discord.png)

**Discord RPC** is made available under the **MIT License**. Check [Discord's RPC LICENSE](https://github.com/discord/discord-rpc/blob/master/LICENSE) for more information.
