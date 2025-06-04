// char_count_serial.cpp
// Serial character counting implementation with user input
#include "utils.h"
#include <limits>

/**
 * Serial implementation of character counter
 * Counts ALL characters in the string and returns character frequency map
 */
class SerialCharacterCounter : public CharacterCounterBase {
public:
    //Updated to count all characters, not just a target
    std::unordered_map<char, size_t> countAllCharacters(const char* str, size_t length, 
                                                       PerformanceMetrics& metrics) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::unordered_map<char, size_t> charCounts;
        
        // Serial algorithm: iterate through each character and count all
        for (size_t i = 0; i < length - 1; ++i) { // -1 to skip null terminator
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
    
    size_t countCharacter(const char* str, size_t length, char target, 
                         PerformanceMetrics& metrics) override {
        auto allCounts = countAllCharacters(str, length, metrics);
        metrics.searchCharacter = target;
        metrics.characterCount = allCounts[target];
        return allCounts[target];
    }
    
    std::string getImplementationName() const override {
        return "Serial";
    }
};

/**
 * Get user input for test configuration
 */
struct TestConfiguration {
    size_t stringLength;
    size_t alignment;
    int repetitions;
    bool showDetailedResults;
    bool exportCSV;
};

TestConfiguration getUserConfiguration() {
    TestConfiguration config;
    
    std::cout << "\n=== Test Configuration ===" << std::endl;
    
    // Get string length
    do {
        std::cout << "Enter string length (bytes, min 1024): ";
        std::cin >> config.stringLength;
        
        if (std::cin.fail() || config.stringLength < 1024) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number >= 1024." << std::endl;
            continue;
        }
        break;
    } while (true);
    
    // Get memory alignment
    do {
        std::cout << "Enter memory alignment (bytes, must be power of 2: 1, 2, 4, 8, 16, 32, 64): ";
        std::cin >> config.alignment;
        
        if (std::cin.fail() || config.alignment == 0 || (config.alignment & (config.alignment - 1)) != 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Alignment must be a power of 2." << std::endl;
            continue;
        }
        break;
    } while (true);
    
    // Get number of repetitions for averaging
    do {
        std::cout << "Enter number of repetitions for averaging (min 1, recommended 10-100): ";
        std::cin >> config.repetitions;
        
        if (std::cin.fail() || config.repetitions < 1) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Must be at least 1." << std::endl;
            continue;
        }
        break;
    } while (true);
    
    // Ask for detailed results
    char showDetailed;
    std::cout << "Show detailed character frequency results? (y/n): ";
    std::cin >> showDetailed;
    config.showDetailedResults = (showDetailed == 'y' || showDetailed == 'Y');
    
    // Ask for CSV export
    char exportCSV;
    std::cout << "Export results to CSV format? (y/n): ";
    std::cin >> exportCSV;
    config.exportCSV = (exportCSV == 'y' || exportCSV == 'Y');
    
    return config;
}

/**
 * Performance analysis function with user-specified parameters
 * Now counts ALL characters in the string
 */
