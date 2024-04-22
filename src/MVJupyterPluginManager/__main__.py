import argparse
import subprocess
from pathlib import Path
import os
import threading
import sys
import signal


stopProcess = False

# Define a SIGINT handler and register it
def handler(signum, frame):
    global stopProcess
    stopProcess = True
    print('Shutting down jupyter lab server...')
    sys.exit(0)
signal.signal(signal.SIGINT, handler)
signal.signal(signal.SIGTERM, handler)


## Perhaps add an option for the connection file Path

#    parser.add_argument("connectionfile", help="Required absolute path to connection file, must match the path in JupyterLauncher")
#    args = parser.parse_args()
#    os.environ["MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE"] = args.connectionfile

#    with --config={args.connection.file}

def main():
    """
    Start the jupyter lab with the package configuration file.
    The user muat supply the the path tothe connection file
    that contains the connection configuration written by the 
    ManiVault Studio JupyterPlugin
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", help="Optional path to Jupyter config file")
    args = parser.parse_args()

    # Command to start Jupyter Lab
    cmd = ["python", "-m",  "jupyter", "lab"]
    if args.config:
        cmd += ["--config", args.config] 

    # Start the process
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    # Function to handle output
    def handle_output(process, stream_name, outfile):
        while True:
            if stopProcess:
                break
            stream = getattr(process, stream_name)
            for line in iter(stream.readline, b''):
                print(f'{line.rstrip().decode()}', outfile)
            stream.close()
            if stopProcess:
                break
        if stream_name == 'stdout':
            process.kill()
            exit(0)

    # Start two threads to handle stdout and stderr
    stdout_thread = threading.Thread(target=handle_output, args=(process, 'stdout', sys.stdout))
    stderr_thread = threading.Thread(target=handle_output, args=(process, 'stderr', sys.stderr))
    stdout_thread.start()
    stderr_thread.start()

    # Wait for the process to finish
    process.wait()

    # Wait for output handling threads to finish
    stdout_thread.join()
    stderr_thread.join()

if __name__ == '__main__':
    main()