# Python scripts in ManiVault

Python scripts offer a simple way of extending the feature set of ManiVault.
They make it easy to load a custom file format into ManiVault, use any analysis module available in Python or save data in arbitrary formats to disk without having to compile a new ManiVault plugin.

## How to write a script extension for ManiVault

You'll need:
1. A Python script
2. A corresponding ManiVault Python extension file
3. (Optionally) a requirements file

For example:
```
load_custom_format.py
load_custom_format.json
load_custom_format_requirements.txt
```

When starting the Python extension, ManiVault will scan the `examples/JupyterPlugin/scripts` folder for ManiVault Python extension files and add entries for each script in the UI.

## ManiVault Python extension files

ManiVault requires an extension file for each python script, in order to properly find it and pass necessary arguments to the script.
The extension file is a simple `.json` file and defines mainly the type of arguments that the script expects, but also lists requirements and other meta information.
When executing a script, a user dialog is automatically populated with input fields for each argument.

Fields of the extension file:
- `script` (required): Name of the python script file
- `name` (required): Name of the script as to be displayed in ManiVault
- `type` (required): Corresponding to ManiVault's plugin types, i.e., "Loader", "Writer", "Analysis", "transform" and "View"
- `requirements` (optional): Name of a requirements file
- `description` (optional): Description of what the script does. Will be displayed in the user-input dialog
- `input-datatypes` (optional): ManiVault dataset types to which the script might apply (_currently not used_)
- `arguments` (optional): List of arguments passed to the script. Used to automatically populate a user dialog
    - `name` (required): Name of the argument as shown in the user dialog
    - `argument` (required): Command line argument name, e.g. "--input-file-image"
    - `type` (required): Data type, defined user dialog element. Available: "int", "float", "str", "file-in", "file-out", and "mv-data-in"
    - `required` (required): Whether this argument is required for the script (_currently not used, all defined arguments are used_)

Optional fields for specific argument types:
- `file-out`:
    - `file-extension` (optional): Possible file extension for output file 
    - `dialog-caption` (optional): Title of user dialog to select/define an output file
    - `dialog-filter` (optional): Filter shown in user dialog by extension
- `float`:
    - `default` (optional): Default value for the number
    - `range-min` and `range-max` (optional): Min and max range for the value slider in the UI element
    - `step-size` (optional): Step-size for the value slider in the UI element
    - `num-decimals` (optional): Precision of the floating point value
- `int`:
    - `default` (optional): Default value for the number
    - `range-min` and `range-max` (optional): Min and max range for the value slider in the UI element
- `mv-data-in`:
    - `datatypes` (optional): ManiVault dataset type which are allowed as input

### Argument types

- `int`: Adds an input field and slider for integers
- `float`: Adds an input field and slider for floating points
- `str`: Add an input field for text
- `file-in`: Adds a `mv::gui::FilePickerAction` which opens an OS-native file dialog to pick an input file from disk. The absolute file path is passed to the script.
- `file-out`: Opens an OS-native file dialog to select/create an output file. The absolute file path is passed to the script.
- `mv-data-in`: Adds a `mv::gui::DatasetPickerAction` and will pass the dataset GUID to the python script.


## Examples
See [../examples/scripts](../examples/scripts) for several example scripts:

- [Load cell image and mask](../examples/scripts/load_cell_image_and_mask.json): A loader script which asks for two files on disk and loads them into ManiVault after some pre-processing
- [Export image as PNG](../examples/scripts/write_image_as_png.json): A writer script which simply exports an image as a PNG to disk
- [Set selection using mask](../examples/scripts/select_image_mask.json): An analysis script which programmatically sets a selection in ManiVault. It asks for two ManiVault datasets as input (one to make the selection in and one to base the selection on) and a control parameter.


### Recommended Python script layout

We recommend using [`argparse`](https://docs.python.org/3/library/argparse.html) to parse command line arguments in the Python script.
This is a skeleton of how your script could look like, an `example_script.py`:

```python
# -*- coding: utf-8 -*-
# SPDX-License-Identifier: Apache-2.0 
"""
This script provides an example.

Usage: 
    example_script.py -i ./myData.bin

Author: First Last, 20xx
"""

def main(args):
    """
    Main execution function.

    Args:
        args (argparse.Namespace): Parsed command-line arguments.
    """
    try:
        import numpy as np

        # Load data
        data_path = args.input_file

        # Do stuff...

    except Exception as e:
        print("An error occurred:", e)


def parse_arguments():
    """Parses command-line arguments using argparse."""
    import argparse
    parser = argparse.ArgumentParser(
                    prog='example name',
                    description='example text')
    
    parser.add_argument(
        "-i", "--input-file",
        required=True,
        help="Path to the input file (required)."
    )

    return parser.parse_args()


if __name__ == "__main__":
    parsed_args = parse_arguments()
    main(parsed_args)
```

With a corresponding ManiVault extension files, a `example_script.json`:
```json
{
    "script": "example_script.py",
    "name": "Example script",
    "type": "Loader",
    "description": "Load data of arbitrary shape and form",
    "requirements": "example_script_requirements.txt",
    "arguments": [
        {
        "argument": "--input-file",
        "name": "Input file",
        "type": "file-in",
        "required": true
        }
    ]
}
```

And a requirements file `example_script_requirements.txt`:
```txt
numpy~=1.26
```
