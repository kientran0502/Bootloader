import hashlib
import json
import os

def calculate_sha256(file_path):
    sha256_hash = hashlib.sha256()
    try:
        with open(file_path, "rb") as file:
            for byte_block in iter(lambda: file.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest(), None
    except FileNotFoundError:
        return None, "Error: .bin file not found!"
    except Exception as e:
        return None, f"Error: {str(e)}!"

def get_file_size(file_path):
    try:
        return os.path.getsize(file_path), None
    except FileNotFoundError:
        return None, "Error: File not found!"
    except Exception as e:
        return None, f"Error: {str(e)}!"

def read_json_file(json_file):
    try:
        with open(json_file, "r") as file:
            data = json.load(file)
            if not data or not isinstance(data, list) or not data[0]:
                return None, None, None, "Error: Invalid JSON format!"
            record = data[0]
            return (record.get("file_name"), record.get("version"), 
                    record.get("sha256_hash"), record.get("file_size"))
    except FileNotFoundError:
        return None, None, None, "Error: .json file not found!"
    except json.JSONDecodeError:
        return None, None, None, "Error: Invalid JSON file!"
    except Exception as e:
        return None, None, None, f"Error: {str(e)}!"

def is_valid_bin_file(file_path):
    """Check if file has .bin extension"""
    return file_path.lower().endswith('.bin')

def is_valid_json_file(file_path):
    """Check if file has .json extension"""
    return file_path.lower().endswith('.json')

def main():
    while True:
        bin_file = input("Enter path to .bin file: ")
        if is_valid_bin_file(bin_file):
            break
        print("Error: File must have .bin extension")
    json_file = os.path.splitext(bin_file)[0] + '.json'
    file_name, version, stored_hash, stored_size = read_json_file(json_file)
    if stored_size and isinstance(stored_size, str) and stored_size.startswith("Error"):
        print(stored_size)
        return
    
    current_hash, hash_error = calculate_sha256(bin_file)
    current_size, size_error = get_file_size(bin_file)
    
    if current_hash is None:
        print(hash_error)
        return
    if current_size is None:
        print(size_error)
        return
    
    print(f"File name: {file_name}")
    print(f"Version: {version}")
    print(f"Stored size: {stored_size} bytes")
    print(f"Current size: {current_size} bytes")
    print(f"Stored SHA-256: {stored_hash}")
    print(f"Current SHA-256: {current_hash}")
    
    if current_hash == stored_hash and current_size == stored_size:
        print("Result: .bin file matches stored SHA-256 and size.")
    else:
        print("Result: .bin file does NOT match stored SHA-256 or size.")

if __name__ == "__main__":
    main()