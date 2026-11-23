# hxdiscord_rpc

![](https://img.shields.io/github/repo-size/BrenninhoTools/hxdiscord_rpc) ![](https://badgen.net/github/open-issues/BrenninhoTools/hxdiscord_rpc) ![](https://badgen.net/badge/license/MIT/green)

Haxe/hxcpp @:native bindings for [Discord RPC](https://github.com/discord/discord-rpc).

> [!NOTE]
> These bindings are using a fork of Discord RPC, you can read more about it [here](https://github.com/BrenninhoTools/hxdiscord_rpc/tree/main/project/discord-rpc).

### Installation

You can install it through `Haxelib`
```bash
haxelib install hxdiscord_rpc
```
Or through `Git`, if you want the latest updates
```bash
haxelib git hxdiscord_rpc https://github.com/BrenninhoTools/hxdiscord_rpc.git
```

### Platforms

| Platform | Status |
| --- | --- |
| Windows | Full Rich Presence |
| macOS | Full Rich Presence |
| Linux | Full Rich Presence (including Snap Discord) |
| Android | Compiles and runs, calls are no-ops |
| iOS | Compiles and runs, calls are no-ops |

Discord's mobile apps don't expose the local IPC that Rich Presence relies on, so on Android and iOS the library ships a stub backend: every function is safe to call, nothing is sent and no background thread is started. This lets the same code build for every target without `#if desktop` guards.

Use `Discord.IsSupported()` to check at runtime:

```haxe
if (Discord.IsSupported())
	Discord.Initialize("APPLICATION_ID", cpp.RawPointer.addressOf(handlers), false, null);
```

### Usage Example

Check out the [Sample Folder](sample/) for an example on how to use this library.

### Licensing

**hxdiscord_rpc** is made available under the **MIT License**. Check [LICENSE](./LICENSE) for more information.

![](https://raw.githubusercontent.com/github/explore/2a3ce46f963399611d8e2054bb0ce9a4b539296a/topics/discord/discord.png)

**Discord RPC** is made available under the **MIT License**. Check [Discord's RPC LICENSE](https://github.com/discord/discord-rpc/blob/master/LICENSE) for more information.
