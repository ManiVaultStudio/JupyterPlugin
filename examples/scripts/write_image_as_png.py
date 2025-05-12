# -*- coding: utf-8 -*-
# SPDX-License-Identifier: Apache-2.0 
"""

This scripts writes a PNG file to disk 
The accompanying `write_image_as_png.json` describes the input parameters for ManiVault, 
which automatically populates a user dialog.

Usage: 
    select_image_mask.py -i mv_mask_guid -s mv_sel_guid -t 1

Author: Alexander Vieth, 2025
"""


def get_data_from_mv(get_guid):
    """
    :param get_guid: ManiVault data guid
    :return: ManiVault data reference 
    """
    # Connect to ManiVault
    import mvstudio.data
    mv = mvstudio.data.Hierarchy()

    # Get data from ManiVault
    data = mv.getItemByDataID(get_guid)
    print(f"Image height, width and channels: {data.image.shape}")
    return data


def main(args):
    """
    Main execution function.

    Args:
        args (argparse.Namespace): Parsed command-line arguments.
    """
    try:
        # Get data from ManiVault
        image_guid      = args.input_image_data_guid
        image_mv_ref    = get_data_from_mv(image_guid)

        # Export image
        from PIL import Image
        import numpy as np
        image_array     = image_mv_ref.image.astype(np.uint8)

        # Handle different memory model in ManiVault by flipping a bunch...
        image_array = np.flipud(image_array)

        # Transpose to (height, width, channels) from (channels, height, width)
        if image_array.shape[0] == 3:
            image_array = np.transpose(image_array, (1, 2, 0))

        image_array     = np.flipud(image_array)
        image_pil       = Image.fromarray(image_array)

        image_path      = args.export_path
        image_pil.save(image_path)

    except Exception as e:
        print("An error occurred:", e)


def parse_arguments():
    """Parses command-line arguments using argparse."""
    import argparse
    parser = argparse.ArgumentParser(
                    prog='select_image_mask',
                    description='Sets a selection based on a mask')
    
    parser.add_argument(
        "-i", "--input-image-data-guid",
        required=True,
        help="GUID of the input image data in ManiVault (required)."
    )

    parser.add_argument(
        "-o", "--export-path",
        required=True,
        help="Path on disk where to save the file (required)."
    )

    return parser.parse_args()


if __name__ == "__main__":
    parsed_args = parse_arguments()
    main(parsed_args)
