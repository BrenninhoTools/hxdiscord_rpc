import argparse
import io
import os
import re
import shutil
import sys
import zipfile

VOICE_PERMISSIONS = [
    'android.permission.RECORD_AUDIO',
    'android.permission.FOREGROUND_SERVICE',
    'android.permission.FOREGROUND_SERVICE_MICROPHONE',
    'android.permission.FOREGROUND_SERVICE_MEDIA_PLAYBACK',
    'android.permission.MODIFY_AUDIO_SETTINGS',
    'android.permission.BLUETOOTH',
    'android.permission.BLUETOOTH_CONNECT',
]

BUILD_GRADLE = '''configurations.maybeCreate("default")
artifacts.add("default", file('discord_partner_sdk.aar'))
dependencies.add("default", "androidx.browser:browser:1.8.0")
'''

AUTH_ACTIVITY = '''
        <activity android:name="com.discord.socialsdk.AuthenticationActivity" android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.VIEW" />
                <category android:name="android.intent.category.DEFAULT" />
                <category android:name="android.intent.category.BROWSABLE" />
                <data android:scheme="discord-{app_id}" />
            </intent-filter>
        </activity>
'''


def find(names, suffix):
    for name in names:
        if name.endswith(suffix):
            return name
    return None


def strip_voice(manifest):
    for permission in VOICE_PERMISSIONS:
        manifest = re.sub(r'\s*<uses-permission[^>]*android:name="' + re.escape(permission) + r'"[^>]*/>', '', manifest)
    manifest = re.sub(r'\s*<service[^>]*com\.discord\.socialsdk\.ForegroundService[^>]*?(/>|>.*?</service>)', '', manifest, flags=re.S)
    return manifest


def add_auth_activity(manifest, app_id):
    manifest = re.sub(r'\s*<activity[^>]*com\.discord\.socialsdk\.AuthenticationActivity[^>]*?(/>|>.*?</activity>)', '', manifest, flags=re.S)
    activity = AUTH_ACTIVITY.format(app_id=app_id)
    if re.search(r'<application[^>]*/>', manifest):
        return re.sub(r'<application([^>]*)/>', lambda m: '<application' + m.group(1) + '>' + activity + '    </application>', manifest, count=1)
    if '</application>' in manifest:
        return manifest.replace('</application>', activity + '    </application>', 1)
    return manifest.replace('</manifest>', '    <application>' + activity + '    </application>\n</manifest>', 1)


def patch_aar(data, keep_voice, app_id):
    libraries = {}
    output = io.BytesIO()
    with zipfile.ZipFile(io.BytesIO(data)) as source, zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as target:
        for item in source.infolist():
            content = source.read(item.filename)
            match = re.match(r'^jni/([^/]+)/libdiscord_partner_sdk\.so$', item.filename)
            if match:
                libraries[match.group(1)] = content
            if item.filename == 'AndroidManifest.xml':
                manifest = content.decode('utf-8')
                if not keep_voice:
                    manifest = strip_voice(manifest)
                manifest = add_auth_activity(manifest, app_id)
                content = manifest.encode('utf-8')
            target.writestr(item, content)
    return output.getvalue(), libraries


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('sdk', help='DiscordSocialSdk zip')
    parser.add_argument('output', help='folder to pass as DISCORD_SOCIAL_SDK')
    parser.add_argument('--app-id', required=True, help='Discord application id used for the discord-APP_ID redirect scheme')
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--keep-voice', action='store_true')
    args = parser.parse_args()

    if not args.app_id.isdigit():
        sys.exit('--app-id must be the numeric Discord application id')

    flavor = 'debug' if args.debug else 'release'

    with zipfile.ZipFile(args.sdk) as archive:
        names = archive.namelist()
        aar_name = find(names, 'lib/' + flavor + '/discord_partner_sdk.aar')
        header = find(names, 'include/discordpp.h')
        c_header = find(names, 'include/cdiscord.h')
        if aar_name is None or header is None or c_header is None:
            sys.exit('The zip does not look like the Discord Social SDK')

        if os.path.exists(args.output):
            shutil.rmtree(args.output)
        os.makedirs(os.path.join(args.output, 'include'))

        for name in (header, c_header):
            with open(os.path.join(args.output, 'include', os.path.basename(name)), 'wb') as handle:
                handle.write(archive.read(name))

        aar, libraries = patch_aar(archive.read(aar_name), args.keep_voice, args.app_id)

    module = os.path.join(args.output, 'gradle')
    os.makedirs(module)
    with open(os.path.join(module, 'discord_partner_sdk.aar'), 'wb') as handle:
        handle.write(aar)

    for abi, content in libraries.items():
        folder = os.path.join(args.output, 'jni', abi)
        os.makedirs(folder, exist_ok=True)
        with open(os.path.join(folder, 'libdiscord_partner_sdk.so'), 'wb') as handle:
            handle.write(content)

    with open(os.path.join(module, 'build.gradle'), 'w') as handle:
        handle.write(BUILD_GRADLE)

    print('Discord Social SDK ready in ' + args.output + ' for ' + ', '.join(sorted(libraries)))


if __name__ == '__main__':
    main()
