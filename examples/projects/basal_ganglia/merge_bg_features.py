"""
Downloads Mammalian Basal Ganglia Consensus Cell Type Atlas data
and merges all gene expression features of genes shared across
the three species human, mouse and marmoset into a single h5 file.

NOTE:
    This script will download ~160GB
    This script requires a lot of memory, 64GB is NOT enough
    Downloaded files and merged file will be placed next to this script
    The merged fill will be roughly ~75GB large

Alexander Vieth
2025
"""
from pathlib import Path
import anndata as ad
import functools as fct
import numpy as np
import h5py
from scipy import sparse as sp

def fetch_data(download_url : str, download_file_name : Path) -> None:
    from urllib.request import urlopen
    from urllib.error import HTTPError, URLError
    import ssl
    import shutil

    if not download_url.startswith("https://"):
        raise ValueError("Only HTTPS URLs are allowed.")

    if download_file_name.exists():
        print(f"Using existing file at: {download_file_name.resolve()}")
        return

    # Ensure parent directory exists
    download_file_name.parent.mkdir(parents=True, exist_ok=True)

    try:
        print(f"Downloading file to: {download_file_name.resolve()}")

        context = ssl.create_default_context()

        # Download the file from `url` and save it locally under `file_name`:
        with urlopen(download_url, context=context, timeout=10) as response, download_file_name.open('wb') as download_out_file:
            shutil.copyfileobj(response, download_out_file)

        print(f"Downloaded file to {download_file_name.resolve()}")
    except HTTPError as e:
        print(f"HTTP error: {e.code} - {e.reason}")
    except URLError as e:
        print(f"URL error: {e.reason}")
    except Exception as e:
        print(f"Unexpected error: {e}")

def save_h5(sparse_mat: sp.csr_matrix | sp.csc_matrix, filename: str | Path,
            storage_type: str, var_names = None, obs_names = None) -> None:
    """
    Save sparse matrices in CSC (Compressed Sparse Column) or CSR (Compressed Sparse Row) format to HDF5.
    """
    if storage_type.lower() == 'csr_matrix' and not isinstance(sparse_mat, sp.csr_matrix):
        raise TypeError('Data is not CSR.')
    if storage_type.lower() == 'csc_matrix' and not isinstance(sparse_mat, sp.csc_matrix):
        raise TypeError('Data is not CSC.')

    data_string_dt = h5py.string_dtype(encoding='utf-8')

    if not isinstance(filename, Path):
        filename = Path(filename)

    filename.parent.mkdir(parents=True, exist_ok=True)

    with h5py.File(filename, 'w') as f:
        group_x = f.create_group("X")

        group_x.attrs['encoding-type'] = storage_type
        group_x.attrs['encoding-version'] = "0.1.0"
        group_x.attrs['shape'] = sparse_mat.shape
        group_x.attrs['data-type'] = str(sparse_mat.data.dtype)
        group_x.attrs['indices-type'] = str(sparse_mat.indices.dtype)
        group_x.attrs['indptr-type'] = str(sparse_mat.indptr.dtype)

        group_x.create_dataset('data', data=sparse_mat.data)
        group_x.create_dataset('indices', data=sparse_mat.indices)  # row indices
        group_x.create_dataset('indptr', data=sparse_mat.indptr)  # column pointers

        # Optional metadata
        if obs_names is not None and len(obs_names) == sparse_mat.shape[0]:
            group_obs = f.create_group("obs")
            group_obs.create_dataset('_index', data=obs_names, dtype=data_string_dt)
        if var_names is not None and len(var_names) == sparse_mat.shape[1]:
            group_var = f.create_group("var")
            group_var.create_dataset('_index', data=var_names, dtype=data_string_dt)

def extract_column_from_csr(h5_file: h5py.File, extract_col_idx: int, extract_matrix_path: str = 'X') -> np.ndarray:
    # Access the csr matrix components
    matrix_group = h5_file[f'{extract_matrix_path}']

    assert matrix_group.attrs['encoding-type'] == 'csr_matrix'

    data = matrix_group['data']
    indices = matrix_group['indices']
    indptr = matrix_group['indptr']
    shape = matrix_group.attrs['shape']

    n_rows = shape[0]
    column_values = np.zeros(n_rows, dtype=data.dtype)

    # Process each row to find values in the target column
    for row_idx in range(n_rows):
        start_idx = indptr[row_idx]
        end_idx = indptr[row_idx + 1]

        # Get column indices for this row
        row_col_indices = indices[start_idx:end_idx]

        # Find if our target column is in this row
        col_mask = row_col_indices == extract_col_idx
        if np.any(col_mask):
            # Get the corresponding data values
            row_data = data[start_idx:end_idx]
            column_values[row_idx] = row_data[col_mask][0]  # Should be only one value

    return column_values

def extract_column_from_csc(h5_file: h5py.File, extract_col_idx: int, extract_matrix_path: str = "X") -> np.ndarray:
    matrix_group = h5_file[f"{extract_matrix_path}"]

    assert matrix_group.attrs["encoding-type"] == "csc_matrix"

    data = matrix_group["data"]
    indices = matrix_group["indices"]
    indptr = matrix_group["indptr"]
    shape = matrix_group.attrs["shape"]

    n_rows = shape[0]
    column_values = np.zeros(n_rows, dtype=data.dtype)

    # get the slice for this column
    start_idx = indptr[extract_col_idx]
    end_idx = indptr[extract_col_idx + 1]

    col_rows = indices[start_idx:end_idx]
    col_data = data[start_idx:end_idx]

    # fill values
    column_values[col_rows] = col_data

    return column_values