void analyzePerformance(SerialCharacterCounter& counter, const TestConfiguration& config) {
    std::cout << "\n=== Performance Analysis ===" << std::endl;
    std::cout << "Implementation: " << counter.getImplementationName() << std::endl;
    std::cout << "String Length: " << config.stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << config.alignment << " bytes" << std::endl;
    std::cout << "Repetitions: " << config.repetitions << std::endl;
    
    RandomStringGenerator generator;
    
    try {
        // Generate aligned string with random characters
        std::cout << "Generating random string with uniform character distribution..." << std::endl;
        void* aligned = generator.generateAlignedString(config.stringLength, config.alignment);
        
        std::cout << "\nRunning character counting tests..." << std::endl;
        
        // Performance measurements
        double totalTime = 0;
        std::vector<double> executionTimes;
        std::unordered_map<char, size_t> aggregatedCounts;
        size_t totalUniqueChars = 0;
        
        // Run multiple repetitions
        for (int rep = 0; rep < config.repetitions; ++rep) {
            PerformanceMetrics metrics;
            metrics.alignment = config.alignment;
            
            // Count ALL characters in the string
            auto charCounts = counter.countAllCharacters(
                static_cast<char*>(aligned), config.stringLength, metrics);
            
            totalTime += metrics.executionTimeMs;
            executionTimes.push_back(metrics.executionTimeMs);
            
            // Aggregate character counts across repetitions
            if (rep == 0) {
                aggregatedCounts = charCounts;
                totalUniqueChars = charCounts.size();
            }
            
            // Show progress for long tests
            if (config.repetitions > 10 && (rep + 1) % (config.repetitions / 10) == 0) {
                std::cout << "Progress: " << ((rep + 1) * 100 / config.repetitions) << "%" << std::endl;
            }
        }
        
        // Calculate statistics
        double avgTime = totalTime / config.repetitions;
        double avgThroughput = (config.stringLength / (avgTime / 1000.0)) / 1024.0 / 1024.0;
        
        // Calculate standard deviation
        double variance = 0;
        for (double time : executionTimes) {
            variance += (time - avgTime) * (time - avgTime);
        }
        double stdDev = std::sqrt(variance / config.repetitions);
        
        // Display results
        std::cout << "\n=== Character Counting Results ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Total Characters Processed: " << (config.stringLength - 1) << std::endl;
        std::cout << "Unique Characters Found: " << totalUniqueChars << std::endl;
        std::cout << "Average Execution Time: " << avgTime << " ms" << std::endl;
        std::cout << "Standard Deviation: " << stdDev << " ms" << std::endl;
        std::cout << "Average Throughput: " << avgThroughput << " MB/s" << std::endl;
        std::cout << "Characters per Second: " << ((config.stringLength - 1) / (avgTime / 1000.0)) << std::endl;
        
        // Show detailed character frequency if requested
        if (config.showDetailedResults) {
            std::cout << "\n=== Character Frequency Distribution ===" << std::endl;
            
            // Sort characters by frequency for better readability
            std::vector<std::pair<char, size_t>> sortedChars(aggregatedCounts.begin(), aggregatedCounts.end());
            std::sort(sortedChars.begin(), sortedChars.end(), 
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            std::cout << "Character | Count | Frequency (%)" << std::endl;
            std::cout << "----------|-------|-------------" << std::endl;
            
            for (const auto& pair : sortedChars) {
                char ch = pair.first;
                size_t count = pair.second;
                double frequency = (static_cast<double>(count) / (config.stringLength - 1)) * 100.0;
                
                // Handle special characters for display
                std::string charDisplay;
                if (ch == ' ') charDisplay = "SPACE";
                else if (ch == '\t') charDisplay = "TAB";
                else if (ch == '\n') charDisplay = "NEWLINE";
                else if (ch >= 32 && ch <= 126) charDisplay = std::string(1, ch);
                else charDisplay = "ASCII(" + std::to_string(static_cast<int>(ch)) + ")";
                
                std::cout << std::setw(9) << charDisplay << " | " 
                         << std::setw(5) << count << " | " 
                         << std::setw(10) << std::setprecision(3) << frequency << std::endl;
            }
        }
        
        // CSV output for further analysis
        if (config.exportCSV) {
            std::cout << "\n=== CSV Data for Analysis ===" << std::endl;
            std::cout << "StringLength,Alignment,UniqueChars,TotalChars,AvgExecutionTimeMs,StdDevMs,AvgThroughputMBps,CharsPerSecond" << std::endl;
            std::cout << config.stringLength << "," << config.alignment << "," << totalUniqueChars << "," 
                      << (config.stringLength - 1) << "," << avgTime << "," << stdDev << "," << avgThroughput << "," 
                      << ((config.stringLength - 1) / (avgTime / 1000.0)) << std::endl;
            
            // Export character frequency data
            std::cout << "\n=== Character Frequency CSV ===" << std::endl;
            std::cout << "Character,ASCII_Code,Count,Frequency_Percent" << std::endl;
            for (const auto& pair : aggregatedCounts) {
                char ch = pair.first;
                size_t count = pair.second;
                double frequency = (static_cast<double>(count) / (config.stringLength - 1)) * 100.0;
                
                std::cout << "\"" << ch << "\"," << static_cast<int>(ch) << "," 
                         << count << "," << frequency << std::endl;
            }
        }
        
        generator.freeAlignedString(aligned);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during performance analysis: " << e.what() << std::endl;
    }
}

/**
 * Multiple size analysis for cache behavior study
 * Tests character counting performance across different memory sizes
 */
void analyzeCacheBehavior(SerialCharacterCounter& counter) {
    std::cout << "\n=== Cache Behavior Analysis ===" << std::endl;
    
    char choice;
    std::cout << "Do you want to run cache behavior analysis with multiple sizes? (y/n): ";
    std::cin >> choice;
    
    if (choice != 'y' && choice != 'Y') {
        return;
    }
    
    RandomStringGenerator generator;
    
    // Test cache effects with different sizes
    std::vector<size_t> cacheSizes = {
        1024,       // 1KB - L1 cache
        32768,      // 32KB - L1 cache boundary
        262144,     // 256KB - L2 cache
        1048576,    // 1MB - L2 cache boundary
        8388608,    // 8MB - L3 cache
        33554432    // 32MB - Beyond L3 cache
    };
    
    size_t alignment = 32;
    const int repetitions = 20;
    
    std::cout << "\nCache behavior test configuration:" << std::endl;
    std::cout << "Alignment: " << alignment << " bytes" << std::endl;
    std::cout << "Repetitions per size: " << repetitions << std::endl;
    
    std::cout << "\n=== Cache Behavior Results ===" << std::endl;
    std::cout << "Size(bytes),Size(description),AvgTime(ms),Throughput(MB/s),UniqueChars,CharsPerSecond" << std::endl;
    
    for (size_t size : cacheSizes) {
        try {
            void* aligned = generator.generateAlignedString(size, alignment);
            
            double totalTime = 0;
            size_t avgUniqueChars = 0;
            
            for (int rep = 0; rep < repetitions; ++rep) {
                PerformanceMetrics metrics;
                auto charCounts = counter.countAllCharacters(static_cast<char*>(aligned), size, metrics);
                totalTime += metrics.executionTimeMs;
                if (rep == 0) avgUniqueChars = charCounts.size();
            }
            
            double avgTime = totalTime / repetitions;
            double throughput = (size / (avgTime / 1000.0)) / 1024.0 / 1024.0;
            double charsPerSecond = (size - 1) / (avgTime / 1000.0);
            
            std::string sizeDesc;
            if (size < 1024) sizeDesc = std::to_string(size) + "B";
            else if (size < 1048576) sizeDesc = std::to_string(size/1024) + "KB";
            else sizeDesc = std::to_string(size/1048576) + "MB";
            
            std::cout << size << "," << sizeDesc << "," << avgTime << "," << throughput << "," 
                     << avgUniqueChars << "," << charsPerSecond << std::endl;
            
            generator.freeAlignedString(aligned);
            
        } catch (const std::exception& e) {
            std::cerr << "Error with size " << size << ": " << e.what() << std::endl;
        }
    }
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "   Serial Character Counting Benchmark         " << std::endl;
    std::cout << "   CE-4302 Arquitectura de Computadores II     " << std::endl;
    std::cout << "================================================" << std::endl;
    
    SerialCharacterCounter counter;
    
    try {
        // Get user configuration
        TestConfiguration config = getUserConfiguration();
        
        // Run performance analysis with user parameters
        analyzePerformance(counter, config);
        
        // Optional cache behavior analysis
        //analyzeCacheBehavior(counter);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nBenchmark completed successfully!" << std::endl;
    return 0;
}
