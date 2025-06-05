#!/usr/bin/env python3
"""
Performance comparison plot between SIMD and Serial implementations
CE-4302 Arquitectura de Computadores II - Taller 02

16, 32, 64, 256, 512, 1024, 2048, 4096 bytes with single sample per test.
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
            'Serial': {'sizes': [], 'times': [], 'throughputs': []},
            'SIMD': {'sizes': [], 'times': [], 'throughputs': []}
        }
        
    def run_single_test(self, executable: str, string_length: int, alignment: int = 16, 
                   target_char: str = 'a') -> Dict:
        """Run a single performance test with one repetition"""
        
        # Single repetition as requested
        repetitions = 1
        input_data = f"{target_char}\n{string_length}\n{alignment}\n{repetitions}\nn\ny\n"
        
        try:
            process = subprocess.Popen(
                [executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1  # Añadir bufsize para evitar el error
            )
            
            stdout, stderr = process.communicate(input=input_data, timeout=30)
            
            if process.returncode != 0:
                print(f"Error running {executable} (return code {process.returncode}): {stderr}")
                return None
                
            # Parse the CSV output from stdout
            lines = stdout.strip().split('\n')
            
            for line in lines:
                if 'StringLength,Alignment,TargetChar,TotalChars,Occurrences,AvgTimeMs' in line:
                    # Next line should be the data
                    idx = lines.index(line)
                    if idx + 1 < len(lines):
                        csv_line = lines[idx + 1]
                        parts = csv_line.split(',')
                        
                        return {
                            'string_length': int(parts[0]),
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
    
    def run_comparison_tests(self, string_sizes: List[int], target_char: str = ';', alignment: int = 16):
        """Run comparison tests for specified string sizes"""
        
        print("=== Performance Comparison: SIMD vs Serial ===")
        print(f"String sizes: {string_sizes}")
        print(f"Target character: '{target_char}'")
        print(f"Memory alignment: {alignment} bytes")
        print(f"Repetitions per test: 1 (single sample)\n")
        
        for size in string_sizes:
            print(f"Testing string length: {size} bytes")
            
            # Test Serial implementation
            print("  Running Serial implementation...")
            serial_result = self.run_single_test(self.serial_executable, size, alignment, target_char)
            
            if serial_result:
                self.results['Serial']['sizes'].append(size)
                self.results['Serial']['times'].append(serial_result['avg_time_ms'])
                self.results['Serial']['throughputs'].append(serial_result['throughput_mbps'])
                print(f"    Serial: {serial_result['avg_time_ms']:.3f} ms, {serial_result['throughput_mbps']:.1f} MB/s")
            else:
                print("    Serial: FAILED")
            
            # Test SIMD implementation
            print("  Running SIMD implementation...")
            simd_result = self.run_single_test(self.simd_executable, size, alignment, target_char)
            
            if simd_result:
                self.results['SIMD']['sizes'].append(size)
                self.results['SIMD']['times'].append(simd_result['avg_time_ms'])
                self.results['SIMD']['throughputs'].append(simd_result['throughput_mbps'])
                print(f"    SIMD:   {simd_result['avg_time_ms']:.3f} ms, {simd_result['throughput_mbps']:.1f} MB/s")
                
                # Calculate speedup if both results available
                if serial_result:
                    speedup = serial_result['avg_time_ms'] / simd_result['avg_time_ms']
                    print(f"    Speedup: {speedup:.2f}x")
            else:
                print("    SIMD: FAILED")
            print()
        # Validación cruzada para el mayor tamaño
        # Validación cruzada para el mayor tamaño
        max_size = max(string_sizes)
        print(f"\n=== Cross-validation for size: {max_size} ===")

        # Obtener la misma cadena para ambas implementaciones
        input_data = f"{target_char}\n{max_size}\n{alignment}\n1\nn\nn\n"

        # Ejecutar serial y capturar resultado
        serial_count = None
        try:
            process = subprocess.Popen(
                [self.serial_executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            stdout, stderr = process.communicate(input=input_data, timeout=30)
            
            if process.returncode == 0:
                # Parsear el conteo serial del output
                for line in stdout.split('\n'):
                    if "Occurrences Found:" in line:
                        serial_count = int(line.split(':')[1].strip())
                        break
        except Exception as e:
            print(f"Error running serial validation: {e}")

        # Ejecutar SIMD con misma cadena
        simd_count = None
        try:
            process = subprocess.Popen(
                [self.simd_executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            stdout, stderr = process.communicate(input=input_data, timeout=30)
            
            if process.returncode == 0:
                # Parsear el conteo SIMD del output
                for line in stdout.split('\n'):
                    if "Occurrences Found:" in line:
                        simd_count = int(line.split(':')[1].strip())
                        break
        except Exception as e:
            print(f"Error running SIMD validation: {e}")

        if serial_count is not None and simd_count is not None:
            if serial_count != simd_count:
                print(f"VALIDATION FAILED! Serial: {serial_count}, SIMD: {simd_count}")
            else:
                print(f"Validation passed! Results match: {serial_count} occurrences")
        else:
            print("Validation incomplete - could not get results from both implementations")

            
    
    def create_performance_plots(self, output_dir: str = "comparison_plots"):
        """Create performance comparison plots"""
        
        if not self.results['Serial']['sizes'] or not self.results['SIMD']['sizes']:
            print("Insufficient data to create plots")
            return
        
        os.makedirs(output_dir, exist_ok=True)
        
        # Create execution time comparison plot
        self._create_execution_time_plot(output_dir)
        
        # Create throughput comparison plot
        self._create_throughput_plot(output_dir)
        
        # Create speedup plot
        self._create_speedup_plot(output_dir)
        
        print(f"All plots saved in: {output_dir}")
    
    def _create_execution_time_plot(self, output_dir: str):
        """Create execution time comparison plot"""
        
        plt.figure(figsize=(12, 8))
        
        # Plot Serial results
        serial_sizes = self.results['Serial']['sizes']
        serial_times = self.results['Serial']['times']
        plt.loglog(serial_sizes, serial_times, 'b-o', label='Serial Implementation', 
                  linewidth=3, markersize=8, markerfacecolor='lightblue', markeredgecolor='blue')
        
        # Plot SIMD results
        simd_sizes = self.results['SIMD']['sizes']
        simd_times = self.results['SIMD']['times']
        plt.loglog(simd_sizes, simd_times, 'r-s', label='SIMD Implementation (SSE4.2)', 
                  linewidth=3, markersize=8, markerfacecolor='lightcoral', markeredgecolor='red')
        
        plt.xlabel('Input Vector Size (bytes)', fontsize=14, fontweight='bold')
        plt.ylabel('Execution Time (ms)', fontsize=14, fontweight='bold')
        plt.title('Performance Comparison: SIMD vs Serial Implementation\n(16-byte Aligned Data)', 
                 fontsize=16, fontweight='bold')
        plt.legend(fontsize=12, loc='upper left')
        plt.grid(True, alpha=0.3, linestyle='--')
        
        # Customize the plot to look more professional
        plt.gca().tick_params(labelsize=12)
        plt.gca().set_facecolor('#f8f9fa')
        
        # Add value annotations
        for i, (size, time) in enumerate(zip(serial_sizes, serial_times)):
            plt.annotate(f'{time:.3f}ms', 
                        xy=(size, time), xytext=(5, 10), 
                        textcoords='offset points', fontsize=9, 
                        ha='left', alpha=0.8, color='blue')
        
        for i, (size, time) in enumerate(zip(simd_sizes, simd_times)):
            plt.annotate(f'{time:.3f}ms', 
                        xy=(size, time), xytext=(5, -15), 
                        textcoords='offset points', fontsize=9, 
                        ha='left', alpha=0.8, color='red')
        
        plt.tight_layout()
        plt.savefig(f"{output_dir}/execution_time_comparison.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Execution time plot saved: {output_dir}/execution_time_comparison.png")
    
    def _create_throughput_plot(self, output_dir: str):
        """Create throughput comparison plot"""
        
        plt.figure(figsize=(12, 8))
        
        # Plot throughput results
        serial_sizes = self.results['Serial']['sizes']
        serial_throughputs = self.results['Serial']['throughputs']
        simd_sizes = self.results['SIMD']['sizes']
        simd_throughputs = self.results['SIMD']['throughputs']
        
        plt.semilogx(serial_sizes, serial_throughputs, 'b-o', label='Serial Implementation', 
                    linewidth=3, markersize=8, markerfacecolor='lightblue', markeredgecolor='blue')
        plt.semilogx(simd_sizes, simd_throughputs, 'r-s', label='SIMD Implementation (SSE4.2)', 
                    linewidth=3, markersize=8, markerfacecolor='lightcoral', markeredgecolor='red')
        
        plt.xlabel('Input Vector Size (bytes)', fontsize=14, fontweight='bold')
        plt.ylabel('Throughput (MB/s)', fontsize=14, fontweight='bold')
        plt.title('Throughput Comparison: SIMD vs Serial Implementation\n(16-byte Aligned Data)', 
                 fontsize=16, fontweight='bold')
        plt.legend(fontsize=12, loc='lower right')
        plt.grid(True, alpha=0.3, linestyle='--')
        
        plt.gca().tick_params(labelsize=12)
        plt.gca().set_facecolor('#f8f9fa')
        
        plt.tight_layout()
        plt.savefig(f"{output_dir}/throughput_comparison.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Throughput plot saved: {output_dir}/throughput_comparison.png")
    
    def _create_speedup_plot(self, output_dir: str):
        """Create speedup comparison plot"""
        
        # Calculate speedups where both implementations have data
        speedup_sizes = []
        speedups = []
        
        serial_data = dict(zip(self.results['Serial']['sizes'], self.results['Serial']['times']))
        simd_data = dict(zip(self.results['SIMD']['sizes'], self.results['SIMD']['times']))
        
        for size in serial_data:
            if size in simd_data:
                speedup = serial_data[size] / simd_data[size]
                speedup_sizes.append(size)
                speedups.append(speedup)
        
        if not speedups:
            print("No speedup data available")
            return
        
        plt.figure(figsize=(12, 8))
        
        # Create bar plot for speedup
        x_pos = np.arange(len(speedup_sizes))
        bars = plt.bar(x_pos, speedups, color='green', alpha=0.7, edgecolor='darkgreen', linewidth=2)
        
        # Add horizontal line at y=1 (no speedup)
        plt.axhline(y=1, color='black', linestyle='--', alpha=0.8, linewidth=2, label='No speedup')
        
        # Customize the plot
        plt.xlabel('Input Vector Size (bytes)', fontsize=14, fontweight='bold')
        plt.ylabel('Speedup (Serial Time / SIMD Time)', fontsize=14, fontweight='bold')
        plt.title('SIMD Speedup over Serial Implementation\n(Single Sample Measurements)', 
                 fontsize=16, fontweight='bold')
        plt.xticks(x_pos, [f'{size}' for size in speedup_sizes], fontsize=12)
        plt.gca().tick_params(labelsize=12)
        plt.grid(True, alpha=0.3, linestyle='--', axis='y')
        plt.legend(fontsize=12)
        
        # Add value labels on bars
        for i, (bar, speedup) in enumerate(zip(bars, speedups)):
            height = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2., height + 0.05,
                    f'{speedup:.2f}x', ha='center', va='bottom', fontsize=11, fontweight='bold')
        
        plt.gca().set_facecolor('#f8f9fa')
        plt.tight_layout()
        plt.savefig(f"{output_dir}/speedup_comparison.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Speedup plot saved: {output_dir}/speedup_comparison.png")
    
    def print_summary_table(self):
        """Print a summary table of results"""
        
        print("\n" + "="*80)
        print("PERFORMANCE SUMMARY TABLE")
        print("="*80)
        print(f"{'Size (bytes)':<12} {'Serial (ms)':<12} {'SIMD (ms)':<12} {'Speedup':<10} {'SIMD Throughput':<15}")
        print("-"*80)
        
        serial_data = dict(zip(self.results['Serial']['sizes'], self.results['Serial']['times']))
        simd_data = dict(zip(self.results['SIMD']['sizes'], self.results['SIMD']['times']))
        simd_throughput = dict(zip(self.results['SIMD']['sizes'], self.results['SIMD']['throughputs']))
        
        for size in sorted(set(serial_data.keys()) & set(simd_data.keys())):
            serial_time = serial_data[size]
            simd_time = simd_data[size]
            speedup = serial_time / simd_time
            throughput = simd_throughput[size]
            
            print(f"{size:<12} {serial_time:<12.3f} {simd_time:<12.3f} {speedup:<10.2f} {throughput:<15.1f}")
        
        print("="*80)

def main():
    # Tamaños válidos (mínimo 16 bytes según tu implementación)
    STRING_SIZES = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384]
    ALIGNMENTS = [16, 64]
    TARGET_CHAR = ';'  # Definir la variable que faltaba
    
    # Check if executables exist
    if not os.path.exists("./char_count_serial"):
        print("Error: ./char_count_serial not found. Please compile first with 'make'")
        sys.exit(1)
    
    if not os.path.exists("./char_count_simd"):
        print("Error: ./char_count_simd not found. Please compile first with 'make'")
        sys.exit(1)
    
    for alignment in ALIGNMENTS:
        print(f"\n=== Testing with alignment: {alignment} bytes ===")
        comparison = PerformanceComparison()
        comparison.run_comparison_tests(STRING_SIZES, TARGET_CHAR, alignment)
        comparison.create_performance_plots(f"comparison_plots_align{alignment}")
        comparison.print_summary_table()
    
    print("\nPerformance comparison completed!")


if __name__ == "__main__":
    main()
