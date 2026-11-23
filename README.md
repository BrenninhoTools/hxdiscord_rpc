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

### Rich Presence on Android

Discord only shows Rich Presence on Android through the [Discord Social SDK](https://discord.com/developers/docs/discord-social-sdk/overview), with the Discord app installed and signed in. The SDK is not bundled here, download it from the Developer Portal of your application.

1. Prepare the SDK once:
```bash
python3 tools/prepare_social_sdk.py DiscordSocialSdk-x.y.z.zip discord-social-sdk
```
2. Build with the prepared folder:
```bash
lime build android -DDISCORD_SOCIAL_SDK=/full/path/to/discord-social-sdk
```

The same `Discord` API keeps working, and `Discord.IsSupported()` returns `true`. Without `DISCORD_SOCIAL_SDK` the library still builds on Android and iOS, but `Discord.IsSupported()` returns `false`. Voice permissions from the SDK are removed unless `--keep-voice` is passed.

### Usage Example

Check out the [Sample Folder](sample/) for an example on how to use this library.

### Licensing

**hxdiscord_rpc** is made available under the **MIT License**. Check [LICENSE](./LICENSE) for more information.

![](https://raw.githubusercontent.com/github/explore/2a3ce46f963399611d8e2054bb0ce9a4b539296a/topics/discord/discord.png)

**Discord RPC** is made available under the **MIT License**. Check [Discord's RPC LICENSE](https://github.com/discord/discord-rpc/blob/master/LICENSE) for more information.
