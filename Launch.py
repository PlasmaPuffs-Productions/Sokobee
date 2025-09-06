import os
import sys
import shutil
import subprocess
import pathlib
import platform

def main():
        arguments = sys.argv[1:]

        build_type = next((argument for argument in arguments if argument in ("Debug", "Release")), None)
        if not build_type:
                print(f"Usage: {sys.argv[0]} <Debug|Release> [--clean]")
                return 1

        root_directory = pathlib.Path(__file__).resolve().parent
        build_directory = root_directory / "Build"
        cache_file = build_directory / "CMakeCache.txt"
        executable_path = build_directory / "Sokobee"

        if "--clean" in arguments and build_directory.exists():
                shutil.rmtree(build_directory)

        build_directory.mkdir(exist_ok=True)
        os.chdir(build_directory)

        cmake_command = ["cmake", "..", f"-DCMAKE_BUILD_TYPE={build_type}"]

        system = platform.system()
        if system == "Windows":
                executable_path = executable_path.with_suffix(".exe")
                cmake_command += ["-G", "Ninja", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"]
                if build_type == "Debug":
                        cmake_command.append("-DCMAKE_C_FLAGS_DEBUG=-fsanitize=address -fno-omit-frame-pointer")
        elif system == "Darwin":
                if build_type == "Debug":
                        cmake_command.append("-DCMAKE_C_FLAGS_DEBUG=-fsanitize=address -fno-omit-frame-pointer")

        for cache_variable in ("SDL2_DIR", "SDL2_ttf_DIR", "SDL2_mixer_DIR"):
                cache_value = None

                if cache_file.exists():
                        with cache_file.open("r") as file:
                                for line in file:
                                        if line.startswith(cache_variable + ":"):
                                                cache_value = line.split("=", 1)[1].strip()

                if not cache_value:
                        cache_value = input(f"Enter path for {cache_variable}: ").strip()
                        cmake_command.append(f"-D{cache_variable}={cache_value}")

        subprocess.check_call(cmake_command)
        subprocess.check_call(["cmake", "--build", "."])

        if executable_path.is_file() and os.access(executable_path, os.X_OK):
                subprocess.run([str(executable_path)], cwd=root_directory)

if __name__ == "__main__":
        sys.exit(main())