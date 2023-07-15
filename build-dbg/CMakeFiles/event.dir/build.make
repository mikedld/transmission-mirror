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

# Utility rule file for event.

# Include any custom commands dependencies for this target.
include CMakeFiles/event.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/event.dir/progress.make

CMakeFiles/event: CMakeFiles/event-complete

CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-install
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-mkdir
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-download
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-update
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-patch
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-configure
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-build
CMakeFiles/event-complete: third-party/libevent.bld/src/event-stamp/event-install
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Completed 'event'"
	/X/cmake/bin/cmake.exe -E make_directory X:/transmission-jr/build-dbg/CMakeFiles
	/X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/CMakeFiles/event-complete
	/X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-done

third-party/libevent.bld/src/event-stamp/event-build: third-party/libevent.bld/src/event-stamp/event-configure
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Performing build step for 'event'"
	cd /X/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build && $(MAKE)
	cd /X/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build && /X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-build

third-party/libevent.bld/src/event-stamp/event-configure: third-party/libevent.bld/tmp/event-cfgcmd.txt
third-party/libevent.bld/src/event-stamp/event-configure: third-party/libevent.bld/src/event-stamp/event-patch
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Performing configure step for 'event'"
	cd /X/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build && /X/cmake/bin/cmake.exe -Wno-dev --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:PATH= -DCMAKE_USER_MAKE_RULES_OVERRIDE= -DCMAKE_C_COMPILER=X:/msys64/mingw64/bin/cc.exe "-DCMAKE_C_FLAGS:STRING= -DWIN32 -DWINVER=0x0600 -D_WIN32_WINNT=0x0600 -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS -D__USE_MINGW_ANSI_STDIO=1" -DCMAKE_CXX_COMPILER=X:/msys64/mingw64/bin/c++.exe "-DCMAKE_CXX_FLAGS:STRING= -DWIN32 -DWINVER=0x0600 -D_WIN32_WINNT=0x0600 -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS -D__USE_MINGW_ANSI_STDIO=1" -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=X:/transmission-jr/build-dbg/third-party/libevent.bld/pfx -DCMAKE_INSTALL_LIBDIR:STRING=lib -DEVENT__DISABLE_OPENSSL:BOOL=ON -DEVENT__DISABLE_BENCHMARK:BOOL=ON -DEVENT__DISABLE_TESTS:BOOL=ON -DEVENT__DISABLE_REGRESS:BOOL=ON -DEVENT__DISABLE_SAMPLES:BOOL=ON -DEVENT__LIBRARY_TYPE:STRING=STATIC "-GMSYS Makefiles" -S X:/transmission-jr/third-party/libevent -B X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build
	cd /X/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build && /X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-configure

third-party/libevent.bld/src/event-stamp/event-download: third-party/libevent.bld/src/event-stamp/event-source_dirinfo.txt
third-party/libevent.bld/src/event-stamp/event-download: third-party/libevent.bld/src/event-stamp/event-mkdir
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "No download step for 'event'"
	/X/cmake/bin/cmake.exe -E echo_append
	/X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-download

third-party/libevent.bld/src/event-stamp/event-install: third-party/libevent.bld/src/event-stamp/event-build
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Performing install step for 'event'"
	cd /X/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build && $(MAKE) install
	cd /X/transmission-jr/build-dbg/third-party/libevent.bld/src/event-build && /X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-install

third-party/libevent.bld/src/event-stamp/event-mkdir:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Creating directories for 'event'"
	/X/cmake/bin/cmake.exe -Dcfgdir= -P X:/transmission-jr/build-dbg/third-party/libevent.bld/tmp/event-mkdirs.cmake
	/X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-mkdir

third-party/libevent.bld/src/event-stamp/event-patch: third-party/libevent.bld/src/event-stamp/event-patch-info.txt
third-party/libevent.bld/src/event-stamp/event-patch: third-party/libevent.bld/src/event-stamp/event-update
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "No patch step for 'event'"
	/X/cmake/bin/cmake.exe -E echo_append
	/X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-patch

third-party/libevent.bld/src/event-stamp/event-update: third-party/libevent.bld/src/event-stamp/event-update-info.txt
third-party/libevent.bld/src/event-stamp/event-update: third-party/libevent.bld/src/event-stamp/event-download
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/X/transmission-jr/build-dbg/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "No update step for 'event'"
	/X/cmake/bin/cmake.exe -E echo_append
	/X/cmake/bin/cmake.exe -E touch X:/transmission-jr/build-dbg/third-party/libevent.bld/src/event-stamp/event-update

event: CMakeFiles/event
event: CMakeFiles/event-complete
event: third-party/libevent.bld/src/event-stamp/event-build
event: third-party/libevent.bld/src/event-stamp/event-configure
event: third-party/libevent.bld/src/event-stamp/event-download
event: third-party/libevent.bld/src/event-stamp/event-install
event: third-party/libevent.bld/src/event-stamp/event-mkdir
event: third-party/libevent.bld/src/event-stamp/event-patch
event: third-party/libevent.bld/src/event-stamp/event-update
event: CMakeFiles/event.dir/build.make
.PHONY : event

# Rule to build all files generated by this target.
CMakeFiles/event.dir/build: event
.PHONY : CMakeFiles/event.dir/build

CMakeFiles/event.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/event.dir/cmake_clean.cmake
.PHONY : CMakeFiles/event.dir/clean

CMakeFiles/event.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MSYS Makefiles" /X/transmission-jr /X/transmission-jr /X/transmission-jr/build-dbg /X/transmission-jr/build-dbg /X/transmission-jr/build-dbg/CMakeFiles/event.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/event.dir/depend

