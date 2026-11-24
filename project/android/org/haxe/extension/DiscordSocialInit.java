package org.haxe.extension;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.os.Build;
import android.os.Bundle;

import com.discord.socialsdk.DiscordSocialSdkInit;

public class DiscordSocialInit extends Extension
{
	private static ConnectivityManager.NetworkCallback networkCallback;

	private static native void nativeNetworkChanged(boolean online);

	private static native void nativeResumed();

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		attach();
		registerNetwork();
	}

	@Override
	public void onResume()
	{
		attach();
		notifyNetwork(isOnline());

		try
		{
			nativeResumed();
		}
		catch (Throwable ignored) {}
	}

	@Override
	public void onDestroy()
	{
		unregisterNetwork();
	}

	private static void attach()
	{
		try
		{
			if (mainActivity != null)
				DiscordSocialSdkInit.setEngineActivity(mainActivity);
		}
		catch (Throwable ignored) {}
	}

	private static ConnectivityManager manager()
	{
		try
		{
			Context context = mainContext != null ? mainContext : mainActivity;
			if (context == null)
				return null;

			return (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		}
		catch (Throwable ignored)
		{
			return null;
		}
	}

	private static boolean isOnline()
	{
		ConnectivityManager manager = manager();
		if (manager == null)
			return true;

		try
		{
			if (Build.VERSION.SDK_INT >= 23)
			{
				Network network = manager.getActiveNetwork();
				if (network == null)
					return false;

				NetworkCapabilities capabilities = manager.getNetworkCapabilities(network);
				return capabilities != null && capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
			}

			android.net.NetworkInfo info = manager.getActiveNetworkInfo();
			return info != null && info.isConnected();
		}
		catch (Throwable ignored)
		{
			return true;
		}
	}

	private static void notifyNetwork(boolean online)
	{
		try
		{
			nativeNetworkChanged(online);
		}
		catch (Throwable ignored) {}
	}

	private static void registerNetwork()
	{
		if (networkCallback != null || Build.VERSION.SDK_INT < 24)
			return;

		ConnectivityManager manager = manager();
		if (manager == null)
			return;

		ConnectivityManager.NetworkCallback callback = new ConnectivityManager.NetworkCallback()
		{
			@Override
			public void onAvailable(Network network)
			{
				notifyNetwork(true);
			}

			@Override
			public void onLost(Network network)
			{
				notifyNetwork(isOnline());
			}

			@Override
			public void onCapabilitiesChanged(Network network, NetworkCapabilities capabilities)
			{
				notifyNetwork(capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET));
			}
		};

		try
		{
			manager.registerDefaultNetworkCallback(callback);
			networkCallback = callback;
		}
		catch (Throwable ignored) {}
	}

	private static void unregisterNetwork()
	{
		if (networkCallback == null)
			return;

		ConnectivityManager manager = manager();
		try
		{
			if (manager != null)
				manager.unregisterNetworkCallback(networkCallback);
		}
		catch (Throwable ignored) {}

		networkCallback = null;
	}
}
