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


def patch_aar(data, keep_voice):
    libraries = {}
    output = io.BytesIO()
    with zipfile.ZipFile(io.BytesIO(data)) as source, zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as target:
        for item in source.infolist():
            content = source.read(item.filename)
            match = re.match(r'^jni/([^/]+)/libdiscord_partner_sdk\.so$', item.filename)
            if match:
                libraries[match.group(1)] = content
            if item.filename == 'AndroidManifest.xml' and not keep_voice:
                content = strip_voice(content.decode('utf-8')).encode('utf-8')
            target.writestr(item, content)
    return output.getvalue(), libraries


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('sdk', help='DiscordSocialSdk zip')
    parser.add_argument('output', help='folder to pass as DISCORD_SOCIAL_SDK')
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--keep-voice', action='store_true')
    args = parser.parse_args()

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

        aar, libraries = patch_aar(archive.read(aar_name), args.keep_voice)

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
