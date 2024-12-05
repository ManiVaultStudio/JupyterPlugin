import numpy.typing as npt
import numpy as np

ClusterType = dict[str, npt.NDArray[np.uint32]]

class Cluster:
    """_summary_
    Comprises a dict of numpy arrays containing the cluster indexes and names
    appending ids and colors as additional  properties
    """

    def __init__(self, names: list[str], clusters: list[np.uint32], col_ids: list[str], col_colors: list[tuple[float, float, float, float]]):
        """_summary_
        A class that holds the ManiVault cluster indormation. All lists are ordered.

        Args:
            names (list[str]): Cluster names
            clusters (list[np.uint32]): A list numpy uint32 indexes
            col_ids (list): List of cluster guids
            col_colors (list): List of RGBA color float tuples  ()

        Returns:
            _type_: _description_

        Yields:
            _type_: _description_
        """
        self._names = names
        self._clusters = clusters
        self._ids = col_ids
        self._colors = col_colors

    @property
    def names(self) -> list[str]:
        return self._names

    @property
    def indices(self) -> list[np.uint32]:
        return self._clusters
    
    @property
    def cluster_guids(self) -> list[str]:
        return self._ids
    
    @property
    def colors(self) -> list[str]:
        return self._colors
