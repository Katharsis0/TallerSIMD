# CE-4302 Arquitectura de Computadores II
# Sebastián Hidalgo Vargas
# Makefile Serial Implementation
CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall -Wextra -march=native
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


# Debug versions
debug: CXXFLAGS = -std=c++11 -g -Wall -Wextra -DDEBUG
debug: $(TARGET_SERIAL)

# Performance optimized version
performance: CXXFLAGS = -std=c++11 -O3 -Wall -Wextra -march=native -mtune=native -DNDEBUG
performance: $(TARGET_SERIAL)

# Clean targets
clean:
	rm -f $(TARGET_SERIAL) $(TARGET_GENERATOR) *.o

distclean: clean
	rm -f *.csv *.log

# Performance analysis
analyze: $(TARGET_SERIAL)
	@echo "Running performance analysis..."
	@./$(TARGET_SERIAL) << EOF

# Help target
help:
	@echo "Available targets:"
	@echo "  char_count_serial - Build only the serial implementation"
	@echo "  debug        - Build debug version with symbols"
	@echo "  performance  - Build highly optimized version"
	@echo "  test_serial  - Run validation tests"
	@echo "  test_interactive - Run interactive test with sample input"
	@echo "  analyze      - Run performance analysis"
	@echo "  clean        - Remove built executables"
	@echo "  distclean    - Remove all generated files"
	@echo "  help         - Show this help message"

.PHONY: all debug performance test_serial test_batch test_interactive clean distclean analyze help
