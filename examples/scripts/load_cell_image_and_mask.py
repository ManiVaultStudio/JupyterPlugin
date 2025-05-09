import argparse
import sys


def load_json(load_json_file_path):
    import json
    print(f"load_json {load_json_file_path}")
    with open(load_json_file_path) as load_json_file:
        loaded_json_file = json.load(load_json_file)
    return loaded_json_file


def load_tiff(load_tiff_path):
    from tifffile import tifffile
    print(f"load_tiff {load_tiff_path}")
    return tifffile.imread(load_tiff_path)


def extract_mask_from_geojson(geo_json_file, mask_out_shape = (1024, 1024)):
    """
    :param geo_json_file: json file
    :param mask_out_shape: optional, Define output raster shape (e.g., 512x512) and transform (identity for simple pixel grid)
    :return:
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


def main(args):
    """
    Main execution function.

    Args:
        args (argparse.Namespace): Parsed command-line arguments.
    """
    print("ENTER MAIN")
    try:
        # Load data
        print(f"load_tiff {args.input_file_image}")
        image = load_tiff(args.input_file_image)
        print(f"load_json {args.input_file_json}")
        geojson = load_json(args.input_file_json)

        # Convert QuPath segmentation to mask
        mask = extract_mask_from_geojson(geojson, (image.shape[0], image.shape[1]))

        print(f"Loaded mask: {mask.shape}")

        # Connect to ManiVault
        print(f"About to import mvstudio")
        import mvstudio.data
        print(f"Done Importing")
        mv = mvstudio.data.Hierarchy()

        #print(f"mvstudio.data.Hierarchy: {mv}")

        print("Start pushing")

        # Push image and mask
        dataName        = args.output_file_name
        image_mv_ref    = mv.addImageItem(image, f"{dataName} Image")
        mask_mv_ref     = mv.addImageItem(mask, f"{dataName} Mask")

        print("Finished pushing")

    except Exception as e:
        print("An error occurred:", e)

    print("DONE MAIN")


def parse_arguments():
    """Parses command-line arguments using argparse."""
    print("ENTER PARSE ARGUMENTS")
    parser = argparse.ArgumentParser(
                    prog='load_cell_image_and_mask',
                    description='Loads tiff files and associate masks')
    
    print("ADD ARGUMENTS")

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
        help="Name of the output datasets (required)."
    )

    print("PARSE ARGUMENTS")

    args = parser.parse_args()

    print("PRINT ARGUMENTS")

    print(args)

    print("LEAVE PARSE ARGUMENTS")

    return args


if __name__ == "__main__":
    print("ENTER SCRIPT")

    parsed_args = parse_arguments()
    main(parsed_args)

    print("DONE SCRIPT")
