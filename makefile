# This Makefile will build the MinGW Win32 application.

# Directory for build
OUT_DIR = Release

# Object files to create for the executable
OBJS = ${OUT_DIR}/main.o ${OUT_DIR}/res.o

# Warnings to be raised by the C compiler
WARNS = -pedantic -Wall

# Names of tools to use when building
CC = gcc
RC = windres
EXE = putty_serial_launcher.exe

# Compiler flags
CFLAGS = -Os -std=c99 -D _WIN32_IE=0x0500 -D WINVER=0x0500 ${WARNS} -Ires

# If need Unicode support, add the CHARSET=UNICODE parameter to make
ifeq (${CHARSET}, UNICODE)
CFLAGS += -D UNICODE -D _UNICODE
endif

# Linker flags
LDFLAGS = -s -static -lcomctl32 -Wl,--subsystem,windows

.PHONY: all clean

# Build executable by default
all: ${OUT_DIR}/${EXE}

# Delete all build output
clean:
	@rm -f ${OUT_DIR}/*.o
	@rm -f ${OUT_DIR}/*.d
	@rm -f ${OUT_DIR}/${EXE}

# Create build output directories if they don't exist
${OUT_DIR}:
	@mkdir -p "$@"

# Compile object files for executable
${OUT_DIR}/%.o: %.c | ${OUT_DIR}
	${CC} ${CFLAGS} -c "$<" -o "$@"

# Build the resources
${OUT_DIR}/res.o: res/res.rc | ${OUT_DIR}
	${RC} "$<" -o "$@"

# Build the exectuable
${OUT_DIR}/${EXE}: ${OBJS} | ${OUT_DIR}
	${CC} -o "$@" ${OBJS} ${LDFLAGS}

# C header dependencies
${OUT_DIR}/main.o: res/res.h
