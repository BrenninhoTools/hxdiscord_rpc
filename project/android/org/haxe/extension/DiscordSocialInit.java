package org.haxe.extension;

import android.os.Bundle;

import com.discord.socialsdk.DiscordSocialSdkInit;

public class DiscordSocialInit extends Extension
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		try
		{
			DiscordSocialSdkInit.setEngineActivity(mainActivity);
		}
		catch (Throwable ignored) {}
	}

	@Override
	public void onResume()
	{
		try
		{
			if (DiscordSocialSdkInit.getEngineActivity() == null)
				DiscordSocialSdkInit.setEngineActivity(mainActivity);
		}
		catch (Throwable ignored) {}
	}
}
