import os
import sys
import shutil
import subprocess
import pathlib
import time

def main():
        arguments = sys.argv[1:]

        build_type = next((argument for argument in arguments if argument in ("Debug", "Release")), None)
        if not build_type:
                print(f"Usage: {sys.argv[0]} <Debug [--leaks]|Release> [--clean]")
                return 1

        root_directory = pathlib.Path(__file__).resolve().parent
        build_directory = root_directory / "Build"
        executable_path = build_directory / "Sokobee"

        use_leaks = "--leaks" in arguments
        if use_leaks and build_type == "Release":
                print("Cannot use Leaks on the release build mode")
                return 1

        if ("--clean" in arguments or use_leaks) and build_directory.exists():
                shutil.rmtree(build_directory)

        build_directory.mkdir(exist_ok=True)
        os.chdir(build_directory)

        if build_type == "Debug" and not use_leaks:
                subprocess.check_call(["cmake", "..", f"-DCMAKE_BUILD_TYPE={build_type}", "-DCMAKE_C_FLAGS_DEBUG=-fsanitize=address -fno-omit-frame-pointer"])
        else:
                subprocess.check_call(["cmake", "..", f"-DCMAKE_BUILD_TYPE={build_type}"])

        subprocess.check_call(["cmake", "--build", "."])

        if executable_path.is_file() and os.access(executable_path, os.X_OK):
                if build_type == "Debug" and use_leaks:
                        # Run the program with MallocStackLogging enabled
                        env = os.environ.copy()
                        env["MallocStackLogging"] = "1"

                        # Start the program
                        proc = subprocess.Popen([str(executable_path)], cwd=root_directory, env=env)
                        time.sleep(1.0)  # give it time to start up

                        print(f"Running malloc_history for PID {proc.pid} (press Ctrl+C to stop)...")
                        try:
                                subprocess.run(["malloc_history", str(proc.pid), "-callTree", "malloc[1024000-]", "-highWaterMark"])
                                # subprocess.run(["malloc_history", str(proc.pid), "-callTree", "malloc[512000-]", "-highWaterMark"])
                                #subprocess.run(["malloc_history", str(proc.pid)])
                        except KeyboardInterrupt:
                                pass

                        proc.wait()
                        #subprocess.run(["leaks", "-q", "--atExit", "--", str(executable_path)], cwd = root_directory)
                else:
                        subprocess.run([str(executable_path)], cwd = root_directory)

if __name__ == "__main__":
        sys.exit(main())