#!/bin/bash
#
#	Run one of the in-binary self checks on Linux, with g++, without a Windows toolchain.
#
#	The checks in Code/Commando/*selfcheck.cpp are written to need no graphics device, no
#	level and no physics scene -- that is what makes them the acceptance evidence for the
#	world systems.  They are still compiled into renegade.exe, so a session that cannot build
#	the client cannot normally run them at all.  This builds and runs one of them directly:
#	the check's own translation unit, the sources it actually exercises, four real support
#	libraries, and an automatically generated empty-stub object for every other engine symbol
#	the linker asks for.
#
#	What this is good for: arithmetic, policy, lifecycle and bookkeeping -- the parts of a
#	world system that are decisions rather than pixels.
#
#	What it is not: a build.  It is g++ and not MSVC, so it catches different things; every
#	symbol outside the listed sources is an empty stub, so anything the check calls that is
#	not listed will do nothing or crash rather than being checked; and nothing here draws.
#	A green run means the policy is right, not that the game works.
#
#	Usage:
#		tools/linuxcheck/selfcheck.sh particles      # roadmap Section 26, the particle pool
#		tools/linuxcheck/selfcheck.sh programs       # roadmap Section 15, the shader manager
#
#	Adding a check: give it a case below naming its self-check TU, its entry point and the
#	sources whose real behaviour it depends on.  If a check starts crashing inside a stub,
#	that is the list being short, not the engine being wrong.
#
#	Dependencies, installed once into the cache directory: libicu-dev (apt, for
#	wwlib/icu/unichar.h) and DXVK's native d3d8/d3d9 headers (downloaded, for the ww3d2
#	headers).  A translation unit that includes wwlib/thread.h additionally needs the SDL3
#	headers, which this does not fetch.
#

set -u

WHICH="${1:-particles}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CACHE="${LINUXCHECK_CACHE:-${TMPDIR:-/tmp}/openw3d-linuxcheck}"
WORK="$CACHE/$WHICH"

mkdir -p "$CACHE" "$WORK" || exit 1

#
#	Which check, which entry point, and which sources it really needs.
#
case "$WHICH" in
particles)
	CHECK_TU="Code/Commando/terrainselfcheck.cpp"
	CHECK_HEADER="terrainselfcheck.h"
	CHECK_CALL="TerrainSelfCheck::Run"
	SOURCES="Code/wwphys/particlebatchtype.cpp
		Code/wwphys/worldparticlebatchmanager.cpp
		Code/ww3d2/pointgr.cpp
		Code/ww3d2/linegrp.cpp"
	;;
programs)
	CHECK_TU="Code/Commando/shaderselfcheck.cpp"
	CHECK_HEADER="shaderselfcheck.h"
	CHECK_CALL="ShaderSelfCheck::Run"
	SOURCES="Code/ww3d2/shadermgr.cpp
		Code/ww3d2/vertmaterial.cpp"
	;;
*)
	echo "selfcheck.sh: no recipe for '$WHICH'.  See the comment at the top of this script." >&2
	exit 2
	;;
esac

#	Real support libraries, for every recipe.  StringClass, reference counting, the maths and
#	the assertion handler are used by the checks themselves and must not be stubs.
SOURCES="$SOURCES
	Code/wwlib/wwstring.cpp
	Code/wwlib/refcount.cpp
	Code/WWMath/wwmath.cpp
	Code/wwdebug/wwdebug.cpp"

#
#	Dependencies.
#
if [ ! -e /usr/include/unicode/uchar.h ]; then
	echo "selfcheck.sh: installing libicu-dev"
	apt-get install -y -qq libicu-dev > /dev/null || {
		echo "selfcheck.sh: could not install libicu-dev" >&2; exit 1; }
fi

DXVK_INC="$CACHE/dxvk/usr/include/dxvk"
if [ ! -e "$DXVK_INC/d3d9.h" ]; then
	echo "selfcheck.sh: fetching the DXVK native headers"
	mkdir -p "$CACHE/dxvk" || exit 1
	wget --no-verbose -O "$CACHE/dxvk.tar.gz" \
		"https://github.com/doitsujin/dxvk/releases/download/v2.7.1/dxvk-native-2.7.1-steamrt-sniper.tar.gz" \
		|| { echo "selfcheck.sh: could not fetch the DXVK headers" >&2; exit 1; }
	tar -xzf "$CACHE/dxvk.tar.gz" -C "$CACHE/dxvk" || exit 1
