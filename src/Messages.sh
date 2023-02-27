#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: CC0-1.0
$XGETTEXT `find -name \*.cpp -o -name \*.qml -o -name \*.js` -o $podir/kasts.pot
