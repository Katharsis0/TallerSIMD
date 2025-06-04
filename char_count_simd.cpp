// char_count_simd.cpp
#include "utils.h"
#include <immintrin.h>
#include <nmmintrin.h>
#include <algorithm>  
#include <fstream>   

class SIMDCharacterCounter : public CharacterCounterBase {
public:
    std::unordered_map<char, size_t> countAllCharacters(const char* str, size_t length, 
                                                       PerformanceMetrics& metrics) override {
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::unordered_map<char, size_t> charCounts;

        // First, find all unique characters in the string to know which ones to count
        findUniqueCharacters(str, length - 1, charCounts);

        // Then count each unique character using SIMD
        for (auto& pair : charCounts) {
            pair.second = countCharacterOccurrences(str, length - 1, pair.first);
        }
       
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        
        // Fill performance metrics
        metrics.executionTimeMs = duration.count() / 1000000.0;
        metrics.memoryUsedBytes = length;
        metrics.stringLength = length;
        metrics.totalCharacters = length - 1;
        metrics.uniqueCharacters = charCounts.size();
        
        return charCounts;
    }
    
    std::string getImplementationName() const override {
        return "SIMD";
    }

private:
    // Helper function to find all unique characters in the string
    void findUniqueCharacters(const char* str, size_t length, 
                            std::unordered_map<char, size_t>& charCounts) {
        for (size_t i = 0; i < length; ++i) {
            charCounts[str[i]] = 0; // Initialize count to 0
        }
    }

    // SIMD implementation to count occurrences of a specific character
    size_t countCharacterOccurrences(const char* str, size_t length, char char_x) {
        size_t total = 0;
        size_t i = 0;
        
        // Broadcast the character to all positions in a 128-bit vector
        __m128i vector_char = _mm_set1_epi8(char_x);

        // Process 16 bytes at a time
        for (; i <= length - 16; i += 16) {
            // Load 16 bytes (unaligned)
            __m128i block = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + i));
            
            // Compare each byte with the target character
            __m128i comparison = _mm_cmpeq_epi8(block, vector_char);
            
            // Create a bitmask from the comparison results
            int mask = _mm_movemask_epi8(comparison);
            
