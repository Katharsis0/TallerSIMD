# CE-4302 Arquitectura de Computadores II
# Taller 02 - SIMD Extensions and Intrinsics
# Makefile for Serial Implementation

CXX = g++
CXXFLAGS = -std=c++14 -O3 -Wall -Wextra -march=native
TARGET_SERIAL = char_count_serial

# Source files
COMMON_SRC = utils.cpp
SERIAL_SRC = char_count_serial.cpp

# Header files
HEADERS = utils.h

# Default target
all: $(TARGET_SERIAL)

# Serial implementation
$(TARGET_SERIAL): $(SERIAL_SRC) $(COMMON_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SERIAL_SRC) $(COMMON_SRC)

# Debug version
debug: CXXFLAGS = -std=c++11 -g -Wall -Wextra -DDEBUG
debug: $(TARGET_SERIAL)

# Performance optimized version
performance: CXXFLAGS = -std=c++11 -O3 -Wall -Wextra -march=native -mtune=native -DNDEBUG
performance: $(TARGET_SERIAL)

# Quick test with predefined parameters
test: $(TARGET_SERIAL)
	@echo "Running quick test with 1MB string, 16-byte alignment"
	@echo -e "1048576\n16\n5\ny\nn" | ./$(TARGET_SERIAL), target ';'"
	@echo "1048576" | ./$(TARGET_SERIAL)
	@echo "16"
	@echo ";"
	@echo "1"
	@echo "n"
	@echo "n"

# Validation test - runs multiple configurations
validate: $(TARGET_SERIAL)
	@echo "=== Validation Tests ==="
	@echo "Testing different alignments with same string size..."
	@for align in 1 2 4 8 16 32; do \
		echo "Testing alignment: $$align bytes"; \
		echo -e "4096\n$$align\n;\n5\ny" | ./$(TARGET_SERIAL) > test_align_$$align.log 2>&1; \
	done
	@echo "Validation complete. Check test_align_*.log files for results."

# Performance analysis script
analyze: $(TARGET_SERIAL)
	@echo "=== Automated Performance Analysis ==="
	@echo "Running cache behavior analysis..."
	@echo -e "1048576\n16\n;\n10\ny\ny" | ./$(TARGET_SERIAL) > performance_analysis.log 2>&1
	@echo "Analysis complete. Check performance_analysis.log for results."

# Memory alignment verification
verify_alignment: $(TARGET_SERIAL)
	@echo "=== Memory Alignment Verification ==="
	@echo "Testing various alignment values..."
	@for align in 1 2 4 8 16 32 64; do \
		echo "Alignment: $$align bytes"; \
		echo -e "1024\n$$align\n;\n1\nn" | ./$(TARGET_SERIAL) | grep -E "(Alignment Check|Address modulo)"; \
	done

# Clean targets
clean:
	rm -f $(TARGET_SERIAL) *.o

distclean: clean
	rm -f *.log *.csv

# System information for platform-specific optimization
sysinfo:
	@echo "=== System Information ==="
	@echo "CPU Info:"
	@grep -E "(model name|cpu MHz|cache size)" /proc/cpuinfo | head -10
	@echo ""
	@echo "Memory Info:"
	@grep -E "(MemTotal|MemFree)" /proc/meminfo
	@echo ""
	@echo "Compiler Version:"
	@$(CXX) --version | head -1
	@echo ""
	@echo "Compiler Optimization Flags:"
	@$(CXX) -march=native -Q --help=target | grep -E "(march|mtune)" | head -5

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build the serial implementation"
	@echo "  debug         - Build debug version with symbols"
	@echo "  performance   - Build highly optimized version"
	@echo "  test          - Run quick test with predefined parameters"
	@echo "  validate      - Run validation tests with different alignments"
	@echo "  analyze       - Run automated performance analysis"
	@echo "  verify_alignment - Verify memory alignment for different values"
	@echo "  sysinfo       - Display system information for optimization"
	@echo "  clean         - Remove built executables"
	@echo "  distclean     - Remove all generated files"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make performance  # Build optimized version"
	@echo "  make validate     # Test different alignments"
	@echo "  make analyze      # Run performance analysis"

.PHONY: all debug performance test validate analyze verify_alignment clean distclean sysinfo help
