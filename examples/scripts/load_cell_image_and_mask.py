# -*- coding: utf-8 -*-
# SPDX-License-Identifier: Apache-2.0 
"""
This script loads an image and a corresponding segmentation mask, and loads them into a connected ManiVault instance.
A notebook with equivalent functionality and exemplary screenshots can be found at:
https://github.com/ManiVaultStudio/JupyterPlugin/blob/main/examples/projects/cell_segmentation_qupath/QuPathCellMasks.ipynb
The accompanying `load_cell_image_and_mask.json` describes the input parameters for ManiVault, 
which automatically populates a user dialog.

Usage: 
    load_cell_image_and_mask.py -i ./myImage.tiff -j ./myJson.json -o NameInManiVault

Author: Alexander Vieth, 2025
"""

def load_json(load_json_file_path):
    import json
    with open(load_json_file_path) as load_json_file:
        loaded_json_file = json.load(load_json_file)
    return loaded_json_file


def load_tiff(load_tiff_path):
    from tifffile import tifffile
    return tifffile.imread(load_tiff_path)


def extract_mask_from_geojson(geo_json_file, mask_out_shape = (1024, 1024)):
    """
    :param geo_json_file: json file
    :param mask_out_shape: optional, Define output raster shape (e.g., 512x512) and transform (identity for simple pixel grid)
    :return: segmentation mask
    """
    from shapely.geometry import shape
    from rasterio.features import rasterize

    # Extract polygon geometries
    shapes = [shape(feature['geometry']) for feature in geo_json_file['features']]

    # Rasterize polygons into mask
    mask_out = rasterize(
        [(geom, 1) for geom in shapes],
        out_shape=mask_out_shape,
        fill=0,
        default_value=1
    )

    return mask_out


def push_images_to_mv(push_image, push_mask, push_name):
    """
    :param push_image: image data
    :param push_mask: mask data
    :param push_name: shared name
    """
    # Connect to ManiVault
    import mvstudio.data
    mv = mvstudio.data.Hierarchy()

    # Push data to ManiVault
    _           = mv.addImageItem(push_image, f"{push_name} Image")
    _           = mv.addImageItem(push_mask, f"{push_name} Mask")


def main(args):
    """
    Main execution function.

    Args:
        args (argparse.Namespace): Parsed command-line arguments.
    """
    try:
        # Load data
        image   = load_tiff(args.input_file_image)
        geojson = load_json(args.input_file_json)

        # Convert QuPath segmentation to mask
        mask    = extract_mask_from_geojson(geojson, (image.shape[0], image.shape[1]))

        # Push image and mask to ManiVault
        dataName = args.output_file_name
        push_images_to_mv(image, mask, dataName)

    except Exception as e:
        print("An error occurred:", e)


def parse_arguments():
    """Parses command-line arguments using argparse."""
    import argparse
    parser = argparse.ArgumentParser(
                    prog='load_cell_image_and_mask',
                    description='Loads tiff files and associate masks')
    
    parser.add_argument(
        "-i", "--input-file-image",
        required=True,
        help="Path to the input image (required)."
    )

    parser.add_argument(
        "-j", "--input-file-json",
        required=True,
        help="Path to the input json (required)."
    )

    parser.add_argument(
        "-o", "--output-file-name",
        required=True,
        help="Name of the output datasets in ManiVault (required)."
    )

    return parser.parse_args()


if __name__ == "__main__":
    parsed_args = parse_arguments()
    main(parsed_args)
