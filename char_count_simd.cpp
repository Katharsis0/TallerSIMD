// char_count_simd.cpp
// CE-4302 Arquitectura de Computadores II

#include "utils.h"
#include <immintrin.h>
#include <nmmintrin.h>
#include <algorithm>  
#include <fstream>   

// Forward declarations
void displayCharacterOccurrences(char targetChar, size_t occurrences, size_t totalChars);
void exportResultsCSV(char targetChar, size_t occurrences, size_t totalChars, 
                     const std::vector<double>& executionTimes, const TestConfiguration& config,
                     const std::string& filename);

/**
 * SIMD implementation of character occurrence counter using SSE4.2 intrinsics
 * Counts occurrences of a SPECIFIC character in the string
 */
class SIMDCharacterCounter : public CharacterCounterBase {
public:
    size_t countCharacterOccurrences(const char* str, size_t length, char targetChar,
                                   PerformanceMetrics& metrics) override {
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        size_t totalOccurrences = countCharacterSIMD(str, length - 1, targetChar);
       
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        
        // Fill performance metrics
        metrics.executionTimeMs = duration.count() / 1000000.0;
        metrics.memoryUsedBytes = length;
        metrics.stringLength = length;
        metrics.totalCharacters = length - 1;
        metrics.targetCharacter = targetChar;
        metrics.occurrences = totalOccurrences;
        
        return totalOccurrences;
    }
    
    std::string getImplementationName() const override {
        return "SIMD-SSE4.2";
    }

private:
    /**
     * SIMD implementation to count occurrences of a specific character
     * Based on the workshop requirements using SSE4.2 intrinsics
     */
    size_t countCharacterSIMD(const char* str, size_t length, char targetChar) {
        size_t total = 0;
        size_t i = 0;
        
        // Broadcast the target character to all positions in a 128-bit vector (16 bytes)
        // This implements the intrinsic from point b) of the workshop
        __m128i vector_char = _mm_set1_epi8(targetChar);

        // Process 16 bytes at a time using SIMD
        for (; i <= length - 16; i += 16) {
            // Load 16 bytes from string (handles unaligned data as required)
            __m128i string_block = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + i));
            
            // Compare each byte with the target character
            // This implements the intrinsic from point c) of the workshop
            __m128i comparison_result = _mm_cmpeq_epi8(string_block, vector_char);
            
            // Create a bitmask from the comparison results
            // This implements the get_mask() function mentioned in the workshop
            int mask = _mm_movemask_epi8(comparison_result);
            
            // Count the number of set bits (1s) in the mask
            // This implements the count_ones() function mentioned in the workshop
            total += _mm_popcnt_u32(static_cast<unsigned int>(mask));
        }

        // Handle remaining bytes (less than 16) - "safe" handling as required
        // This ensures we process all characters even when length is not multiple of 16
        for (; i < length; ++i) {
            if (str[i] == targetChar) {
                ++total;
            }
        }

        return total;
    }
};



/**
 * Run performance analysis with given configuration
 */
