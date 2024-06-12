import importlib.util
from importlib.metadata import version
import sys

def main(version):
    print("Searching for mvstudio")
    
    if importlib.util.find_spec('mvstudio_data') is None:
        exit(1)
    if version('mvstudio_data') != version:
        exit(1)
    exit(0)

if __name__ == "__main__":
    version = str(sys.argv[1])
    main(version)