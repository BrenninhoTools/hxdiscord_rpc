package;

import hxdiscord_rpc.DiscordEvents;

class Main
{
	public static function main():Void
	{
		if (!DiscordEvents.supported)
		{
			Sys.println('Discord RPC is not available on this platform.');
			return;
		}

		Sys.println('Initializing Discord RPC...');

		DiscordEvents.init('345229890980937739');

		final start:Float = Sys.time();

		while (Sys.time() - start < 10)
		{
			DiscordEvents.update();

			DiscordEvents.pollAll(function(event:DiscordEvent):Void
			{
				switch (event)
				{
					case Ready(user):
						if (user.discriminator != '0' && user.discriminator.length > 0)
							Sys.println('Discord: Connected to user ${user.username}#${user.discriminator} (${user.globalName})');
						else
							Sys.println('Discord: Connected to user @${user.username} (${user.globalName})');

						final presence:DiscordPresence = new DiscordPresence();
						presence.type = 3;
						presence.state = 'West of House';
						presence.details = 'Frustração';
						presence.largeImageKey = 'canary-large';
						presence.smallImageKey = 'ptb-small';
						presence.statusDisplayType = 2;
						presence.detailsUrl = 'https://example.com';
						presence.setButton(0, 'Test 1', 'https://example.com');
						presence.setButton(1, 'Test 2', 'https://discord.gg/fortnite');
						presence.setParty('sample', 1, 4).progress(0, 180);
						DiscordEvents.updatePresence(presence);
					case Disconnected(code, message):
						Sys.println('Discord: Disconnected ($code:$message)');
					case Errored(code, message):
						Sys.println('Discord: Error ($code:$message)');
					default:
				}
			});

			Sys.sleep(0.5);
		}

		Sys.println('Shutting down Discord RPC...');

		DiscordEvents.shutdown();
	}
}