fi

#
#	The compile line.  The defines are the ones Code/CMakeLists.txt puts on every target,
#	plus the two the non-Windows branch of the root CMakeLists adds.
#
INCLUDES=""
for d in BinkMovie Combat Commando SControl ww3d2 WWAudio wwbitpack wwdebug wwlib WWMath \
	wwnet WWOnline wwphys wwsaveload wwtranslatedb wwui wwutil wolapi dxvk_wrapper wwlib/icu . ; do
	INCLUDES="$INCLUDES -I$ROOT/Code/$d"
done
INCLUDES="$INCLUDES -I$DXVK_INC -I$WORK"

cat > "$WORK/winshim.h" <<'SHIM'
#pragma once
/*	The three Win32 entry points the support libraries call in their failure paths.  */
#include <stdlib.h>
#define DebugBreak() ::abort()
#define ExitProcess(code) ::exit((int)(code))
#define Is_Trying_To_Exit() (0)
SHIM

compile ()
{
	g++ -std=c++20 -w -g -O0 -include "$WORK/winshim.h" \
		-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601 -DDIRECTINPUT_VERSION=0x800 \
		-D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS \
		-DWWDEBUG -DOPENW3D_SDL3=1 -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp \
		$INCLUDES "$@"
}

#
#	The entry point.
#
cat > "$WORK/main.cpp" <<MAIN
#include "$CHECK_HEADER"
#include <stdio.h>
int main (int argc, char ** argv)
{
	const char * which = (argc > 1) ? argv[1] : "$WHICH";
	int rc = $CHECK_CALL (which);
	::printf ("selfcheck.sh: %s(\"%s\") returned %d\n","$CHECK_CALL",which,rc);
	return rc;
}
MAIN

OBJS=""
for src in $CHECK_TU $SOURCES; do
	obj="$WORK/$(basename "$src" .cpp).o"
	compile -c -o "$obj" "$ROOT/$src" || { echo "selfcheck.sh: $src would not compile" >&2; exit 1; }
	OBJS="$OBJS $obj"
done
compile -c -o "$WORK/main.o" "$WORK/main.cpp" || exit 1
OBJS="$OBJS $WORK/main.o"

#
#	Everything else the linker wants becomes an empty stub.  A missing symbol whose demangled
#	name has no parentheses is data and is stubbed as zero-filled bytes -- a stubbed data
#	symbol read as a pointer has to be null, or the code under test follows it.
#
: > "$WORK/stubs.list"
for round in 1 2 3 4 5; do
	{ echo '/*	generated link stubs -- regenerated on every run	*/'
	  echo 'extern "C" {'
	  cat "$WORK/stubs.list"
	  echo '}'; } > "$WORK/stubs.cpp"
	compile -c -o "$WORK/stubs.o" "$WORK/stubs.cpp" || exit 1

	if g++ -g -o "$WORK/check" $OBJS "$WORK/stubs.o" -Wl,--no-demangle 2> "$WORK/link.err" ; then
		break
	fi
	if [ "$round" = "5" ]; then
		echo "selfcheck.sh: still would not link:" >&2
		tail -20 "$WORK/link.err" >&2
		exit 1
	fi

	python3 - "$WORK/link.err" "$WORK/stubs.list" <<'STUBS'
import re, subprocess, sys
err = open(sys.argv[1]).read()
listfile = sys.argv[2]
have = set(re.findall(r'^(?:void|char) (_[A-Za-z0-9_]+)', open(listfile).read(), re.M))
missing = sorted(set(re.findall(r"undefined reference to [`']([^'\"]+)'", err)) - have)
with open(listfile, 'a') as f:
    for symbol in missing:
        demangled = subprocess.run(['c++filt', symbol], capture_output=True, text=True).stdout
        if '(' in demangled:
            f.write('void %s(void);\nvoid %s(void) { }\n' % (symbol, symbol))
        else:
            f.write('char %s[8192];\n' % symbol)
print('selfcheck.sh: stubbed %d more symbol(s)' % len(missing))
STUBS
done

echo "selfcheck.sh: running $WHICH"
"$WORK/check" "$WHICH"
rc=$?
echo "selfcheck.sh: exit $rc"
exit $rc
