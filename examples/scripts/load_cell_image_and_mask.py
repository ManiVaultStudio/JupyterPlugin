import argparse
import sys


def load_json(load_json_file_path):
    import json
    with open(load_json_file_path) as load_json_file:
        loaded_json_file = json.load(load_json_file)
    return loaded_json_file


def load_tiff(load_tiff_path):
    from tifffile import tifffile
    return tifffile.imread(load_tiff_path)


def extract_mask_from_geojson(geo_json_in, mask_out_shape = (1024, 1024)):
    """
    :param geo_json_in: json file
    :param mask_out_shape: optional, Define output raster shape (e.g., 512x512) and transform (identity for simple pixel grid)
    :return:
    """
    from shapely.geometry import shape
    from rasterio.features import rasterize

    # Extract polygon geometries
    shapes = [shape(feature['geometry']) for feature in geo_json_in['features']]

    # Rasterize polygons into mask
    mask_out = rasterize(
        [(geom, 1) for geom in shapes],
        out_shape=mask_out_shape,
        fill=0,
        default_value=1
    )

    return mask_out


def main(args):
    """
    Main execution function.

    Args:
        args (argparse.Namespace): Parsed command-line arguments.
    """
    # Load data
    image = load_tiff(args.input_file_image)
    geojson = load_json(args.input_file_json)

    # Convert QuPath segmentation to mask
    mask = extract_mask_from_geojson(geojson, (image.shape[0], image.shape[1]))

    try:
        # Connect to ManiVault
        import mvstudio.data
        mv = mvstudio.data.Hierarchy()

        # Push image and mask
        image_mv_ref = mv.addImageItem(image, "Cell Image")
        mask_mv_ref = mv.addImageItem(mask, "Mask Image")
    except:
        sys.exit(1) # Exit with a non-zero status code to indicate failure

    sys.exit(0) # Explicitly exit with 0 for success


def parse_arguments():
    """Parses command-line arguments using argparse."""

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

    return parser.parse_args()


if __name__ == "__main__":
    parsed_args = parse_arguments()
    main(parsed_args)
