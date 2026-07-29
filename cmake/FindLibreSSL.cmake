# SPDX-FileCopyrightText: Copyright (c) 2019 John Norrbin <jlnorrbin@johnex.se>
# SPDX-License-Identifier: MIT

#[=======================================================================[

Copyright (c) 2019 John Norrbin <jlnorrbin@johnex.se>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

FindLibreSSL
------------

Find the LibreSSL encryption library.

Optional Components
^^^^^^^^^^^^^^^^^^^

This module supports two optional components: SSL and TLS.  Both
components have associated imported targets, as described below.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following imported targets:

LibreSSL::Crypto
    The LibreSSL crypto library, if found.

LibreSSL::SSL
    The LibreSSL ssl library, if found. Requires and includes LibreSSL::Crypto automatically.

LibreSSL::TLS
    The LibreSSL tls library, if found. Requires and includes LibreSSL::SSL and LibreSSL::Crypto automatically.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

LIBRESSL_FOUND
    System has the LibreSSL library. If no components are requested it only requires the crypto library.
LIBRESSL_INCLUDE_DIR
    The LibreSSL include directory.
LIBRESSL_CRYPTO_LIBRARY
    The LibreSSL crypto library.
LIBRESSL_SSL_LIBRARY
    The LibreSSL SSL library.
LIBRESSL_TLS_LIBRARY
    The LibreSSL TLS library.
LIBRESSL_LIBRARIES
    All LibreSSL libraries.
LIBRESSL_VERSION
    This is set to $major.$minor.$revision (e.g. 2.6.8).

Hints
^^^^^

Set LIBRESSL_ROOT_DIR to the root directory of an LibreSSL installation.

]=======================================================================]

INCLUDE(FindPackageHandleStandardArgs)

# Set Hints
set(_LIBRESSL_ROOT_HINTS
    ${LIBRESSL_ROOT_DIR}
    ENV LIBRESSL_ROOT_DIR
)

# Set Paths
if (WIN32)
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
    set(_LIBRESSL_ROOT_PATHS
        "${_programfiles}/LibreSSL"
    )
    unset(_programfiles)
elseif(APPLE)
    # Homebrew installs LibreSSL here
    set(_LIBRESSL_ROOT_PATHS
        "/usr/local/opt/libressl"
    )
else()
    set(_LIBRESSL_ROOT_PATHS
        "/usr/local/"
    )
endif()

# Combine
set(_LIBRESSL_ROOT_HINTS_AND_PATHS
    HINTS ${_LIBRESSL_ROOT_HINTS}
    PATHS ${_LIBRESSL_ROOT_PATHS}
)

# Find Include Path
find_path(LIBRESSL_INCLUDE_DIR
    NAMES
        tls.h
    ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
    PATH_SUFFIXES
        include
)

# Find Crypto Library
find_library(LIBRESSL_CRYPTO_LIBRARY
    NAMES
        libcrypto
        crypto
        NAMES_PER_DIR
    ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
    PATH_SUFFIXES
        lib
)

# Find SSL Library
find_library(LIBRESSL_SSL_LIBRARY
    NAMES
        libssl
        ssl
        NAMES_PER_DIR
    ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
    PATH_SUFFIXES
        lib
)

# Find TLS Library
find_library(LIBRESSL_TLS_LIBRARY
    NAMES
        libtls
        tls
        NAMES_PER_DIR
    ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
    PATH_SUFFIXES
        lib
)

# Set Libraries
set(LIBRESSL_LIBRARIES ${LIBRESSL_CRYPTO_LIBRARY} ${LIBRESSL_SSL_LIBRARY} ${LIBRESSL_TLS_LIBRARY})

# Mark Variables As Advanced
mark_as_advanced(LIBRESSL_INCLUDE_DIR LIBRESSL_LIBRARIES LIBRESSL_CRYPTO_LIBRARY LIBRESSL_SSL_LIBRARY LIBRESSL_TLS_LIBRARY)

# Find Version File
if(LIBRESSL_INCLUDE_DIR AND EXISTS "${LIBRESSL_INCLUDE_DIR}/openssl/opensslv.h")

    # Get Version From File
    file(STRINGS "${LIBRESSL_INCLUDE_DIR}/openssl/opensslv.h" OPENSSLV.H REGEX "#define LIBRESSL_VERSION_TEXT[ ]+\".*\"")

    # Match Version String
    string(REGEX REPLACE ".*\".*([0-9]+)\\.([0-9]+)\\.([0-9]+)\"" "\\1;\\2;\\3" LIBRESSL_VERSION_LIST "${OPENSSLV.H}")

    # Split Parts
    list(GET LIBRESSL_VERSION_LIST 0 LIBRESSL_VERSION_MAJOR)
    list(GET LIBRESSL_VERSION_LIST 1 LIBRESSL_VERSION_MINOR)
    list(GET LIBRESSL_VERSION_LIST 2 LIBRESSL_VERSION_REVISION)

    # Set Version String
    set(LIBRESSL_VERSION "${LIBRESSL_VERSION_MAJOR}.${LIBRESSL_VERSION_MINOR}.${LIBRESSL_VERSION_REVISION}")

endif()

# Set Find Package Arguments
find_package_handle_standard_args(LibreSSL
    REQUIRED_VARS
        LIBRESSL_CRYPTO_LIBRARY
        LIBRESSL_INCLUDE_DIR
    VERSION_VAR
        LIBRESSL_VERSION
    HANDLE_COMPONENTS
        FAIL_MESSAGE
        "Could NOT find LibreSSL, try setting the path to LibreSSL using the LIBRESSL_ROOT_DIR environment variable"
)

# LibreSSL Found
if(LIBRESSL_FOUND)

    # Set LibreSSL::Crypto
    if(NOT TARGET LibreSSL::Crypto AND EXISTS "${LIBRESSL_CRYPTO_LIBRARY}")

        # Add Library
        add_library(LibreSSL::Crypto UNKNOWN IMPORTED)

        # Set Properties
        set_target_properties(
            LibreSSL::Crypto
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LIBRESSL_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBRESSL_CRYPTO_LIBRARY}"
        )

    endif() # LibreSSL::Crypto

    # Set LibreSSL::SSL
    if(NOT TARGET LibreSSL::SSL AND EXISTS "${LIBRESSL_SSL_LIBRARY}")

        # Add Library
        add_library(LibreSSL::SSL UNKNOWN IMPORTED)

        # Set Properties
        set_target_properties(
            LibreSSL::SSL
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LIBRESSL_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBRESSL_SSL_LIBRARY}"
                INTERFACE_LINK_LIBRARIES LibreSSL::Crypto
        )

    endif() # LibreSSL::SSL

    # Set LibreSSL::TLS
    if(NOT TARGET LibreSSL::TLS AND EXISTS "${LIBRESSL_TLS_LIBRARY}")
        add_library(LibreSSL::TLS UNKNOWN IMPORTED)
        set_target_properties(
            LibreSSL::TLS
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LIBRESSL_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBRESSL_TLS_LIBRARY}"
                INTERFACE_LINK_LIBRARIES LibreSSL::SSL
        )

    endif() # LibreSSL::TLS

endif(LIBRESSL_FOUND)
