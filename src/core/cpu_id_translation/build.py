#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

"""Build the experimental Linux x86-64 CPU identity translation runtime and emulator."""

import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

REVISION = "a522a505582076eb7f68363b5d301ddca44399e2"
REPOSITORY = "https://github.com/DynamoRIO/dynamorio.git"
ROOT = Path(__file__).resolve().parents[3]
PATCH = Path(__file__).with_name("dynamorio.patch")


def run(*args, cwd=None):
    subprocess.run([str(arg) for arg in args], cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-cpu-id-translation")
    parser.add_argument("--runtime-source", type=Path)
    parser.add_argument("--runtime-build", type=Path)
    parser.add_argument("--runtime-only", action="store_true")
    parser.add_argument("--cmake", default=shutil.which("cmake"))
    parser.add_argument("--ninja", default=shutil.which("ninja"))
    parser.add_argument("--cc", help="Emulator C compiler")
    parser.add_argument("--cxx", help="Emulator C++23 compiler")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1))
    args = parser.parse_args()
    if platform.system() != "Linux" or platform.machine() not in {"x86_64", "AMD64"}:
        parser.error("This experiment supports Linux x86-64 only")
    if not args.cmake or not args.ninja or args.jobs < 1:
        parser.error("cmake, ninja, and a positive --jobs value are required")
    cache = Path.home() / ".cache/shadps4-cpu-id-translation" / REVISION
    source = (args.runtime_source or cache / "source").resolve()
    runtime = (args.runtime_build or cache / "build").resolve()
    build = args.build_dir.resolve()
    if not (source / ".git").exists():
        if source.exists() and any(source.iterdir()):
            raise RuntimeError("Runtime source is not an empty directory or a git checkout; preserved")
        source.mkdir(parents=True, exist_ok=True)
        run("git", "init", source)
        run("git", "remote", "add", "origin", REPOSITORY, cwd=source)
        run("git", "fetch", "--depth", "1", "--no-recurse-submodules", "origin", REVISION,
            cwd=source)
        run("git", "checkout", "--detach", REVISION, cwd=source)
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    if head != REVISION:
        raise RuntimeError("Runtime source has a different revision; preserved")
    diff = subprocess.check_output(["git", "diff", "--abbrev=7", "HEAD", "--"], cwd=source)
    if not diff:
        run("git", "apply", "--check", PATCH, cwd=source)
        run("git", "apply", PATCH, cwd=source)
    elif diff != PATCH.read_bytes():
        raise RuntimeError("Runtime source contains other changes; preserved")
    run("git", "submodule", "update", "--init", "third_party/elfutils", cwd=source)
    run(args.cmake, "-S", source, "-B", runtime, "-G", "Ninja",
        "-DCMAKE_MAKE_PROGRAM=" + args.ninja, "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
        "-DBUILD_TESTS=OFF", "-DBUILD_SAMPLES=OFF", "-DBUILD_CLIENTS=OFF",
        "-DBUILD_DOCS=OFF", "-DBUILD_EXT=ON", "-DDISABLE_DRGUI=ON",
        "-DDISABLE_WARNINGS=ON", "-Dpreferred_base=0x710000000000",
        "-DPREFERRED_BASE=0x710040000000")
    run(args.cmake, "--build", runtime, "--target", "dynamorio", "drpreload", "drrun", "drmgr", "drwrap",
        "--parallel", args.jobs)
    if args.runtime_only:
        return
    generator = [] if (build / "CMakeCache.txt").exists() else [
        "-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=" + args.ninja]
    compilers = []
    if args.cc:
        compilers.append("-DCMAKE_C_COMPILER=" + args.cc)
    if args.cxx:
        compilers.append("-DCMAKE_CXX_COMPILER=" + args.cxx)
    run(args.cmake, "-S", ROOT, "-B", build, *generator, *compilers, "-DCMAKE_BUILD_TYPE=Release",
        "-DENABLE_CPU_ID_TRANSLATION=ON", "-DDynamoRIO_DIR=" + str(runtime / "cmake"),
        "-DDynamoRIO_SOURCE_DIR=" + str(source))
    run(args.cmake, "--build", build, "--target", "shadps4", "shadps4_cpu_id",
        "--parallel", args.jobs)
    command = [str(runtime / "bin64/drrun"), "-disable_rseq", "-vm_base", "0x710020000000",
               "-no_vm_base_near_app", "-c",
               str(build / "src/core/cpu_id_translation/libshadps4_cpu_id.so"), "--",
               str(build / "shadps4")]
    launcher = build / "run-translated.py"
    launcher.write_text('''#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
import os
import signal
import subprocess
import sys
COMMAND = ''' + repr(command) + '''
process = subprocess.Popen(COMMAND + sys.argv[1:], start_new_session=True)
try:
    raise SystemExit(process.wait())
except KeyboardInterrupt:
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGINT)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    raise SystemExit(130)
''')
    print("LAUNCHER=" + str(launcher))


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print("BUILD_FAILED=" + str(error), file=sys.stderr)
        raise SystemExit(1)
