#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <iomanip>

/**
 * Performance metrics structure to standardize measurements
 * between serial and SIMD implementations
 */
struct PerformanceMetrics {
    double executionTimeMs = 0.0;
    size_t memoryUsedBytes = 0;
    size_t stringLength = 0;
    size_t alignment = 0;
    size_t totalCharacters = 0;       // Total characters processed
    size_t uniqueCharacters = 0;      // Number of unique characters found
    
    void print() const;
    void printCSVHeader() const;
    void printCSVRow() const;
    double getThroughputMBps() const;
    double getCharactersPerSecond() const;
};

/**
 * Deterministic random string generator with configurable alignment
 * Uses fixed seed for reproducible results between serial and SIMD implementations
 */
class RandomStringGenerator {
public:
    explicit RandomStringGenerator(uint32_t seed = 42);
    ~RandomStringGenerator();
    
    /**
     * Generate aligned string with specified length and alignment
     * @param length Total length including null terminator
     * @param alignment Memory alignment (must be power of 2)
     * @return Pointer to aligned memory containing random string
     */
    void* generateAlignedString(size_t length, size_t alignment);
    
    /**
     * Free previously allocated aligned string
     * @param alignedPtr Pointer returned by generateAlignedString
     */
    void freeAlignedString(void* alignedPtr);
    
    /**
     * Reset generator to initial seed state for reproducible results
     */
    void resetSeed();
    
    /**
     * Get current seed value
     */
    uint32_t getSeed() const { return seed; }

private:
    std::mt19937 rng;
    uint32_t seed;
    std::unordered_map<void*, void*> originalPointers;
    
    void* align(size_t alignment, size_t size, void* ptr, size_t space);
    void generateRandomUTF8(char* buffer, size_t length);
};

/**
 * Base class for character counting algorithms
 * Ensures consistent interface between serial and SIMD versions
 */
class CharacterCounterBase {
public:
    virtual ~CharacterCounterBase() = default;
    
    /**
     * Count ALL characters in string and return frequency map
     * @param str Input string
     * @param length String length (including null terminator)
     * @param metrics Output performance metrics
     * @return Map of character frequencies
     */
    virtual std::unordered_map<char, size_t> countAllCharacters(const char* str, size_t length, 
                                                              PerformanceMetrics& metrics) = 0;
    
    /**
     * Get implementation name for reporting
     */
    virtual std::string getImplementationName() const = 0;
};

/**
 * Test configuration structure for user input
 */
struct TestConfiguration {
    size_t stringLength;
    size_t alignment;
    int repetitions;
    bool exportCSV;
    bool showDetailedFrequency;
    uint32_t randomSeed;
};

/**
 * Utility functions
 */
TestConfiguration getUserConfiguration();
void validateConfiguration(const TestConfiguration& config);
bool isPowerOfTwo(size_t value);

#endif // UTILS_H
