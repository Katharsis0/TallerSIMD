# CE-4302 Arquitectura de Computadores II
# Taller 02 - SIMD Extensions and Intrinsics
# Makefile for Serial and SIMD Implementations

CXX = g++
CXXFLAGS = -std=c++14 -O3 -Wall -Wextra -march=native -msse4.2
LDFLAGS = 
TARGET_SERIAL = char_count_serial
TARGET_SIMD = char_count_simd

# Source files
COMMON_SRC = utils.cpp
SERIAL_SRC = char_count_serial.cpp
SIMD_SRC = char_count_simd.cpp

# Header files
HEADERS = utils.h

# Default target - builds both implementations
all: $(TARGET_SERIAL) $(TARGET_SIMD)

# Serial implementation
$(TARGET_SERIAL): $(SERIAL_SRC) $(COMMON_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SERIAL_SRC) $(COMMON_SRC) $(LDFLAGS)

# SIMD implementation (requires SSE4.2)
$(TARGET_SIMD): $(SIMD_SRC) $(COMMON_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SIMD_SRC) $(COMMON_SRC) $(LDFLAGS)

# Debug versions
debug: CXXFLAGS = -std=c++14 -g -Wall -Wextra -DDEBUG
debug: $(TARGET_SERIAL) $(TARGET_SIMD)

# Performance optimized versions
performance: CXXFLAGS = -std=c++14 -O3 -Wall -Wextra -march=native -mtune=native -msse4.2 -DNDEBUG
performance: $(TARGET_SERIAL) $(TARGET_SIMD)


# Clean targets
clean:
		rm -rf $(TARGET_SERIAL) $(TARGET_SIMD) *.o *.csv ./comparison_plots/

distclean: clean
	rm -f *.log *.csv



.PHONY: all debug clean distclean
