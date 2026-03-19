import hashlib
import os
import json
import re

def calculate_sha256(file_path):
    sha256_hash = hashlib.sha256()
    try:
        with open(file_path, "rb") as file:
            for byte_block in iter(lambda: file.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest(), None
    except FileNotFoundError:
        return None, "Error: File not found"
    except Exception as e:
        return None, f"Error: {str(e)}"

def get_file_size(file_path):
    try:
        return os.path.getsize(file_path), None
    except FileNotFoundError:
        return None, "Error: File not found!"
    except Exception as e:
        return None, f"Error: {str(e)}!"

def save_to_json(file_path, version, hash_value, file_size, output_file):
    file_name = os.path.basename(file_path)
    data = [{
        "file_name": file_name,
        "version": version,
        "sha256_hash": hash_value,
        "file_size": file_size
    }]
    
    try:
        with open(output_file, "w") as json_file:
            json.dump(data, json_file, indent=2)
    except Exception as e:
        return f"Error saving JSON file: {str(e)}"
    return None

def main():
    while True:
        file_path = input("Enter path to .bin file: ")
        if file_path.lower().endswith('.bin'):
            break
        print("Error: File must have .bin extension!")
    output_file = os.path.splitext(file_path)[0] + '.json'
    while True:
        version = input("Enter version (e.g., 1.0.0): ")
        if bool(re.match(r"^\d+\.\d+\.\d+$", version)):
            break
        print("Error: Version must follow the format m.n.p (e.g., 1.0.0)!")
    
    hash_value, sha_error = calculate_sha256(file_path)
    if hash_value:
        file_size, fs_eror = get_file_size(file_path)
        if file_size is not None:
            error = save_to_json(file_path, version, hash_value, file_size, output_file)
            if error:
                print(error)
            else:
                print(f"SHA-256 hash: {hash_value}")
                print(f"File size: {file_size} bytes")
                print(f"Saved to {output_file}!")
        else: print(fs_eror)
    else: print(sha_error)
if __name__ == "__main__":
    main()