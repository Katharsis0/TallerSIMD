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

// Forward declarations
struct PerformanceMetrics;
class RandomStringGenerator;

/**
 * Performance metrics structure to standardize measurements
 * between serial and SIMD implementations
 */
struct PerformanceMetrics {
    double executionTimeMs = 0.0;
    size_t memoryUsedBytes = 0;
    size_t stringLength = 0;
    size_t alignment = 0;
    size_t characterCount = 0;        // Count of specific character (for compatibility)
    size_t totalCharacters = 0;       // Total characters processed
    size_t uniqueCharacters = 0;      // Number of unique characters found
    char searchCharacter = '\0';      // Target character (for compatibility)
    
    void print() const;
    void printCSVHeader() const;
    void printCSVRow() const;
    double getThroughputMBps() const;
};

/**
 * Random string generator with configurable alignment
 * Shared between serial and SIMD implementations
 */
class RandomStringGenerator {
public:
    RandomStringGenerator();
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
     * Generate string with specific character distribution for testing
     * @param length String length
     * @param alignment Memory alignment
     * @param targetChar Character to include with specified frequency
     * @param frequency Approximate frequency of targetChar (0.0 to 1.0)
     */
    void* generateAlignedStringWithFrequency(size_t length, size_t alignment, 
                                           char targetChar, double frequency);

private:
    std::mt19937 rng; // Random number generator
    std::unordered_map<void*, void*> originalPointers;
    
    void* align(size_t alignment, size_t size, void* ptr, size_t space);
    void generateRandomUTF8(char* buffer, size_t length);
    void generateRandomUTF8WithFrequency(char* buffer, size_t length, 
                                       char targetChar, double frequency);
};

/**
 * Base class for character counting algorithms
 * Ensures consistent interface between serial and SIMD versions
 */
class CharacterCounterBase {
public:
    virtual ~CharacterCounterBase() = default;
    
    /**
     * Count occurrences of target character in string
     * @param str Input string
     * @param length String length
     * @param target Character to count
     * @param metrics Output performance metrics
     * @return Number of occurrences
     */
    virtual size_t countCharacter(const char* str, size_t length, char target, 
                                PerformanceMetrics& metrics) = 0;
    
    /**
     * Get implementation name for reporting
     */
    virtual std::string getImplementationName() const = 0;
    
    /**
     * Validate correctness by counting all characters
     */
    std::unordered_map<char, size_t> countAllCharacters(const char* str, size_t length);
};

#endif // UTILS_H
