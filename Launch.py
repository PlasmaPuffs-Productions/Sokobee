import os
import sys
import shutil
import subprocess
import pathlib

def main():
        arguments = sys.argv[1:]

        build_type = next((argument for argument in arguments if argument in ("Debug", "Release")), None)
        if not build_type:
                print(f"Usage: {sys.argv[0]} <Debug|Release> [--clean]")
                return 1

        root_directory = pathlib.Path(__file__).resolve().parent
        build_directory = root_directory / "Build"
        executable_path = build_directory / "Sokobee"

        if "--clean" in arguments and build_directory.exists():
                shutil.rmtree(build_directory)

        build_directory.mkdir(exist_ok=True)
        os.chdir(build_directory)

        if build_type == "Debug":
                subprocess.check_call(["cmake", "..", f"-DCMAKE_BUILD_TYPE={build_type}", "-DCMAKE_C_FLAGS_DEBUG=-fsanitize=address -fno-omit-frame-pointer"])
        else:
                subprocess.check_call(["cmake", "..", f"-DCMAKE_BUILD_TYPE={build_type}"])

        subprocess.check_call(["cmake", "--build", "."])

        if executable_path.is_file() and os.access(executable_path, os.X_OK):
                subprocess.run([str(executable_path)], cwd = root_directory)

if __name__ == "__main__":
        sys.exit(main())