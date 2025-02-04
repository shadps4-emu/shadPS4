#!/bin/bash

SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")

set -e

sudo apt-get -y install qt6-l10n-tools python3

PATH=/usr/lib/qt6/bin:$PATH "$SCRIPTDIR/../../../src/qt_gui/translations/update_translation.sh"
