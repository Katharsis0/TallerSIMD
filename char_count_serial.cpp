// char_count_serial.cpp
// Serial character frequency analysis implementation
// CE-4302 Arquitectura de Computadores II

#include "utils.h"
#include <vector>
#include <algorithm>

/**
 * Serial implementation of character frequency analyzer
 * Counts ALL characters in the string and returns frequency map
 */
class SerialCharacterCounter : public CharacterCounterBase {
public:
    std::unordered_map<char, size_t> countAllCharacters(const char* str, size_t length, 
                                                       PerformanceMetrics& metrics) override {
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::unordered_map<char, size_t> charCounts;
        
        // Serial algorithm: iterate through each character and count all occurrences
        // Note: length includes null terminator, so we process length-1 characters
        for (size_t i = 0; i < length - 1; ++i) {
            charCounts[str[i]]++;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        
        // Fill performance metrics
        metrics.executionTimeMs = duration.count() / 1000000.0; // Convert to milliseconds
        metrics.memoryUsedBytes = length;
        metrics.stringLength = length;
        metrics.totalCharacters = length - 1; // Exclude null terminator
        metrics.uniqueCharacters = charCounts.size();
        
        return charCounts;
    }
    
    std::string getImplementationName() const override {
        return "Serial";
    }
};

/**
 * Display character frequency results in a readable format
 */
void displayCharacterFrequency(const std::unordered_map<char, size_t>& charCounts, 
                              size_t totalChars, bool showDetailed) {
    
    if (!showDetailed) {
        std::cout << "Character frequency analysis completed. Use detailed view to see frequencies." << std::endl;
        return;
    }
    
    // Sort characters by frequency for better readability
    std::vector<std::pair<char, size_t>> sortedChars(charCounts.begin(), charCounts.end());
    std::sort(sortedChars.begin(), sortedChars.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "\n=== Character Frequency Distribution ===" << std::endl;
    std::cout << "Rank | Character | ASCII | Count | Frequency (%)" << std::endl;
    std::cout << "-----|-----------|-------|-------|-------------" << std::endl;
    
    int rank = 1;
    for (const auto& pair : sortedChars) {
        char ch = pair.first;
        size_t count = pair.second;
        double frequency = (static_cast<double>(count) / totalChars) * 100.0;
        
        // Handle special characters for display
        std::string charDisplay;
        if (ch == ' ') charDisplay = "SPACE";
        else if (ch == '\t') charDisplay = "TAB";
        else if (ch == '\n') charDisplay = "NEWLINE";
        else if (ch >= 32 && ch <= 126) charDisplay = std::string(1, ch);
        else charDisplay = "CTRL";
        
        std::cout << std::setw(4) << rank << " | " 
                 << std::setw(9) << charDisplay << " | " 
                 << std::setw(5) << static_cast<int>(ch) << " | "
                 << std::setw(5) << count << " | " 
                 << std::setw(10) << std::setprecision(3) << std::fixed << frequency << std::endl;
        
        rank++;
        if (rank > 20 && !showDetailed) break; // Limit display for readability
    }
    
    if (sortedChars.size() > 20) {
        std::cout << "... and " << (sortedChars.size() - 20) << " more characters" << std::endl;
    }
}

/**
 * Export character frequency data to CSV format
 */
void exportCharacterFrequencyCSV(const std::unordered_map<char, size_t>& charCounts, 
                                size_t totalChars) {
    std::cout << "\n=== Character Frequency CSV Data ===" << std::endl;
    std::cout << "Character,ASCII_Code,Count,Frequency_Percent" << std::endl;
    
    // Sort by ASCII value for consistent output
    std::vector<std::pair<char, size_t>> sortedChars(charCounts.begin(), charCounts.end());
    std::sort(sortedChars.begin(), sortedChars.end(), 
             [](const auto& a, const auto& b) { return a.first < b.first; });
    
    for (const auto& pair : sortedChars) {
        char ch = pair.first;
        size_t count = pair.second;
        double frequency = (static_cast<double>(count) / totalChars) * 100.0;
        
        std::cout << "\"" << ch << "\"," << static_cast<int>(ch) << "," 
                 << count << "," << std::fixed << std::setprecision(6) << frequency << std::endl;
    }
}

/**
 * Run performance analysis with given configuration
 */
void runPerformanceAnalysis(SerialCharacterCounter& counter, const TestConfiguration& config) {
    std::cout << "\n=== Performance Analysis ===" << std::endl;
    std::cout << "Implementation: " << counter.getImplementationName() << std::endl;
    std::cout << "String Length: " << config.stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << config.alignment << " bytes" << std::endl;
    std::cout << "Repetitions: " << config.repetitions << std::endl;
    std::cout << "Random Seed: " << config.randomSeed << std::endl;
    
    RandomStringGenerator generator(config.randomSeed);
    
    try {
        // Generate aligned string with deterministic random characters
        std::cout << "\nGenerating deterministic random string..." << std::endl;
        void* aligned = generator.generateAlignedString(config.stringLength, config.alignment);
        
        std::cout << "Running character frequency analysis..." << std::endl;
        
        // Performance measurements
        std::vector<double> executionTimes;
        std::unordered_map<char, size_t> finalCharCounts;
        
        // Run multiple repetitions with same string
        for (int rep = 0; rep < config.repetitions; ++rep) {
            PerformanceMetrics metrics;
            metrics.alignment = config.alignment;
            
            auto charCounts = counter.countAllCharacters(
                static_cast<char*>(aligned), config.stringLength, metrics);
            
            executionTimes.push_back(metrics.executionTimeMs);
            
            // Store character counts from first run (should be identical across runs)
            if (rep == 0) {
                finalCharCounts = charCounts;
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
        
        // Display performance results
        std::cout << "\n=== Character Analysis Results ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Total Characters Analyzed: " << totalChars << std::endl;
        std::cout << "Unique Characters Found: " << finalCharCounts.size() << std::endl;
        
        std::cout << "\n=== Performance Results ===" << std::endl;
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
        
        // Display character frequency analysis
        displayCharacterFrequency(finalCharCounts, totalChars, config.showDetailedFrequency);
        
        // CSV output for analysis scripts
        if (config.exportCSV) {
            std::cout << "\n=== Performance CSV Export ===" << std::endl;
            std::cout << "StringLength,Alignment,TotalChars,UniqueChars,AvgTimeMs,StdDevMs,MinTimeMs,MaxTimeMs,ThroughputMBps,CharsPerSec" << std::endl;
            std::cout << config.stringLength << "," << config.alignment << "," << totalChars << "," 
                      << finalCharCounts.size() << "," << avgTime << "," << stdDev << "," << minTime << "," << maxTime << "," 
                      << avgThroughput << "," << avgCharsPerSec << std::endl;
            
            // Export character frequency data
            exportCharacterFrequencyCSV(finalCharCounts, totalChars);
        }
        
        generator.freeAlignedString(aligned);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during performance analysis: " << e.what() << std::endl;
        throw;
    }
}

/**
 * Run a batch test with multiple sizes for cache behavior analysis
 */
void runCacheBehaviorAnalysis(SerialCharacterCounter& counter, size_t alignment, uint32_t seed) {
    std::cout << "\n=== Cache Behavior Analysis ===" << std::endl;
    
    // Test sizes that span different cache levels
    std::vector<size_t> testSizes = {
        1024,      // 1KB  - L1 cache
        4096,      // 4KB  - Page size
        32768,     // 32KB - L1 cache boundary
        262144,    // 256KB - L2 cache
        1048576,   // 1MB  - L2/L3 boundary
        4194304,   // 4MB  - L3 cache
        16777216   // 16MB - Beyond cache
    };
    
    const int repetitions = 20;
    RandomStringGenerator generator(seed);
    
    std::cout << "Size(bytes),SizeDesc,AvgTimeMs,ThroughputMBps,CharsPerSec,UniqueChars,CharDensity" << std::endl;
    
    for (size_t size : testSizes) {
        try {
            void* aligned = generator.generateAlignedString(size, alignment);
            
            std::vector<double> times;
            size_t avgUniqueChars = 0;
            
            for (int rep = 0; rep < repetitions; ++rep) {
                PerformanceMetrics metrics;
                auto charCounts = counter.countAllCharacters(static_cast<char*>(aligned), size, metrics);
                times.push_back(metrics.executionTimeMs);
                if (rep == 0) avgUniqueChars = charCounts.size();
            }
            
            double avgTime = std::accumulate(times.begin(), times.end(), 0.0) / repetitions;
            double throughput = (size / (avgTime / 1000.0)) / (1024.0 * 1024.0);
            double charsPerSec = (size - 1) / (avgTime / 1000.0);
            double charDensity = static_cast<double>(avgUniqueChars) / (size - 1) * 100.0;
            
            std::string sizeDesc;
            if (size < 1024) sizeDesc = std::to_string(size) + "B";
            else if (size < 1048576) sizeDesc = std::to_string(size/1024) + "KB";
            else sizeDesc = std::to_string(size/1048576) + "MB";
            
            std::cout << std::fixed << std::setprecision(6);
            std::cout << size << "," << sizeDesc << "," << avgTime << "," << throughput 
                     << "," << charsPerSec << "," << avgUniqueChars << "," << charDensity << std::endl;
            
            generator.freeAlignedString(aligned);
            
        } catch (const std::exception& e) {
            std::cerr << "Error with size " << size << ": " << e.what() << std::endl;
        }
    }
}

int main() {
    std::cout << "======================================================" << std::endl;
    std::cout << "   Serial Character Frequency Analysis               " << std::endl;
    std::cout << "   CE-4302 Arquitectura de Computadores II           " << std::endl;
    std::cout << "======================================================" << std::endl;
    
    SerialCharacterCounter counter;
    
    try {
        // Get user configuration
        TestConfiguration config = getUserConfiguration();
        validateConfiguration(config);
        
        // Run main performance analysis
        runPerformanceAnalysis(counter, config);
        
        // Ask for cache behavior analysis
        char runCache;
        std::cout << "\nRun cache behavior analysis with multiple sizes? (y/n): ";
        std::cin >> runCache;
        
        if (runCache == 'y' || runCache == 'Y') {
            runCacheBehaviorAnalysis(counter, config.alignment, config.randomSeed);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nSerial character frequency analysis completed successfully!" << std::endl;
    std::cout << "Note: Use the same random seed (" << 42 << ") for SIMD implementation comparison." << std::endl;
    
    return 0;
}
