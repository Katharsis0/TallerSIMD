// char_count_serial.cpp
// Serial character occurrence counting implementation
// CE-4302 Arquitectura de Computadores II

#include "utils.h"
#include <vector>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <numeric>

// Forward declarations
void displayCharacterOccurrences(char targetChar, size_t occurrences, size_t totalChars);
void exportResultsCSV(char targetChar, size_t occurrences, size_t totalChars, 
                     const std::vector<double>& executionTimes, const TestConfiguration& config,
                     const std::string& filename);

/**
 * Serial implementation of character occurrence counter
 * Counts occurrences of a SPECIFIC character in the string
 */
class SerialCharacterCounter : public CharacterCounterBase {
public:
    size_t countCharacterOccurrences(const char* str, size_t length, char targetChar,
                                   PerformanceMetrics& metrics) override {
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        size_t occurrences = 0;
        
        // Serial algorithm: iterate through each character and count target occurrences
        // Note: length includes null terminator, so we process length-1 characters
        for (size_t i = 0; i < length - 1; ++i) {
            if (str[i] == targetChar) {
                occurrences++;
            }
        }
       
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        
        // Fill performance metrics
        metrics.executionTimeMs = duration.count() / 1000000.0; // Convert to milliseconds
        metrics.memoryUsedBytes = length;
        metrics.stringLength = length;
        metrics.totalCharacters = length - 1; // Exclude null terminator
        metrics.targetCharacter = targetChar;
        metrics.occurrences = occurrences;
        
        return occurrences;
    }
    
    std::string getImplementationName() const override {
        return "Serial";
    }
};

/**
 * Display character occurrence results in a readable format
 */




/**
 * Run performance analysis with given configuration
 */
void runPerformanceAnalysis(SerialCharacterCounter& counter, const TestConfiguration& config) {
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
        
        std::cout << "Searching for character '" << config.targetCharacter << "'..." << std::endl;
        
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
            
            exportResultsCSV(config.targetCharacter, totalOccurrences, totalChars, executionTimes, config, "serial_results.csv");
        }
        
        generator.freeAlignedString(aligned);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during performance analysis: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "======================================================" << std::endl;
    std::cout << "   Serial Character Occurrence Counting             " << std::endl;
    std::cout << "   CE-4302 Arquitectura de Computadores II           " << std::endl;
    std::cout << "======================================================" << std::endl;
    
    SerialCharacterCounter counter;
    
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
    
    std::cout << "\nSerial character occurrence counting completed successfully!" << std::endl;
    
    return 0;
}