            // Count the number of set bits (1s) in the mask
            total += _mm_popcnt_u32(static_cast<unsigned int>(mask));
        }

        // Process remaining bytes (less than 16)
        for (; i < length; ++i) {
            if (str[i] == char_x) {
                ++total;
            }
        }

        return total;
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
void runPerformanceAnalysis(SIMDCharacterCounter& counter, const TestConfiguration& config) {
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
 * Export comprehensive performance data to CSV file
 */
void exportPerformanceDataCSV(const std::vector<double>& executionTimes,
                             const TestConfiguration& config,
                             const std::unordered_map<char, size_t>& charCounts,
                             const std::string& filename = "") {
    
    std::string csvFilename = filename;
    if (csvFilename.empty()) {
        // Generate timestamp-based filename
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        char timestamp[100];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm);
        csvFilename = "simd_performance_" + std::string(timestamp) + ".csv";
    }
    
    std::ofstream file(csvFilename);
    if (!file.is_open()) {
        std::cerr << "Failed to create CSV file: " << csvFilename << std::endl;
        return;
    }
    
    // Calculate statistics
    double totalTime = std::accumulate(executionTimes.begin(), executionTimes.end(), 0.0);
    double avgTime = totalTime / executionTimes.size();
    
    double variance = 0;
    for (double time : executionTimes) {
        variance += (time - avgTime) * (time - avgTime);
    }
    double stdDev = std::sqrt(variance / executionTimes.size());
    double minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
    double maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
    
    size_t totalChars = config.stringLength - 1;
    double avgThroughput = (config.stringLength / (avgTime / 1000.0)) / (1024.0 * 1024.0);
    double avgCharsPerSec = totalChars / (avgTime / 1000.0);
    
    // Write metadata and summary statistics
    file << "# SIMD Character Frequency Analysis Results\n";
    file << "# Configuration\n";
    file << "StringLength," << config.stringLength << "\n";
    file << "Alignment," << config.alignment << "\n";
    file << "Repetitions," << config.repetitions << "\n";
    file << "RandomSeed," << config.randomSeed << "\n";
    file << "TotalCharacters," << totalChars << "\n";
    file << "UniqueCharacters," << charCounts.size() << "\n";
    file << "\n";
    
    // Summary statistics
    file << "# Performance Summary\n";
    file << "Metric,Value,Unit\n";
    file << "AvgExecutionTime," << std::fixed << std::setprecision(6) << avgTime << ",ms\n";
    file << "StdDeviation," << stdDev << ",ms\n";
    file << "MinExecutionTime," << minTime << ",ms\n";
    file << "MaxExecutionTime," << maxTime << ",ms\n";
    file << "AvgThroughput," << avgThroughput << ",MB/s\n";
    file << "AvgCharsPerSecond," << avgCharsPerSec << ",chars/s\n";
    file << "\n";
    
    // Individual execution times
    file << "# Individual Execution Times\n";
    file << "Run,ExecutionTime_ms,Throughput_MBps,CharsPerSecond\n";
    for (size_t i = 0; i < executionTimes.size(); ++i) {
        double throughput = (config.stringLength / (executionTimes[i] / 1000.0)) / (1024.0 * 1024.0);
        double charsPerSec = totalChars / (executionTimes[i] / 1000.0);
        file << (i + 1) << "," << executionTimes[i] << "," << throughput << "," << charsPerSec << "\n";
    }
    file << "\n";
    
    // Character frequency data
    file << "# Character Frequency Analysis\n";
    file << "Character,ASCII_Code,Count,Frequency_Percent,Printable\n";
    
    // Sort by frequency for better analysis
    std::vector<std::pair<char, size_t>> sortedChars(charCounts.begin(), charCounts.end());
    std::sort(sortedChars.begin(), sortedChars.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& pair : sortedChars) {
        char ch = pair.first;
        size_t count = pair.second;
        double frequency = (static_cast<double>(count) / totalChars) * 100.0;
        bool isPrintable = (ch >= 32 && ch <= 126);
        
        // Escape special characters for CSV
        std::string charStr;
        if (ch == '"') charStr = "\"\"\"\"";  // Escape quotes
        else if (ch == ',') charStr = "\",\"";  // Escape commas
        else if (ch == '\n') charStr = "\\n";
        else if (ch == '\r') charStr = "\\r";
        else if (ch == '\t') charStr = "\\t";
        else if (isPrintable) charStr = std::string(1, ch);
        else charStr = "\\x" + std::to_string(static_cast<unsigned char>(ch));
        
        file << "\"" << charStr << "\"," << static_cast<int>(ch) << "," 
             << count << "," << std::fixed << std::setprecision(6) << frequency << "," 
             << (isPrintable ? "Yes" : "No") << "\n";
    }
    
    file.close();
    std::cout << "Performance data exported to: " << csvFilename << std::endl;
}


/**
 * Export character distribution for statistical analysis
 */
void exportCharacterDistributionCSV(const std::unordered_map<char, size_t>& charCounts,
                                  size_t totalChars,
                                  const std::string& filename = "") {
    
    std::string csvFilename = filename;
    if (csvFilename.empty()) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        char timestamp[100];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm);
        csvFilename = "char_distribution_" + std::string(timestamp) + ".csv";
    }
    
    std::ofstream file(csvFilename);
    if (!file.is_open()) {
        std::cerr << "Failed to create character distribution CSV file: " << csvFilename << std::endl;
        return;
    }
    
    // Write metadata
    file << "# Character Distribution Analysis\n";
    file << "TotalCharacters," << totalChars << "\n";
    file << "UniqueCharacters," << charCounts.size() << "\n";
    file << "\n";
    
    // Create different views of the data
    
    // 1. Frequency-sorted view
    file << "# Frequency Sorted Data\n";
    file << "Rank,Character,ASCII_Code,Count,Frequency_Percent,Category\n";
    
    std::vector<std::pair<char, size_t>> sortedByFreq(charCounts.begin(), charCounts.end());
    std::sort(sortedByFreq.begin(), sortedByFreq.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int rank = 1;
    for (const auto& pair : sortedByFreq) {
        char ch = pair.first;
        size_t count = pair.second;
        double frequency = (static_cast<double>(count) / totalChars) * 100.0;
        
        std::string category;
        if (ch >= 'A' && ch <= 'Z') category = "Uppercase";
        else if (ch >= 'a' && ch <= 'z') category = "Lowercase";
        else if (ch >= '0' && ch <= '9') category = "Digit";
        else if (ch == ' ') category = "Space";
        else if (ch >= 32 && ch <= 126) category = "Punctuation";
        else if (ch == '\t') category = "Tab";
        else if (ch == '\n') category = "Newline";
        else category = "Control";
        
        std::string charDisplay = (ch >= 32 && ch <= 126) ? std::string(1, ch) : 
                                 ("\\x" + std::to_string(static_cast<unsigned char>(ch)));
        
        file << rank << ",\"" << charDisplay << "\"," << static_cast<int>(ch) << "," 
             << count << "," << std::fixed << std::setprecision(6) << frequency << "," 
             << category << "\n";
        rank++;
    }
    file << "\n";
    
    // 2. ASCII-sorted view for pattern analysis
    file << "# ASCII Sorted Data\n";
    file << "ASCII_Code,Character,Count,Frequency_Percent,Category\n";
    
    std::vector<std::pair<char, size_t>> sortedByASCII(charCounts.begin(), charCounts.end());
    std::sort(sortedByASCII.begin(), sortedByASCII.end(), 
             [](const auto& a, const auto& b) { return a.first < b.first; });
    
    for (const auto& pair : sortedByASCII) {
        char ch = pair.first;
        size_t count = pair.second;
        double frequency = (static_cast<double>(count) / totalChars) * 100.0;
        
        std::string category;
        if (ch >= 'A' && ch <= 'Z') category = "Uppercase";
        else if (ch >= 'a' && ch <= 'z') category = "Lowercase";
        else if (ch >= '0' && ch <= '9') category = "Digit";
        else if (ch == ' ') category = "Space";
        else if (ch >= 32 && ch <= 126) category = "Punctuation";
        else category = "Control";
        
        std::string charDisplay = (ch >= 32 && ch <= 126) ? std::string(1, ch) : 
                                 ("\\x" + std::to_string(static_cast<unsigned char>(ch)));
        
        file << static_cast<int>(ch) << ",\"" << charDisplay << "\"," 
             << count << "," << std::fixed << std::setprecision(6) << frequency << "," 
             << category << "\n";
    }
    
    file.close();
    std::cout << "Character distribution data exported to: " << csvFilename << std::endl;
}

