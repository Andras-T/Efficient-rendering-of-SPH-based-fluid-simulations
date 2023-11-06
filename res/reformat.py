import os
import subprocess

file_extensions = [".cpp", ".h", ".frag", ".vert", ".comp"]
project_directory = os.getcwd() + "\..\src"
shader_directory = os.getcwd() + "\..\shaders"

def find_files(directory):
    cpp_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(tuple(file_extensions)) and ".vshistory" not in root:
                cpp_files.append(os.path.join(root, file))
    return cpp_files

def clang_format_files(files):
    for file in files:
        try:
            subprocess.run(["clang-format", "-i", file], check=True)
            print(f"Formatted: {file}")
        except subprocess.CalledProcessError as e:
            print(f"Error formatting {file}: {e}")

if __name__ == "__main__":
    cpp_files = find_files(project_directory)
    clang_format_files(cpp_files)
    shader_files = find_files(shader_directory)
    clang_format_files(shader_files)
