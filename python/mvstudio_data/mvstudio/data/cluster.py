import mvstudio_core
from .item import Item
import numpy.typing as npt
import numpy as np

ClusterType = dict[str, npt.NDArray[np.uint32]]

class Cluster:
    """_summary_
    Comprises a dict of numpy arrays containing the cluster indexes and names
    appending ids and colors as additional  properties
    """

    def __init__(self, clusters: ClusterType, col_ids: list[str], col_colors: list[tuple[float, float, float, float]]):
        """_summary_

        Args:
            clusters (ClusterType): A dict (keys are cluster names) of numpy uint32 indexes
            col_ids (list): List of cluster guids
            col_colors (list): List of RGBA color float tuples  ()

        Returns:
            _type_: _description_

        Yields:
            _type_: _description_
        """
        self._clusters = clusters
        self._ids = col_ids
        self._colors = col_colors

    @property
    def clusters(self) -> ClusterType:
        return self._clusters
    
    @property
    def col_ids(self) -> list[str]:
        return self._ids
    
    @property
    def colors(self) -> list[str]:
        return self._colors

Clusters = list[Cluster]

class ClusterMixin:
    """Mixin for Item to create a Cluster Item"""
    @property
    def cluster(self) -> Cluster:
        if self.type is not Item.ItemType.Cluster:
            return None
        (indexes, names, colors, ids) = mvstudio_core.get_cluster(self.datasetId)
        clusterDict = dict(zip(names, indexes))
        return Cluster(clusterDict, ids, colors)  

class ClusterItem(ClusterMixin, Item):
    """
    ClusterItem adds the cluster property to the
    basic Item type 
    """
    def __init__(self, guid_tuple, item_name, hierarchy_id):
        super().__init__(guid_tuple, item_name, hierarchy_id)