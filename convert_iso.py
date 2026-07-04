import csv
import sys
import os
import re
import tempfile
from datetime import datetime, timezone

ISO_8601_RE = re.compile(
    r'^\d{4}-\d{2}-\d{2}'
    r'T'
    r'\d{2}:\d{2}:\d{2}'
    r'(?:\.\d+)?'
    r'(?:Z|[+-]\d{2}:\d{2})$'
)

def convert_to_iso_inplace(filepath):
    """
    CSV parsing library used by the engine works with ISO 8601 timestamps.
    This python script converts the "datetime" column to iSO 8601 *in-place*!
    """
    print(f"Processing {filepath}...")
    
    # Create a temporary file to safely write data without corrupting the original
    temp_file = tempfile.NamedTemporaryFile(mode='w', delete=False, newline='')
    
    converted_count = 0
    
    try:
        with open(filepath, 'r', newline='') as infile, temp_file:
            reader = csv.DictReader(infile)
            
            if 'datetime' not in reader.fieldnames:
                print("Error: Could not find a 'datetime' column in the CSV.")
                sys.exit(1)
                
            writer = csv.DictWriter(temp_file, fieldnames=reader.fieldnames)
            writer.writeheader()
            
            for row in reader:
                dt_str = row['datetime']
                
                # Check if it already has the 'Z' (skip if already ISO)
                if not ISO_8601_RE.fullmatch(dt_str):
                    try:
                        # Parse format: '2008-01-02 06:00:00'
                        dt_obj = datetime.strptime(dt_str, '%Y-%m-%d %H:%M:%S')

                        # Convert to strict ISO 8601 UTC: '2008-01-02T06:00:00Z'
                        row['datetime'] = dt_obj.strftime('%Y-%m-%dT%H:%M:%SZ')
                        converted_count += 1

                    except ValueError:
                        try:
                            # Parse format with timezone offset:
                            # '2025-12-15 00:00:00-05:00'
                            dt_obj = datetime.strptime(dt_str, '%Y-%m-%d %H:%M:%S%z')

                            # Convert offset-aware datetime to UTC
                            dt_obj_utc = dt_obj.astimezone(timezone.utc)

                            row['datetime'] = dt_obj_utc.strftime('%Y-%m-%dT%H:%M:%SZ')
                            converted_count += 1

                        except ValueError:
                            # If parsing fails, leave it as is
                            pass
                
                writer.writerow(row)
                
        # Safely overwrite the original file with the new temporary file
        os.replace(temp_file.name, filepath)
        if(converted_count > 0):
            print(f"Success! Converted {converted_count} rows in-place.")
        else:
            print("Error! Could not convert timestamps.")

    except Exception as e:
        # Clean up the temp file if something crashes so we don't leave trash behind
        os.remove(temp_file.name)
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python convert_iso.py <path_to_your_csv>")
        sys.exit(1)
        
    csv_path = sys.argv[1]
    
    if not os.path.exists(csv_path):
        print(f"File not found: {csv_path}")
        sys.exit(1)
        
    convert_to_iso_inplace(csv_path)

# Run via:
# python convert_iso.py ./data/used_data/{filename}.csv