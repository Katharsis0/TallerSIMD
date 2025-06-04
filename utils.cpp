#include "utils.h"
#include <algorithm>
#include <limits>

// PerformanceMetrics implementation
void PerformanceMetrics::print() const {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== Performance Metrics ===" << std::endl;
    std::cout << "String Length: " << stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << alignment << " bytes" << std::endl;
    std::cout << "Total Characters: " << totalCharacters << std::endl;
    std::cout << "Unique Characters: " << uniqueCharacters << std::endl;
    std::cout << "Execution Time: " << executionTimeMs << " ms" << std::endl;
    std::cout << "Memory Used: " << memoryUsedBytes << " bytes" << std::endl;
    std::cout << "Throughput: " << getThroughputMBps() << " MB/s" << std::endl;
    std::cout << "Characters/sec: " << getCharactersPerSecond() << std::endl;
    std::cout << "=========================" << std::endl;
}

void PerformanceMetrics::printCSVHeader() const {
    std::cout << "StringLength,Alignment,TotalChars,UniqueChars,ExecutionTimeMs,ThroughputMBps,CharsPerSecond" << std::endl;
}

void PerformanceMetrics::printCSVRow() const {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << stringLength << "," << alignment << "," << totalCharacters << "," 
              << uniqueCharacters << "," << executionTimeMs << "," 
              << getThroughputMBps() << "," << getCharactersPerSecond() << std::endl;
}

double PerformanceMetrics::getThroughputMBps() const {
    if (executionTimeMs <= 0) return 0.0;
    return (stringLength / (executionTimeMs / 1000.0)) / (1024.0 * 1024.0);
}

double PerformanceMetrics::getCharactersPerSecond() const {
    if (executionTimeMs <= 0) return 0.0;
    return totalCharacters / (executionTimeMs / 1000.0);
}

// RandomStringGenerator implementation
RandomStringGenerator::RandomStringGenerator(uint32_t seed) : seed(seed), rng(seed) {}

RandomStringGenerator::~RandomStringGenerator() {
    // Clean up any remaining allocated memory
    for (auto& pair : originalPointers) {
        free(pair.second);
    }
}

void RandomStringGenerator::resetSeed() {
    rng.seed(seed);
}

void* RandomStringGenerator::generateAlignedString(size_t length, size_t alignment) {
    if (!isPowerOfTwo(alignment)) {
        throw std::invalid_argument("Alignment must be power of 2");
    }
    
    if (length == 0) {
        throw std::invalid_argument("Length must be greater than 0");
    }

    // Calculate total size needed (string + alignment padding)
    size_t totalSize = length + alignment - 1;
    
    // Allocate raw memory
    void* rawMemory = malloc(totalSize);
    if (!rawMemory) {
        throw std::bad_alloc();
    }
    
    // Align the memory address
    void* alignedMemory = align(alignment, length, rawMemory, totalSize);
    
    if (!alignedMemory) {
        free(rawMemory);
        throw std::runtime_error("Failed to align memory");
    }
    
    // Generate random UTF-8 string in the aligned memory
    generateRandomUTF8(static_cast<char*>(alignedMemory), length);
    
    // Store original pointer for proper deallocation
    originalPointers[alignedMemory] = rawMemory;
    
    return alignedMemory;
}

void RandomStringGenerator::freeAlignedString(void* alignedPtr) {
    auto it = originalPointers.find(alignedPtr);
    if (it != originalPointers.end()) {
        free(it->second);
        originalPointers.erase(it);
    }
}

void* RandomStringGenerator::align(size_t alignment, size_t size, void* ptr, size_t space) {
    return std::align(alignment, size, ptr, space);
}

void RandomStringGenerator::generateRandomUTF8(char* buffer, size_t length) {
    // Use printable ASCII characters (32-126) for simplicity and consistency
    std::uniform_int_distribution<int> dist(32, 126);
    
    for (size_t i = 0; i < length - 1; ++i) {
        buffer[i] = static_cast<char>(dist(rng));
    }
    buffer[length - 1] = '\0'; // Null-terminator
}

// Utility functions
TestConfiguration getUserConfiguration() {
    TestConfiguration config;
    
    std::cout << "\n=== Character Frequency Analysis Configuration ===" << std::endl;
    
    // Get string length
    do {
        std::cout << "Enter string length (bytes, minimum 16): ";
        std::cin >> config.stringLength;
        
        if (std::cin.fail() || config.stringLength < 16) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number >= 16." << std::endl;
            continue;
        }
        break;
    } while (true);
    
    // Get memory alignment
    do {
        std::cout << "Enter memory alignment (bytes, must be power of 2: 1, 2, 4, 8, 16, 32, 64): ";
        std::cin >> config.alignment;
        
        if (std::cin.fail() || !isPowerOfTwo(config.alignment)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Alignment must be a power of 2." << std::endl;
            continue;
        }
        break;
    } while (true);
    
    // Get number of repetitions
    do {
        std::cout << "Enter number of repetitions for averaging (1-1000): ";
        std::cin >> config.repetitions;
        
        if (std::cin.fail() || config.repetitions < 1 || config.repetitions > 1000) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Must be between 1 and 1000." << std::endl;
            continue;
        }
        break;
    } while (true);
    
    // Ask for detailed frequency results
    char showDetailed;
    std::cout << "Show detailed character frequency analysis? (y/n): ";
    std::cin >> showDetailed;
    config.showDetailedFrequency = (showDetailed == 'y' || showDetailed == 'Y');
    
    // Ask for CSV export
    char exportCSV;
    std::cout << "Export results to CSV format? (y/n): ";
    std::cin >> exportCSV;
    config.exportCSV = (exportCSV == 'y' || exportCSV == 'Y');
    
    // Set deterministic seed for reproducible results
    config.randomSeed = 42;
    
    std::cout << "Using deterministic seed: " << config.randomSeed << " (for SIMD comparison)" << std::endl;
    
    return config;
}

void validateConfiguration(const TestConfiguration& config) {
    if (config.stringLength < 16) {
        throw std::invalid_argument("String length must be at least 16 bytes");
    }
    
    if (!isPowerOfTwo(config.alignment)) {
        throw std::invalid_argument("Alignment must be a power of 2");
    }
    
    if (config.repetitions < 1 || config.repetitions > 1000) {
        throw std::invalid_argument("Repetitions must be between 1 and 1000");
    }
}

bool isPowerOfTwo(size_t value) {
    return value > 0 && (value & (value - 1)) == 0;
}
