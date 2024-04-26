import importlib.util
from importlib.metadata import version

def main():
    print("Searching for mvstudio")
    
    if importlib.util.find_spec('mvstudio') is None:
        exit(1)
    if version('mvstudio') != '0.8.0':
        exit(1)
    exit(0)

if __name__ == "__main__":
    main()