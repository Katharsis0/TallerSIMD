#include "utils.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include <numeric>

// PerformanceMetrics implementation
void PerformanceMetrics::print() const {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== Performance Metrics ===" << std::endl;
    std::cout << "Target Character: '" << targetCharacter << "' (ASCII: " << static_cast<int>(targetCharacter) << ")" << std::endl;
    std::cout << "String Length: " << stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << alignment << " bytes" << std::endl;
    std::cout << "Total Characters: " << totalCharacters << std::endl;
    std::cout << "Occurrences Found: " << occurrences << std::endl;
    std::cout << "Execution Time: " << executionTimeMs << " ms" << std::endl;
    std::cout << "Memory Used: " << memoryUsedBytes << " bytes" << std::endl;
    std::cout << "Throughput: " << getThroughputMBps() << " MB/s" << std::endl;
    std::cout << "Characters/sec: " << getCharactersPerSecond() << std::endl;
    std::cout << "=========================" << std::endl;
}

void PerformanceMetrics::printCSVHeader() const {
    std::cout << "StringLength,Alignment,TargetChar,TotalChars,Occurrences,ExecutionTimeMs,ThroughputMBps,CharsPerSecond" << std::endl;
}

void PerformanceMetrics::printCSVRow() const {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << stringLength << "," << alignment << "," << targetCharacter << "," << totalCharacters << "," 
              << occurrences << "," << executionTimeMs << "," 
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
RandomStringGenerator::RandomStringGenerator(uint32_t seed) : rng(seed), seed(seed) {}

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
    std::uniform_int_distribution<int> dist1(0x20, 0x7E); // ASCII básico
    std::uniform_int_distribution<int> dist2(0xC2, 0xF4); // Inicios de secuencia UTF-8
    
    for (size_t i = 0; i < length - 1; ) {
        if (dist1(rng) % 4 == 0) { // 25% de probabilidad de caracter no ASCII
            int lead = dist2(rng);
            int charSize = 1;
            
            if (lead < 0xE0) charSize = 2;
            else if (lead < 0xF0) charSize = 3;
            else charSize = 4;
            
            if (i + charSize >= length - 1) break;
            
            buffer[i++] = static_cast<char>(lead);
            for (int j = 1; j < charSize; j++) {
                buffer[i++] = static_cast<char>(0x80 + (dist1(rng) % 0x40));
            }
        } else {
            buffer[i++] = static_cast<char>(dist1(rng));
        }
    }
    buffer[length - 1] = '\0';
}

// HighPrecisionTimer implementation
std::vector<double> HighPrecisionTimer::measureExecutionTimes(
    std::function<size_t()> operation, 
    int repetitions, 
    int warmup_runs) {
    
    std::vector<double> times;
    times.reserve(repetitions);
    
    // Warmup runs to stabilize cache and CPU
    for (int i = 0; i < warmup_runs; ++i) {
        operation();
    }
    
    for (int i = 0; i < repetitions; ++i) {
        // Usar alta precisión temporal
        auto start = std::chrono::high_resolution_clock::now();
        
        // Ejecutar la operación
        operation();
        
        auto end = std::chrono::high_resolution_clock::now();
        
        // Convertir a microsegundos para mejor precisión
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double time_ms = duration.count() / 1000000.0; // Convertir a ms
        
        times.push_back(time_ms);
    }
    
    return times;
}

double HighPrecisionTimer::calculateMedian(std::vector<double> times) {
    std::sort(times.begin(), times.end());
    size_t n = times.size();
    if (n % 2 == 0) {
        return (times[n/2 - 1] + times[n/2]) / 2.0;
    } else {
        return times[n/2];
    }
}

std::pair<double, double> HighPrecisionTimer::removeOutliers(const std::vector<double>& times) {
    if (times.size() < 3) {
        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        return std::make_pair(sum / times.size(), 0.0);
    }
    
    std::vector<double> sorted_times = times;
    std::sort(sorted_times.begin(), sorted_times.end());
    
    // Remover el 10% superior e inferior para eliminar outliers
    size_t remove_count = std::max(1UL, times.size() / 10);
    std::vector<double> trimmed(
        sorted_times.begin() + remove_count, 
        sorted_times.end() - remove_count
    );
    
    double sum = std::accumulate(trimmed.begin(), trimmed.end(), 0.0);
    double mean = sum / trimmed.size();
    
    // Calcular desviación estándar
    double variance = 0;
    for (double time : trimmed) {
        variance += (time - mean) * (time - mean);
    }
    double stddev = std::sqrt(variance / trimmed.size());
    
    return std::make_pair(mean, stddev);
}

