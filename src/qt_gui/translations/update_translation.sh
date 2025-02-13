#!/bin/bash

SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")

OPTS="-tr-function-alias QT_TRANSLATE_NOOP+=TRANSLATE,QT_TRANSLATE_NOOP+=TRANSLATE_SV,QT_TRANSLATE_NOOP+=TRANSLATE_STR,QT_TRANSLATE_NOOP+=TRANSLATE_FS,QT_TRANSLATE_N_NOOP3+=TRANSLATE_FMT,QT_TRANSLATE_NOOP+=TRANSLATE_NOOP,translate+=TRANSLATE_PLURAL_STR,translate+=TRANSLATE_PLURAL_FS -no-obsolete"
SRCDIRS=$(realpath "$SCRIPTDIR/..")/\ $(realpath "$SCRIPTDIR/../..")/
OUTDIR=$(realpath "$SCRIPTDIR")

lupdate $SRCDIRS $OPTS -locations none -source-language en_US  -ts "$OUTDIR/en_US.ts"

if ! head -n 2 "$OUTDIR/en_US.ts" | grep -q "SPDX-FileCopyrightText"; then
    sed -i '2i\<!-- SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project\n     SPDX-License-Identifier: GPL-2.0-or-later -->' "$OUTDIR/en_US.ts"
fi
