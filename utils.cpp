#include "utils.h"

// PerformanceMetrics implementation
void PerformanceMetrics::print() const {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== Performance Metrics ===" << std::endl;
    std::cout << "String Length: " << stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << alignment << " bytes" << std::endl;
    std::cout << "Search Character: '" << searchCharacter << "'" << std::endl;
    std::cout << "Character Count: " << characterCount << std::endl;
    std::cout << "Execution Time: " << executionTimeMs << " ms" << std::endl;
    std::cout << "Memory Used: " << memoryUsedBytes << " bytes" << std::endl;
    std::cout << "Throughput: " << getThroughputMBps() << " MB/s" << std::endl;
    std::cout << "=========================" << std::endl;
}

void PerformanceMetrics::printCSVHeader() const {
    std::cout << "StringLength,Alignment,SearchChar,Count,ExecutionTimeMs,ThroughputMBps" << std::endl;
}

void PerformanceMetrics::printCSVRow() const {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << stringLength << "," << alignment << "," << searchCharacter << "," 
              << characterCount << "," << executionTimeMs << "," 
              << getThroughputMBps() << std::endl;
}

double PerformanceMetrics::getThroughputMBps() const {
    return (stringLength / (executionTimeMs / 1000.0)) / 1024.0 / 1024.0;
}

// RandomStringGenerator implementation
RandomStringGenerator::RandomStringGenerator() : rng(std::random_device{}()) {}

RandomStringGenerator::~RandomStringGenerator() {
    // Clean up any remaining allocated memory
    for (auto& pair : originalPointers) {
        free(pair.second);
    }
}

void* RandomStringGenerator::generateAlignedString(size_t length, size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        throw std::invalid_argument("Alineamiento debe ser potencia de 2.");
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
        throw std::runtime_error("Error al alinear");
    }
    
    // Generate random UTF-8 string in the aligned memory
    generateRandomUTF8(static_cast<char*>(alignedMemory), length);
    
    // Store original pointer for proper deallocation
    originalPointers[alignedMemory] = rawMemory;
    
    return alignedMemory;
}

void* RandomStringGenerator::generateAlignedStringWithFrequency(size_t length, size_t alignment, 
                                                              char targetChar, double frequency) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        throw std::invalid_argument("Alineamiento debe ser potencia de 2.");
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
        throw std::runtime_error("Error al alinear");
    }
    
    // Generate random UTF-8 string with specific frequency
    generateRandomUTF8WithFrequency(static_cast<char*>(alignedMemory), length, targetChar, frequency);
    
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
    std::uniform_int_distribution<int> dist(32, 126); // ASCII printable chars
    
    for (size_t i = 0; i < length - 1; ++i) {
        buffer[i] = static_cast<char>(dist(rng));
    }
    buffer[length - 1] = '\0'; // Null-terminator
}

void RandomStringGenerator::generateRandomUTF8WithFrequency(char* buffer, size_t length, 
                                                          char targetChar, double frequency) {
    std::uniform_int_distribution<int> charDist(32, 126); // ASCII printable chars
    std::uniform_real_distribution<double> freqDist(0.0, 1.0);
    
    for (size_t i = 0; i < length - 1; ++i) {
        if (freqDist(rng) < frequency) {
            buffer[i] = targetChar;
        } else {
            do {
                buffer[i] = static_cast<char>(charDist(rng));
            } while (buffer[i] == targetChar); // Ensure we don't accidentally include target char
        }
    }
    buffer[length - 1] = '\0'; // Null-terminator
}

// CharacterCounterBase implementation
std::unordered_map<char, size_t> CharacterCounterBase::countAllCharacters(const char* str, size_t length) {
    std::unordered_map<char, size_t> charCounts;
    
    for (size_t i = 0; i < length - 1; ++i) { // -1 to skip null terminator
        charCounts[str[i]]++;
    }
    
    return charCounts;
}


