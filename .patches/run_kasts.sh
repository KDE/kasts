#!/bin/sh
# SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
# SPDX-License-Identifier: BSD-2-Clause

export QML_DISABLE_DISK_CACHE=1
exec kasts-bin "$@"
