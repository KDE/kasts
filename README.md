# Alligator

Alligator is a convergent RSS/Atom feed reader.

# Get it

Nightly Android APKs are available at [KDE's Binary Factory](https://binary-factory.kde.org/view/Android/job/Alligator_android/).

# Building

## Requirements
 - KCoreAddons
 - KI18n
 - KConfig
 - Kirigami
 - Syndication

## Linux

```
git clone https://invent.kde.org/kde/alligator
cd alligator
mkdir build && cd  build
cmake .. -DCMAKE_PREFIX_PATH=/usr
make
sudo make install
```

This assumes all dependencies are installed. If your distribution does not provide
them, you can use [kdesrc-build](https://kdesrc-build.kde.org/) to build all of them.

## Android 

You can build Alligator for Android using KDE's [Docker-based build environment](https://community.kde.org/Android/Environment_via_Container).