/**
 * Export summary statistics for comparison studies
 */
void exportSummaryStatsCSV(const TestConfiguration& config,
                          const std::vector<double>& executionTimes,
                          const std::unordered_map<char, size_t>& charCounts,
                          const std::string& filename = "") {
    
    std::string csvFilename = filename;
    if (csvFilename.empty()) {
        csvFilename = "simd_summary_stats.csv";
    }
    
    // Check if file exists to determine if we need headers
    bool fileExists = std::ifstream(csvFilename).good();
    
    std::ofstream file(csvFilename, std::ios::app);  // Append mode
    if (!file.is_open()) {
        std::cerr << "Failed to create summary stats CSV file: " << csvFilename << std::endl;
        return;
    }
    
    // Write headers if new file
    if (!fileExists) {
        file << "Timestamp,Implementation,StringLength,Alignment,Repetitions,RandomSeed,";
        file << "TotalChars,UniqueChars,AvgTime_ms,StdDev_ms,MinTime_ms,MaxTime_ms,";
        file << "AvgThroughput_MBps,AvgCharsPerSec,CharDensity_Percent\n";
    }
    
    // Calculate statistics
    double totalTime = std::accumulate(executionTimes.begin(), executionTimes.end(), 0.0);
    double avgTime = totalTime / executionTimes.size();
    
    double variance = 0;
    for (double time : executionTimes) {
        variance += (time - avgTime) * (time - avgTime);
    }
    double stdDev = std::sqrt(variance / executionTimes.size());
    double minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
    double maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
    
    size_t totalChars = config.stringLength - 1;
    double avgThroughput = (config.stringLength / (avgTime / 1000.0)) / (1024.0 * 1024.0);
    double avgCharsPerSec = totalChars / (avgTime / 1000.0);
    double charDensity = static_cast<double>(charCounts.size()) / totalChars * 100.0;
    
    // Generate timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
    
    // Write data row
    file << timestamp << ",SIMD," << config.stringLength << "," << config.alignment << ","
         << config.repetitions << "," << config.randomSeed << "," << totalChars << ","
         << charCounts.size() << "," << std::fixed << std::setprecision(6)
         << avgTime << "," << stdDev << "," << minTime << "," << maxTime << ","
         << avgThroughput << "," << avgCharsPerSec << "," << charDensity << "\n";
    
    file.close();
    std::cout << "Summary statistics appended to: " << csvFilename << std::endl;
}

int main() {
    std::cout << "======================================================" << std::endl;
    std::cout << "   SIMD Character Frequency Analysis                 " << std::endl;
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
    
    std::cout << "\nSIMD character frequency analysis completed successfully!" << std::endl;
    
    return 0;
}
