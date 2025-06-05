#!/usr/bin/env python3
"""
CE-4302 Arquitectura de Computadores II - Taller 02
"""
import subprocess
import matplotlib.pyplot as plt
import numpy as np
import os
import sys
from typing import Dict, List

class PerformanceComparison:
    def __init__(self, serial_executable="./char_count_serial", simd_executable="./char_count_simd"):
        self.serial_executable = serial_executable
        self.simd_executable = simd_executable
        self.results = {
            'Serial_16': {'sizes': [], 'times': [], 'throughputs': []},
            'SIMD_16': {'sizes': [], 'times': [], 'throughputs': []},
            'SIMD_32': {'sizes': [], 'times': [], 'throughputs': []},
            'SIMD_unaligned': {'sizes': [], 'times': [], 'throughputs': []}}
        
    def run_single_test(self, executable: str, string_length: int, alignment: int = 16, 
                   target_char: str = ';', repetitions: int = 100) -> Dict:
        """Run a single performance test with multiple repetitions for better timing resolution"""
        
        input_data = f"{target_char}\n{string_length}\n{alignment}\n{repetitions}\nn\ny\n"
        
        try:
            process = subprocess.Popen(
                [executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            stdout, stderr = process.communicate(input=input_data, timeout=60)
            
            if process.returncode != 0:
                print(f"Error running {executable} (return code {process.returncode}): {stderr}")
                return None
                
            # Parse the CSV output from stdout
            lines = stdout.strip().split('\n')
            
            for line in lines:
                if 'StringLength,Alignment,TargetChar,TotalChars,Occurrences,AvgTimeMs' in line:
                    idx = lines.index(line)
                    if idx + 1 < len(lines):
                        csv_line = lines[idx + 1]
                        parts = csv_line.split(',')
                        
                        return {
                            'string_length': int(parts[0]),
                            'alignment': int(parts[1]),  # Add alignment to return dict
                            'avg_time_ms': float(parts[5]),
                            'throughput_mbps': float(parts[9])
                        }
            
            print(f"Could not parse output from {executable}")
            return None
            
        except subprocess.TimeoutExpired:
            print(f"Timeout running {executable}")
            process.kill()
            return None
        except Exception as e:
            print(f"Error running {executable}: {e}")
            return None
    
    def run_comparison_tests(self, string_sizes: List[int], target_char: str = ';'):
        """Run comparison tests for specified string sizes"""
        
        print("=== Performance Comparison ===")
        print(f"String sizes: {string_sizes}")
        print(f"Target character: '{target_char}'")
        print("Testing: Serial, SIMD (16B aligned), SIMD (unaligned)\n")
        
        for size in string_sizes:
            print(f"Testing string length: {size} bytes")
            
            # Adjust repetitions based on size for better timing resolution
            if size <= 1024:
                repetitions = 1000
            elif size <= 8192:
                repetitions = 500
            else:
                repetitions = 100
            
            print(f"  Using {repetitions} repetitions for this size")
            
            # Test Serial implementation (16-byte aligned)
            print("  Running Serial implementation...")
            serial_result = self.run_single_test(self.serial_executable, size, 16, target_char, repetitions)
            
            if serial_result:
                self.results['Serial_16']['sizes'].append(size)
                self.results['Serial_16']['times'].append(serial_result['avg_time_ms'])
                self.results['Serial_16']['throughputs'].append(serial_result['throughput_mbps'])
                print(f"    Serial (16B): {serial_result['avg_time_ms']:.6f} ms")
            
            # Test SIMD implementation (16-byte aligned)
            print("  Running SIMD implementation (16B aligned)...")
            simd_result = self.run_single_test(self.simd_executable, size, 16, target_char, repetitions)
            
            if simd_result:
                self.results['SIMD_16']['sizes'].append(size)
                self.results['SIMD_16']['times'].append(simd_result['avg_time_ms'])
                self.results['SIMD_16']['throughputs'].append(simd_result['throughput_mbps'])
                print(f"    SIMD (16B):   {simd_result['avg_time_ms']:.6f} ms")
            
            # Test SIMD implementation (unaligned)
            print("  Running SIMD implementation (unaligned)...")
            simd_unaligned_result = self.run_single_test(self.simd_executable, size, 1, target_char, repetitions)
            
            if simd_unaligned_result:
                self.results['SIMD_unaligned']['sizes'].append(size)
                self.results['SIMD_unaligned']['times'].append(simd_unaligned_result['avg_time_ms'])
                self.results['SIMD_unaligned']['throughputs'].append(simd_unaligned_result['throughput_mbps'])
                print(f"    SIMD (unaligned): {simd_unaligned_result['avg_time_ms']:.6f} ms")
            
            # Test SIMD implementation (32-byte aligned)
            print("  Running SIMD implementation (32B aligned)...")
            simd_32_result = self.run_single_test(self.simd_executable, size, 32, target_char, repetitions)

            if simd_32_result:
                self.results['SIMD_32']['sizes'].append(size)
                self.results['SIMD_32']['times'].append(simd_32_result['avg_time_ms'])
                self.results['SIMD_32']['throughputs'].append(simd_32_result['throughput_mbps'])
                print(f"    SIMD (32B):   {simd_32_result['avg_time_ms']:.6f} ms")
            
            print()
    
    def create_normalized_time_plot(self, output_dir: str = "comparison_plots"):
        """Create a single plot comparing normalized execution times"""
        
        # Verify we have data for all configurations
        required_configs = ['Serial_16', 'SIMD_16', 'SIMD_32', 'SIMD_unaligned']
        for config in required_configs:
            if not self.results[config]['sizes']:
                print(f"Missing data for {config}")
                return
        
        os.makedirs(output_dir, exist_ok=True)
        
        plt.figure(figsize=(12, 8))
        
        # Get common sizes where all implementations have data
        common_sizes = sorted(set(
            self.results['Serial_16']['sizes'] +
            self.results['SIMD_16']['sizes'] +
            self.results['SIMD_32']['sizes'] +
            self.results['SIMD_unaligned']['sizes']
        ))
        
        if not common_sizes:
            print("No common sizes found for all implementations")
            return
        
        # Normalize times relative to serial implementation
        max_serial_time = max(self.results['Serial_16']['times'])
        normalized_serial = [t/max_serial_time for t in self.results['Serial_16']['times']]
        normalized_simd_16 = [t/max_serial_time for t in self.results['SIMD_16']['times']]
        normalized_simd_32 = [t/max_serial_time for t in self.results['SIMD_32']['times']]
        normalized_simd_unaligned = [t/max_serial_time for t in self.results['SIMD_unaligned']['times']]
        
        # Plot results with distinct styles
        plt.loglog(common_sizes, normalized_serial, 'b-o', label='Serial (16B aligned)', 
                linewidth=3, markersize=8, markerfacecolor='lightblue', markeredgecolor='blue')
        
        plt.loglog(common_sizes, normalized_simd_16, 'r-s', label='SIMD (16B aligned)', 
                linewidth=3, markersize=8, markerfacecolor='lightcoral', markeredgecolor='red')
        
        plt.loglog(common_sizes, normalized_simd_32, 'm-D', label='SIMD (32B aligned)', 
                linewidth=3, markersize=8, markerfacecolor='violet', markeredgecolor='purple')
        
        plt.loglog(common_sizes, normalized_simd_unaligned, 'g-^', label='SIMD (unaligned)', 
                linewidth=3, markersize=8, markerfacecolor='lightgreen', markeredgecolor='green')
        
        plt.xlabel('Input Vector Size (bytes)', fontsize=14, fontweight='bold')
        plt.ylabel('Normalized Execution Time (Serial max = 1.0)', fontsize=14, fontweight='bold')
        plt.title('Performance Comparison with data alignment\n(Serial vs SIMD)', 
                fontsize=16, fontweight='bold')
        
        # Set custom X-axis ticks
        plt.xticks(common_sizes, [str(size) for size in common_sizes], rotation=45)
        plt.xscale('log', base=2)
        plt.gca().get_xaxis().set_major_formatter(plt.ScalarFormatter())
        
        plt.legend(fontsize=12, loc='upper left')
        plt.grid(True, alpha=0.3, linestyle='--')
        
        # Customize the plot
        plt.gca().tick_params(labelsize=12)
        plt.gca().set_facecolor('#f8f9fa')
        
        plt.tight_layout()
        plt.savefig(f"{output_dir}/normalized_time_comparison.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Normalized execution time plot saved: {output_dir}/normalized_time_comparison.png")

def main():
    # Fixed string sizes as specified
    STRING_SIZES = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536]
    TARGET_CHAR = ';'
    
    # Check if executables exist
    if not os.path.exists("./char_count_serial"):
        print("Error: ./char_count_serial not found. Please compile first with 'make'")
        sys.exit(1)
    
    if not os.path.exists("./char_count_simd"):
        print("Error: ./char_count_simd not found. Please compile first with 'make'")
        sys.exit(1)
    
    comparison = PerformanceComparison()
    comparison.run_comparison_tests(STRING_SIZES, TARGET_CHAR)
    comparison.create_normalized_time_plot()
    
    print("\nPerformance comparison completed! :3")

if __name__ == "__main__":
    main()