// Utility functions
TestConfiguration getUserConfiguration() {
    TestConfiguration config;
    
    std::cout << "\n=== Character Occurrence Counting Configuration ===" << std::endl;
    
    // Get target character
    std::cout << "Enter the character to search for: ";
    std::cin >> config.targetCharacter;
    
    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        throw std::invalid_argument("Invalid character input");
    }
    
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
    
    // Ask for detailed results
    char showDetailed;
    std::cout << "Show detailed results? (y/n): ";
    std::cin >> showDetailed;
    config.showDetailedResults = (showDetailed == 'y' || showDetailed == 'Y');
    
    // Ask for CSV export
    char exportCSV;
    std::cout << "Export results to CSV format? (y/n): ";
    std::cin >> exportCSV;
    config.exportCSV = (exportCSV == 'y' || exportCSV == 'Y');
    
    // Set deterministic seed for reproducible results
    config.randomSeed = 42;
    
    std::cout << "Using deterministic seed: " << config.randomSeed << " (for reproducible results)" << std::endl;
    std::cout << "Target character: '" << config.targetCharacter << "' (ASCII: " << static_cast<int>(config.targetCharacter) << ")" << std::endl;
    
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
    
    // Validate target character (should be printable ASCII for this workshop)
    if (config.targetCharacter < 32 || config.targetCharacter > 126) {
        std::cout << "Warning: Target character is not printable ASCII. Results may vary." << std::endl;
    }
}

bool isPowerOfTwo(size_t value) {
    return value > 0 && (value & (value - 1)) == 0;
}

bool validateResults(size_t serialCount, size_t simdCount, const char* str, size_t length, char) {
    if (serialCount != simdCount) {
        std::cerr << "Validation failed! Serial: " << serialCount 
                  << " SIMD: " << simdCount << std::endl;
        
        // Mostrar fragmento problemático
        size_t start = std::max(0, static_cast<int>(length/2 - 10));
        std::cerr << "String fragment: " << std::string(str + start, 20) << std::endl;
        return false;
    }
    return true;
}

/**
 * Display character occurrence results in a readable format
 */
void displayCharacterOccurrences(char targetChar, size_t occurrences, size_t totalChars) {
    std::cout << "\n=== Character Occurrence Analysis ===" << std::endl;
    
    // Handle special characters for display
    std::string charDisplay;
    if (targetChar == ' ') charDisplay = "SPACE";
    else if (targetChar == '\t') charDisplay = "TAB";
    else if (targetChar == '\n') charDisplay = "NEWLINE";
    else if (targetChar >= 32 && targetChar <= 126) charDisplay = std::string(1, targetChar);
    else charDisplay = "CTRL";
    
    double frequency = totalChars > 0 ? (static_cast<double>(occurrences) / totalChars) * 100.0 : 0.0;
    
    std::cout << "Target Character: " << charDisplay << " (ASCII: " << static_cast<int>(targetChar) << ")" << std::endl;
    std::cout << "Total Characters Analyzed: " << totalChars << std::endl;
    std::cout << "Occurrences Found: " << occurrences << std::endl;
    std::cout << "Frequency: " << std::fixed << std::setprecision(6) << frequency << "%" << std::endl;
    std::cout << "====================================" << std::endl;
}

/**
 * Export results to CSV format
 */
void exportResultsCSV(char targetChar, size_t occurrences, size_t totalChars, 
                     const std::vector<double>& executionTimes, const TestConfiguration& config,
                     const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to create CSV file: " << filename << std::endl;
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
    
    double avgThroughput = (config.stringLength / (avgTime / 1000.0)) / (1024.0 * 1024.0);
    double avgCharsPerSec = totalChars / (avgTime / 1000.0);
    double frequency = totalChars > 0 ? (static_cast<double>(occurrences) / totalChars) * 100.0 : 0.0;
    
    // Determine implementation type based on filename
    std::string implType = (filename.find("simd") != std::string::npos) ? "SIMD-SSE4.2" : "Serial";
    
    // Write metadata and summary
    file << "# " << implType << " Character Occurrence Counting Results\n";
    file << "# Configuration\n";
    file << "Implementation," << implType << "\n";
    file << "TargetCharacter," << targetChar << "\n";
    file << "TargetCharacterASCII," << static_cast<int>(targetChar) << "\n";
    file << "StringLength," << config.stringLength << "\n";
    file << "Alignment," << config.alignment << "\n";
    file << "Repetitions," << config.repetitions << "\n";
    file << "RandomSeed," << config.randomSeed << "\n";
    file << "TotalCharacters," << totalChars << "\n";
    file << "Occurrences," << occurrences << "\n";
    file << "Frequency," << std::fixed << std::setprecision(6) << frequency << "\n";
    file << "\n";
    
    // Performance summary
    file << "# Performance Summary\n";
    file << "Metric,Value,Unit\n";
    file << "AvgExecutionTime," << avgTime << ",ms\n";
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
    
    file.close();
    std::cout << "Results exported to: " << filename << std::endl;
}

