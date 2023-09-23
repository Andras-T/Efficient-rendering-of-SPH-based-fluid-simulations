import os
import subprocess

file_extensions = [".cpp", ".h"]
project_directory = os.getcwd() + "\..\src"

def find_cpp_files(directory):
    cpp_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(tuple(file_extensions)):
                cpp_files.append(os.path.join(root, file))
    return cpp_files

def format_cpp_files(files):
    for file in files:
        try:
            subprocess.run(["clang-format", "-i", file], check=True)
            print(f"Formatted: {file}")
        except subprocess.CalledProcessError as e:
            print(f"Error formatting {file}: {e}")

if __name__ == "__main__":
    cpp_files = find_cpp_files(project_directory)
    format_cpp_files(cpp_files)
