import matplotlib.pyplot as plt
import numpy as np
import sys
import re
import matplotlib
from matplotlib.ticker import MultipleLocator, AutoMinorLocator

# Set fonts
matplotlib.rcParams['font.sans-serif'] = ['DejaVu Sans', 'Arial', 'Helvetica']
matplotlib.rcParams['axes.unicode_minus'] = False

def read_data_from_file(filename):
    """Read all numbers from file, regardless of formatting"""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Extract all numbers using regex
    numbers = list(map(int, re.findall(r'-?\d+', content)))
    
    if len(numbers) < 1:
        raise ValueError("File must contain at least one number (n)")
    
    # First number is n
    n = numbers[0]
    
    # Next n numbers are a_0 to a_{n-1}
    if len(numbers) < n + 1:
        raise ValueError(f"File should contain n+1 numbers, but only found {len(numbers)}")
    
    a_list = numbers[1:n+1]
    
    return n, a_list

def calculate_y_axis_limit(y_values):
    """Intelligently calculate y-axis limits to avoid extreme values affecting visualization"""
    if not y_values:
        return 0, 100
    
    # Sort values
    sorted_values = np.sort(y_values)
    
    # Calculate percentiles
    p95 = np.percentile(sorted_values, 95)
    p99 = np.percentile(sorted_values, 99)
    max_value = max(y_values)
    
    # If maximum value is much larger than 99th percentile, use 99th percentile as y-axis upper limit
    if max_value > p99 * 10:
        # Set upper limit to 1.2 times the 99th percentile
        y_max = p99 * 1.2
        return 0, y_max, True, max_value
    else:
        # Use maximum value as upper limit
        y_max = max_value * 1.1
        return 0, y_max, False, max_value

def auto_adjust_x_ticks(x_values):
    """Automatically adjust x-axis ticks to avoid label overcrowding"""
    if not x_values:
        return x_values, []
    
    n_ticks = len(x_values)
    
    # If too many x values, automatically select tick spacing
    if n_ticks > 50:
        # Calculate appropriate spacing
        range_size = max(x_values) - min(x_values)
        
        # Select appropriate spacing based on range size
        if range_size <= 100:
            step = max(1, int(range_size / 20))
            step = find_nice_step(step)
        elif range_size <= 500:
            step = 10
        elif range_size <= 1000:
            step = 20
        elif range_size <= 5000:
            step = 50
        elif range_size <= 10000:
            step = 100
        else:
            step = 200
        
        # Generate sparse ticks
        start = min(x_values)
        end = max(x_values)
        tick_positions = []
        tick_labels = []
        
        # Start from the first number divisible by step
        first_tick = start + ((step - (start % step)) % step)
        
        for i in range(first_tick, end+1, step):
            if i in x_values:
                tick_positions.append(i)
                tick_labels.append(str(i))
        
        # Ensure at least 5 ticks
        if len(tick_positions) < 5 and n_ticks > 5:
            # Force display some ticks
            indices = np.linspace(0, len(x_values)-1, min(10, n_ticks), dtype=int)
            tick_positions = [x_values[i] for i in indices]
            tick_labels = [str(x_values[i]) for i in indices]
    else:
        tick_positions = x_values
        tick_labels = [str(x) for x in x_values]
    
    return tick_positions, tick_labels

def find_nice_step(step):
    """Find a visually pleasing tick spacing (multiples of 10, 100, etc.)"""
    nice_steps = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000]
    
    for nice_step in nice_steps:
        if nice_step >= step:
            return nice_step
    
    return 1000

