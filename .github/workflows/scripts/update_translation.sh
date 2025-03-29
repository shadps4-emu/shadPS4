#!/bin/bash

set -e

sudo apt-get -y install qt6-l10n-tools python3

SCRIPT_PATH="src/qt_gui/translations/update_translation.sh"

chmod +x "$SCRIPT_PATH"

PATH=/usr/lib/qt6/bin:$PATH "$SCRIPT_PATH"