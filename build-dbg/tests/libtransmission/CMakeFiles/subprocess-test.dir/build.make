# CMAKE generated file: DO NOT EDIT!
# Generated by "MSYS Makefiles" Generator, CMake Version 3.27

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /X/cmake/bin/cmake.exe

# The command to remove a file.
RM = /X/cmake/bin/cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /X/transmission-jr

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /X/transmission-jr/build-dbg

# Include any dependencies generated for this target.
include tests/libtransmission/CMakeFiles/subprocess-test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tests/libtransmission/CMakeFiles/subprocess-test.dir/compiler_depend.make

# Include the progress variables for this target.
include tests/libtransmission/CMakeFiles/subprocess-test.dir/progress.make

# Include the compile flags for this target's objects.
include tests/libtransmission/CMakeFiles/subprocess-test.dir/flags.make

tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj: tests/libtransmission/CMakeFiles/subprocess-test.dir/flags.make
tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj: tests/libtransmission/CMakeFiles/subprocess-test.dir/includes_CXX.rsp
tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj: X:/transmission-jr/tests/libtransmission/subprocess-test-program.cc
tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj: tests/libtransmission/CMakeFiles/subprocess-test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj"
	cd /X/transmission-jr/build-dbg/tests/libtransmission && /X/msys64/mingw64/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj -MF CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj.d -o CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj -c /X/transmission-jr/tests/libtransmission/subprocess-test-program.cc

tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.i"
	cd /X/transmission-jr/build-dbg/tests/libtransmission && /X/msys64/mingw64/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /X/transmission-jr/tests/libtransmission/subprocess-test-program.cc > CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.i

tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.s"
	cd /X/transmission-jr/build-dbg/tests/libtransmission && /X/msys64/mingw64/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /X/transmission-jr/tests/libtransmission/subprocess-test-program.cc -o CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.s

# Object files for target subprocess-test
subprocess__test_OBJECTS = \
"CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj"

# External object files for target subprocess-test
subprocess__test_EXTERNAL_OBJECTS =

tests/libtransmission/subprocess-test.exe: tests/libtransmission/CMakeFiles/subprocess-test.dir/subprocess-test-program.cc.obj
tests/libtransmission/subprocess-test.exe: tests/libtransmission/CMakeFiles/subprocess-test.dir/build.make
tests/libtransmission/subprocess-test.exe: tests/libtransmission/CMakeFiles/subprocess-test.dir/compiler_depend.ts
tests/libtransmission/subprocess-test.exe: libtransmission/libtransmission.a
tests/libtransmission/subprocess-test.exe: third-party/libdeflate.bld/pfx/lib/libdeflate.a
tests/libtransmission/subprocess-test.exe: X:/msys64/mingw64/lib/libssl.dll.a
tests/libtransmission/subprocess-test.exe: X:/msys64/mingw64/lib/libcrypto.dll.a
tests/libtransmission/subprocess-test.exe: X:/msys64/mingw64/lib/libcurl.dll.a
tests/libtransmission/subprocess-test.exe: X:/msys64/mingw64/lib/libpsl.dll.a
tests/libtransmission/subprocess-test.exe: third-party/libnatpmp.bld/pfx/lib/libnatpmp.a
tests/libtransmission/subprocess-test.exe: third-party/miniupnpc.bld/pfx/lib/libminiupnpc.a
tests/libtransmission/subprocess-test.exe: third-party/dht.bld/pfx/lib/libdht.a
tests/libtransmission/subprocess-test.exe: third-party/libutp.bld/libutp.a
tests/libtransmission/subprocess-test.exe: third-party/libb64.bld/src/libb64.a
tests/libtransmission/subprocess-test.exe: third-party/jsonsl/libjsonsl.a
tests/libtransmission/subprocess-test.exe: third-party/wildmat/libwildmat.a
tests/libtransmission/subprocess-test.exe: third-party/libevent.bld/pfx/lib/libevent.a
tests/libtransmission/subprocess-test.exe: tests/libtransmission/CMakeFiles/subprocess-test.dir/linkLibs.rsp
tests/libtransmission/subprocess-test.exe: tests/libtransmission/CMakeFiles/subprocess-test.dir/objects1.rsp
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable subprocess-test.exe"
	cd /X/transmission-jr/build-dbg/tests/libtransmission && /X/cmake/bin/cmake.exe -E rm -f CMakeFiles/subprocess-test.dir/objects.a
	cd /X/transmission-jr/build-dbg/tests/libtransmission && /X/msys64/mingw64/bin/ar.exe qc CMakeFiles/subprocess-test.dir/objects.a @CMakeFiles/subprocess-test.dir/objects1.rsp
	cd /X/transmission-jr/build-dbg/tests/libtransmission && /X/msys64/mingw64/bin/c++.exe  -DWIN32 -DWINVER=0x0600 -D_WIN32_WINNT=0x0600 -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS -D__USE_MINGW_ANSI_STDIO=1 -DWIN32_LEAN_AND_MEAN -DNOMINMAX -O2 -g -DNDEBUG -Wl,--dependency-file,CMakeFiles/subprocess-test.dir/link.d -Wl,--whole-archive CMakeFiles/subprocess-test.dir/objects.a -Wl,--no-whole-archive -o subprocess-test.exe -Wl,--out-implib,libsubprocess-test.dll.a -Wl,--major-image-version,0,--minor-image-version,0 @CMakeFiles/subprocess-test.dir/linkLibs.rsp

# Rule to build all files generated by this target.
tests/libtransmission/CMakeFiles/subprocess-test.dir/build: tests/libtransmission/subprocess-test.exe
.PHONY : tests/libtransmission/CMakeFiles/subprocess-test.dir/build

tests/libtransmission/CMakeFiles/subprocess-test.dir/clean:
	cd /X/transmission-jr/build-dbg/tests/libtransmission && $(CMAKE_COMMAND) -P CMakeFiles/subprocess-test.dir/cmake_clean.cmake
.PHONY : tests/libtransmission/CMakeFiles/subprocess-test.dir/clean

tests/libtransmission/CMakeFiles/subprocess-test.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MSYS Makefiles" /X/transmission-jr /X/transmission-jr/tests/libtransmission /X/transmission-jr/build-dbg /X/transmission-jr/build-dbg/tests/libtransmission /X/transmission-jr/build-dbg/tests/libtransmission/CMakeFiles/subprocess-test.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : tests/libtransmission/CMakeFiles/subprocess-test.dir/depend