// Función mejorada para análisis de rendimiento
void runImprovedPerformanceAnalysis(CharacterCounterBase& counter, const TestConfiguration& config) {
    std::cout << "\n=== Improved Performance Analysis ===" << std::endl;
    std::cout << "Implementation: " << counter.getImplementationName() << std::endl;
    std::cout << "Target Character: '" << config.targetCharacter << "' (ASCII: " << static_cast<int>(config.targetCharacter) << ")" << std::endl;
    std::cout << "String Length: " << config.stringLength << " bytes" << std::endl;
    std::cout << "Memory Alignment: " << config.alignment << " bytes" << std::endl;
    std::cout << "Repetitions: " << config.repetitions << std::endl;
    
    RandomStringGenerator generator(config.randomSeed);
    
    try {
        // Generate aligned string
        std::cout << "\nGenerating deterministic random string..." << std::endl;
        void* aligned = generator.generateAlignedString(config.stringLength, config.alignment);
        char* str = static_cast<char*>(aligned);
        
        std::cout << "Searching for character '" << config.targetCharacter << "'..." << std::endl;
        
        // Usar el timer mejorado
        size_t totalOccurrences = 0;
        auto operation = [&]() -> size_t {
            PerformanceMetrics metrics;
            return counter.countCharacterOccurrences(str, config.stringLength, config.targetCharacter, metrics);
        };
        
        // Medir con alta precisión
        std::vector<double> executionTimes = HighPrecisionTimer::measureExecutionTimes(
            operation, config.repetitions, 10); // 10 warmup runs
        
        // Obtener el número de ocurrencias de la primera ejecución medida
        PerformanceMetrics finalMetrics;
        totalOccurrences = counter.countCharacterOccurrences(
            str, config.stringLength, config.targetCharacter, finalMetrics);
        
        // Análisis estadístico mejorado - compatible con C++14
        std::pair<double, double> result = HighPrecisionTimer::removeOutliers(executionTimes);
        double avgTime = result.first;
        double stdDev = result.second;
        
        double medianTime = HighPrecisionTimer::calculateMedian(executionTimes);
        double minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
        double maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
        
        // Calculate derived metrics
        size_t totalChars = config.stringLength - 1; // Exclude null terminator
        double avgThroughput = (config.stringLength / (avgTime / 1000.0)) / (1024.0 * 1024.0);
        double avgCharsPerSec = totalChars / (avgTime / 1000.0);
        
        // Display results
        displayCharacterOccurrences(config.targetCharacter, totalOccurrences, totalChars);
        
        std::cout << "\n=== Improved Performance Results ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Average Execution Time: " << avgTime << " ms" << std::endl;
        std::cout << "Median Execution Time: " << medianTime << " ms" << std::endl;
        std::cout << "Standard Deviation: " << stdDev << " ms" << std::endl;
        std::cout << "Min Execution Time: " << minTime << " ms" << std::endl;
        std::cout << "Max Execution Time: " << maxTime << " ms" << std::endl;
        std::cout << "Average Throughput: " << avgThroughput << " MB/s" << std::endl;
        std::cout << "Characters per Second: " << avgCharsPerSec << std::endl;
        std::cout << "Coefficient of Variation: " << (stdDev / avgTime * 100.0) << "%" << std::endl;
        
        // Memory alignment verification
        std::cout << "\n=== Memory Alignment Verification ===" << std::endl;
        uintptr_t address = reinterpret_cast<uintptr_t>(aligned);
        std::cout << "Memory Address: 0x" << std::hex << address << std::dec << std::endl;
        std::cout << "Alignment Check: " << (address % config.alignment == 0 ? "PASSED" : "FAILED") << std::endl;
        std::cout << "Address modulo alignment: " << (address % config.alignment) << std::endl;
        
        // CSV output usando tiempo promedio sin outliers
        if (config.exportCSV) {
            std::cout << "\n=== CSV Export ===" << std::endl;
            std::cout << "StringLength,Alignment,TargetChar,TotalChars,Occurrences,AvgTimeMs,StdDevMs,MinTimeMs,MaxTimeMs,ThroughputMBps,CharsPerSec" << std::endl;
            std::cout << config.stringLength << "," << config.alignment << "," << config.targetCharacter << "," << totalChars << "," 
                      << totalOccurrences << "," << avgTime << "," << stdDev << "," << minTime << "," << maxTime << "," 
                      << avgThroughput << "," << avgCharsPerSec << std::endl;
            
            // Usar las funciones de exportación existentes con los tiempos mejorados
            exportResultsCSV(config.targetCharacter, totalOccurrences, totalChars, executionTimes, config, 
                           counter.getImplementationName() == "Serial" ? "serial_results.csv" : "simd_results.csv");
        }
        
        generator.freeAlignedString(aligned);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during performance analysis: " << e.what() << std::endl;
        throw;
    }
}