def get_matrix_info(h5_file: h5py.File, matrix_info_path: str = 'X') -> tuple[int, int, int]:
    """Get matrix dimensions from H5 file."""
    matrix_group = h5_file[f'{matrix_info_path}']
    shape = matrix_group.attrs['shape']
    nnz = matrix_group["data"].shape[0]
    return shape[0], shape[1], nnz

def combine_csr_to_csr(ann_dats: list[ad.AnnData], column_indices : list[np.ndarray]) -> sp.csr_matrix | None:
    assert all(x.shape == column_indices[0].shape for x in column_indices)

    num_files = len(ann_dats)
    print(f"Combine {num_files} files")

    # Get matrix dimensions
    shapes = [(ann_dat.n_obs, ann_dat.n_vars, len(ann_dat.X._data)) for ann_dat in ann_dats]

    n_cols_combined = column_indices[0].shape[0]
    n_rows_combined = sum(rows for rows, cols, nnz in shapes)

    for i in range(num_files):
        print(f"Matrix {i} shape: ({shapes[i][0]}, {shapes[i][1]}), nnz: {shapes[i][2]}")
    print(f"Combined shape: ({n_rows_combined}, {n_cols_combined})")

    print(f"Load {ann_dats[0].filename.name}")
    combined_matrix_csr = ann_dats[0].X.to_memory()

    for i in range(1, num_files):
        print(f"Load {ann_dats[i].filename.name}")
        single_matrix = ann_dats[i].X.to_memory()
        combined_matrix_csr = sp.vstack([combined_matrix_csr, single_matrix[:, column_indices[i]]])
        del single_matrix

    print(f"Created CSR matrix with shape: {combined_matrix_csr.shape}")
    print(f"Number of non-zero elements: {combined_matrix_csr.nnz}")

    return combined_matrix_csr

if __name__ == "__main__":
    print(f"START: {__file__}")

    path_base = Path(__file__).parent.resolve()
    save_path = path_base / 'merged_all_csr.h5'

    # Remote data, see https://alleninstitute.github.io/HMBA_BasalGanglia_Consensus_Taxonomy/
    url_base = "https://released-taxonomies-802451596237-us-west-2.s3.us-west-2.amazonaws.com/HMBA/BasalGanglia"
    data_version = "BICAN_05072025_pre-print_release"

    HUMAN = 0
    MACAQ = 1
    MARMO = 2
    SPECIES = [HUMAN, MACAQ, MARMO]

    data_names = [""] * 3
    data_names[HUMAN] = "Human_HMBA_basalganglia_AIT_pre-print.h5ad"
    data_names[MACAQ] = "Macaque_HMBA_basalganglia_AIT_pre-print.h5ad"
    data_names[MARMO] = "Marmoset_HMBA_basalganglia_AIT_pre-print.h5ad"

    data_ait_urls = [f"{url_base}/{data_version}/{data_names[kind]}" for kind in SPECIES]
    data_ait_paths = [path_base / data_names[kind] for kind in SPECIES]

    print("Ensure data is available locally...")

    # Download data
    for kind in SPECIES:
        fetch_data(data_ait_urls[kind], data_ait_paths[kind])

    print("Data is available locally. Loading data...")

    # Load anndata [you'll need a good amount of RAM even though not all data is loaded to memory]
    d_ait = [ad.io.read_h5ad(data_ait_paths[kind], backed='r') for kind in SPECIES]

    # expected:
    # (1'034'819, 36'601)
    # (  548'281, 35'219)
    # (  313'033, 35'787)
    for kind in SPECIES:
        print(f"({d_ait[kind].n_obs:,}, {d_ait[kind].n_vars:,})".replace(",", "'").rjust(19))

    print("Finished loading data.")

    # Prep some meta data
    shared_var_names = fct.reduce(np.intersect1d, [d_ait[kind].var_names.to_numpy() for kind in SPECIES])
    shared_var_names = np.array(np.sort(shared_var_names), dtype=np.dtypes.StringDType())

    all_obs_names = np.concatenate([d_ait[kind].obs_names.to_numpy() for kind in SPECIES])
    shared_obs_names = fct.reduce(np.intersect1d, all_obs_names)

    assert len(shared_obs_names) == 0

    print(f"Compute indices")
    var_indices = [
        np.array([d_ait[kind].var_names.get_loc(var) for var in shared_var_names if var in d_ait[kind].var_names]) for
        kind in SPECIES]

    print(f"Merge to csr")

    merged_features_csr = combine_csr_to_csr(d_ait, var_indices)

    assert len(all_obs_names) == merged_features_csr.shape[0]

    print(f"Save merged features to {save_path}")
    save_h5(merged_features_csr, save_path, 'features_all_csr', shared_var_names, all_obs_names)

    print(f"FINISHED: {__file__}")
