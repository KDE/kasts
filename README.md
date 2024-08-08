# Kasts

Kasts is a convergent podcast application.

<a href='https://flathub.org/apps/details/org.kde.kasts'><img width='190px' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-i-en.png'/></a>

![Timeline](https://cdn.kde.org/screenshots/kasts/kasts-desktop.png)

# Get it

The nightly Android version is available in [KDE's nightly F-Droid repository](https://community.kde.org/Android/F-Droid).

Nightly Windows builds can be found [on KDE's ci-build server](https://cdn.kde.org/ci-builds/multimedia/kasts/master/windows/). Stable Windows versions can also be found [on the same server](https://cdn.kde.org/ci-builds/multimedia/kasts/).

# Bug reports

Please don't use GitLab issues for reporting bugs, instead report them [here](https://bugs.kde.org/enter_bug.cgi?format=guided&product=kasts).

# Building

Note: When using versions of Kasts built from git-master, it's possible that the database format or the name of downloaded files change from one version to another without the necessary migrations to handle it. If you notice bugs after upgrading to a git-master version, export your feeds, delete `~/.local/share/KDE/kasts` and import the feeds again.

## Requirements
 - KCoreAddons
 - KI18n
 - KConfig
 - Kirigami
 - Kirigami-addons
 - Syndication
 - TagLib
 - QtKeychain
 - ThreadWeaver
 - KColorScheme
 - KCrash
 - libVLC (optional, recommended)
 - GStreamer (optional)

## Linux

```
git clone https://invent.kde.org/multimedia/kasts
cd kasts
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr
make
sudo make install
```

This assumes all dependencies are installed. If your distribution does not provide
them, you can use [kdesrc-build](https://kdesrc-build.kde.org/) to build all of them.

## Android

You can build Kasts for Android using KDE's [Docker-based build environment](https://community.kde.org/Android/Environment_via_Container).