def plot_histogram_from_file_with_manual_input(filename):
    """Read n and a_i from file, manually input s, a, b"""
    try:
        # Read n and a_i from file
        n, a_list = read_data_from_file(filename)
        
        print(f"Successfully read from file: n={n}")
        print(f"First 5 a_i values: {a_list[:5]}")
        if len(a_list) > 5:
            print(f"... (total {len(a_list)} values)")
        
        # Manually input s, a, b
        print("\nEnter s, a, b (space separated):")
        s, a_interval, b_interval = map(int, input().split())
        
        # Generate x-axis labels and corresponding values
        x_values = [s + i for i in range(n)]
        y_values = a_list
        
        # Calculate mean and standard error
        total_samples = sum(y_values)
        
        # Calculate weighted mean (considering sample count for each value)
        weighted_sum = sum(x * y for x, y in zip(x_values, y_values))
        mean = weighted_sum / total_samples if total_samples > 0 else 0
        
        # Calculate standard deviation and standard error
        if total_samples > 1:
            variance = sum(y * (x - mean) ** 2 for x, y in zip(x_values, y_values)) / total_samples
            std_dev = np.sqrt(variance)
            std_error = std_dev / np.sqrt(total_samples)
        else:
            std_error = 0
        
        # Filter data within [a, b] interval
        filtered_x = []
        filtered_y = []
        for x, y in zip(x_values, y_values):
            if a_interval <= x <= b_interval:
                filtered_x.append(x)
                filtered_y.append(y)
        
        if not filtered_x:
            print(f"No data in interval [{a_interval}, {b_interval}]")
            return
        
        # Intelligently calculate y-axis limits
        y_min, y_max, has_extreme, max_value = calculate_y_axis_limit(filtered_y)
        
        # Create figure with adjusted size for better layout
        plt.figure(figsize=(14, 8))
        
        # Plot histogram (without value labels on top of bars)
        bars = plt.bar(filtered_x, filtered_y, width=0.8, 
                       color='skyblue', edgecolor='black', alpha=0.7)
        
        # Set plot properties (all in English)
        plt.title(f'Histogram (Interval [{a_interval}, {b_interval}])', fontsize=14, fontweight='bold', pad=20)
        plt.xlabel('X Value', fontsize=12)
        plt.ylabel('Sample Count', fontsize=12)
        plt.grid(True, alpha=0.3, linestyle='--')
        
        # Set y-axis limits
        plt.ylim(y_min, y_max)
        
        # Automatically adjust x-axis ticks
        tick_positions, tick_labels = auto_adjust_x_ticks(filtered_x)
        plt.xticks(tick_positions, tick_labels, fontsize=9, rotation=45)
        
        # Add mean and standard error information (in English)
        info_text = f'File: {filename}\n'
        info_text += f'n = {n}\n'
        info_text += f'Total samples: {total_samples}\n'
        info_text += f'Weighted mean: {mean:.4f}\n'
        info_text += f'Standard error: {std_error:.4f}\n'
        info_text += f'Display interval: [{a_interval}, {b_interval}]'
        if has_extreme:
            info_text += f'\nNote: y-axis truncated, max value = {max_value}'
        
        # Add text box to plot
        plt.text(0.02, 0.98, info_text, transform=plt.gca().transAxes,
                 fontsize=10, verticalalignment='top',
                 bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
        
        # Adjust layout with extra padding at top for title
        plt.tight_layout()
        
        # Save image (optional)
        save_option = input("\nSave image to file? (y/n): ")
        if save_option.lower() == 'y':
            output_filename = input("Enter output filename (e.g., histogram.png): ")
            plt.savefig(output_filename, dpi=300, bbox_inches='tight')
            print(f"Image saved to {output_filename}")
        
        # Display graph
        plt.show()
        
        # Print statistics
        print(f"\nStatistics:")
        print(f"File: {filename}")
        print(f"n = {n}")
        print(f"Total samples: {total_samples}")
        print(f"Weighted mean: {mean:.4f}")
        print(f"Standard error: {std_error:.4f}")
        print(f"Displayed data points: {len(filtered_x)}")
        if has_extreme:
            print(f"Note: y-axis truncated, maximum value = {max_value}")
        
    except Exception as e:
        print(f"Error: {e}")
        print("Please check file format or input values")

def main():
    # Check command line arguments
    if len(sys.argv) != 2:
        print("Usage: python script.py <input_filename>")
        print("File format: Any format, only numbers needed")
        print("             First number is n, next n numbers are a_0 to a_{n-1}")
        return
    
    filename = sys.argv[1]
    plot_histogram_from_file_with_manual_input(filename)

if __name__ == "__main__":
    main()