void runPerformanceAnalysis(SIMDCharacterCounter& counter, const TestConfiguration& config) {
    std::cout << "\n=== Performance Analysis ===" << std::endl;
    std::cout << "Implementation: " << counter.getImplementationName() << std::endl;
    std::cout << "Target Character: '" << config.targetCharacter << "' (ASCII: " << static_cast<int>(config.targetCharacter) << ")" << std::endl;
    std::cout << "String Length: " << config.stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << config.alignment << " bytes" << std::endl;
    std::cout << "Repetitions: " << config.repetitions << std::endl;
    std::cout << "Random Seed: " << config.randomSeed << std::endl;
    
    RandomStringGenerator generator(config.randomSeed);
    
    try {
        // Generate aligned string with deterministic random characters
        std::cout << "\nGenerating deterministic random string..." << std::endl;
        void* aligned = generator.generateAlignedString(config.stringLength, config.alignment);
        
        std::cout << "Searching for character '" << config.targetCharacter << "' using SIMD..." << std::endl;
        
        // Performance measurements
        std::vector<double> executionTimes;
        size_t totalOccurrences = 0;
        
        // Run multiple repetitions with same string
        for (int rep = 0; rep < config.repetitions; ++rep) {
            PerformanceMetrics metrics;
            
            size_t occurrences = counter.countCharacterOccurrences(
                static_cast<char*>(aligned), config.stringLength, config.targetCharacter, metrics);
            
            executionTimes.push_back(metrics.executionTimeMs);
            
            // Store occurrences from first run (should be identical across runs)
            if (rep == 0) {
                totalOccurrences = occurrences;
            }
            
            // Show progress for long tests
            if (config.repetitions > 10 && (rep + 1) % (config.repetitions / 10) == 0) {
                std::cout << "Progress: " << ((rep + 1) * 100 / config.repetitions) << "%" << std::endl;
            }
        }
        
        // Calculate performance statistics
        double totalTime = std::accumulate(executionTimes.begin(), executionTimes.end(), 0.0);
        double avgTime = totalTime / config.repetitions;
        
        // Calculate standard deviation
        double variance = 0;
        for (double time : executionTimes) {
            variance += (time - avgTime) * (time - avgTime);
        }
        double stdDev = std::sqrt(variance / config.repetitions);
        
        // Find min/max times
        double minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
        double maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
        
        // Calculate derived metrics
        size_t totalChars = config.stringLength - 1; // Exclude null terminator
        double avgThroughput = (config.stringLength / (avgTime / 1000.0)) / (1024.0 * 1024.0);
        double avgCharsPerSec = totalChars / (avgTime / 1000.0);
        
        // Display results
        displayCharacterOccurrences(config.targetCharacter, totalOccurrences, totalChars);
        
        std::cout << "\n=== Performance Results ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Average Execution Time: " << avgTime << " ms" << std::endl;
        std::cout << "Standard Deviation: " << stdDev << " ms" << std::endl;
        std::cout << "Min Execution Time: " << minTime << " ms" << std::endl;
        std::cout << "Max Execution Time: " << maxTime << " ms" << std::endl;
        std::cout << "Average Throughput: " << avgThroughput << " MB/s" << std::endl;
        std::cout << "Characters per Second: " << avgCharsPerSec << std::endl;
        
        // Memory alignment verification
        std::cout << "\n=== Memory Alignment Verification ===" << std::endl;
        uintptr_t address = reinterpret_cast<uintptr_t>(aligned);
        std::cout << "Memory Address: 0x" << std::hex << address << std::dec << std::endl;
        std::cout << "Alignment Check: " << (address % config.alignment == 0 ? "PASSED" : "FAILED") << std::endl;
        std::cout << "Address modulo alignment: " << (address % config.alignment) << std::endl;
        
        // CSV output
        if (config.exportCSV) {
            std::cout << "\n=== CSV Export ===" << std::endl;
            std::cout << "StringLength,Alignment,TargetChar,TotalChars,Occurrences,AvgTimeMs,StdDevMs,MinTimeMs,MaxTimeMs,ThroughputMBps,CharsPerSec" << std::endl;
            std::cout << config.stringLength << "," << config.alignment << "," << config.targetCharacter << "," << totalChars << "," 
                      << totalOccurrences << "," << avgTime << "," << stdDev << "," << minTime << "," << maxTime << "," 
                      << avgThroughput << "," << avgCharsPerSec << std::endl;
            
            exportResultsCSV(config.targetCharacter, totalOccurrences, totalChars, executionTimes, config, "simd_results.csv");
        }
        
        generator.freeAlignedString(aligned);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during performance analysis: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "======================================================" << std::endl;
    std::cout << "   SIMD Character Occurrence Counting                " << std::endl;
    std::cout << "   CE-4302 Arquitectura de Computadores II           " << std::endl;
    std::cout << "======================================================" << std::endl;
    
    SIMDCharacterCounter counter;
    
    try {
        // Get user configuration
        TestConfiguration config = getUserConfiguration();
        validateConfiguration(config);
        
        // Run main performance analysis
        runPerformanceAnalysis(counter, config);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nSIMD character occurrence counting completed successfully!" << std::endl;
    
    return 0;
}