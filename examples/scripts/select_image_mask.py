# -*- coding: utf-8 -*-
# SPDX-License-Identifier: Apache-2.0 
"""

A notebook which illustrates the functionality with exemplary screenshots can be found at:
https://github.com/ManiVaultStudio/JupyterPlugin/blob/main/examples/projects/cell_segmentation_qupath/QuPathCellMasks.ipynb
The accompanying `select_image_mask.json` describes the input parameters for ManiVault, 
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
    return data


def main(args):
    """
    Main execution function.

    Args:
        args (argparse.Namespace): Parsed command-line arguments.
    """
    try:
        # Get data from ManiVault
        mask_guid   = args.input_mask_data_guid
        mask_mv_ref = get_data_from_mv(mask_guid)

        sel_guid    = args.input_sel_data_guid
        sel_mv_ref  = get_data_from_mv(sel_guid)

        # Set selection
        import numpy as np
        thresh      = args.selection_threshold
        mask_selection = np.flatnonzero(mask_mv_ref.points > thresh, dtype=np.uint32)

        sel_mv_ref.setSelection(mask_selection)

    except Exception as e:
        print("An error occurred:", e)


def parse_arguments():
    """Parses command-line arguments using argparse."""
    import argparse
    parser = argparse.ArgumentParser(
                    prog='select_image_mask',
                    description='Sets a selection based on a mask')
    
    parser.add_argument(
        "-i", "--input-mask-data-guid",
        required=True,
        help="GUID of the input mask data in ManiVault (required)."
    )

    parser.add_argument(
        "-s", "--input-sel-data-guid",
        required=True,
        help="GUID of the input mask data in ManiVault (required)."
    )

    parser.add_argument(
        "-t", "--selection-threshold",
        required=False,
        default=0,
        type=int,
        help="Selection threshold value (optional)."
    )

    return parser.parse_args()


if __name__ == "__main__":
    parsed_args = parse_arguments()
    main(parsed_args)
