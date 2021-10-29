# Kasts

Kasts is a convergent podcast application.

<a href='https://flathub.org/apps/details/org.kde.kasts'><img width='190px' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-i-en.png'/></a>

# Get it

Nightly Android APKs are available at [KDE's Binary Factory](https://binary-factory.kde.org/view/Android/job/Kasts_Nightly_android-arm64/).

# Building

Note: When using versions of kasts built from git-master, it's possible that the database format or the name of downloaded files change from one version to another without the necessary migrations to handle it. If you notice bugs after upgrading to a git-master version, export your feeds, delete `~/.local/share/KDE/kasts` and import the feeds again.

## Requirements
 - KCoreAddons
 - KI18n
 - KConfig
 - Kirigami
 - Syndication
 - TagLib
 - QtKeychain

## Linux

```
git clone https://invent.kde.org/plasma-mobile/kasts
cd kasts
mkdir build && cd  build
cmake .. -DCMAKE_PREFIX_PATH=/usr
make
sudo make install
```

This assumes all dependencies are installed. If your distribution does not provide
them, you can use [kdesrc-build](https://kdesrc-build.kde.org/) to build all of them.

## Android

You can build Kasts for Android using KDE's [Docker-based build environment](https://community.kde.org/Android/Environment_via_Container).

