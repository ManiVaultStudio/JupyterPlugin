# Available Python functions

Public data-access API in `mvstudio.data` (current version 08.09.2026).

```python
import mvstudio.data

dh = mvstudio.data.Hierarchy()
print(dh)
points_item = dh.getItemByIndex([1])
points_data = points_item.points
```

## Hierarchy

- `Hierarchy(load_option=Item.LoadOption.Immediate)`: Create the data hierarchy.
- `dh.refresh()`: Rebuild the hierarchy from ManiVault.
- `dh.children()`: Iterate over top-level items.
- `print(dh)`: Display item indices, dataset IDs, names and types.

### Loading options

- `Item.LoadOption.Immediate`: Load Points matrices during hierarchy construction (default).
- `Item.LoadOption.Delayed`: Build hierarchy metadata without loading Points matrices; retrieve values when requested.

### Find items

Available on both `Hierarchy` and `Item`. Item lookups search the item itself and its descendants.

- `getItem(itemId)`: Find by hierarchy item ID.
- `getItemByDataID(datasetId)`: Find by dataset ID.
- `getItemByName(name)`: Find by display name.
- `getItemByIndex(index)`: Find by hierarchy index, e.g. `[1, 3]`.

### Add datasets

- `dh.addPointsItem(data, name, parentDataId="", dimensionNames=[])`: Add a Points dataset.
- `dh.addDerivedPointsItem(data, name, sourceDataId, dimensionNames=[])`: Add Points derived from an existing dataset.
- `dh.addImageItem(data, name, dimensionNames=[])`: Add an image dataset.
- `dh.addClusterItem(parent, indices, name, **kwargs)`: Add a cluster dataset; optional keyword arguments: `names`, `colors`.

Points arrays must be C-contiguous and shaped `(num_points, num_dimensions)`. Derived Points currently require the same row count as the source. `parentDataId` specifies hierarchy placement; `sourceDataId` specifies derivation. For clusters, `parent` is the parent Points dataset ID and `indices` is a list of point-index arrays.

## Item

### Properties

- `item.itemId`: Hierarchy item ID.
- `item.datasetId`: Dataset ID.
- `item.name`: Display name.
- `item.type`: `Item.ItemType.Points`, `Item.ItemType.Image` or `Item.ItemType.Cluster`.
- `item.rawname`: Raw data name.
- `item.rawsize`: Raw data size in bytes.

For Points datasets:

- `item.points`: Full points matrix as a NumPy array.
- `item.numpoints`: Number of points.
- `item.numdimensions`: Number of dimensions.
- `item.dimensionNames`: Dimension names in column order.
- `item.properties`: Available dataset-property names.

### Methods

- `item.children()`: Iterate over child items.
- `item.getProperty(name)`: Read a dataset property (Points).
- `item.getSelection()`: Read the current selection indices.
- `item.setSelection(selectionIDs)`: Set the selection using a one-dimensional `np.uint32` array.
- `item.setLinkedData(target, selectionMapping)`: Link Points datasets using an array of target-index arrays, one per source point.

### Partial point-data access

- `item.getPoints(rows=None, dimensions=None)`: Retrieve selected rows and dimensions as a two-dimensional NumPy array.
- `item.getSelectedPoints(dimensions=None)`: Retrieve the currently selected points for the requested dimensions.

Use lists or arrays of zero-based, nonnegative integer indices. `None` selects the full axis; an empty list selects no entries. The existing `item.points` property is available for full-matrix access.

```python
# example
subset = points_item.getPoints(rows=[10, 20, 30], dimensions=[0, 5])
selected_subset = points_item.getSelectedPoints(dimensions=[0, 5])

genes = ["Shh", "Rspo1"]
dimension_names = points_item.dimensionNames
dimensions = [dimension_names.index(gene) for gene in genes]
gene_data = points_item.getPoints(dimensions=dimensions)
```

The current C++ implementation temporarily reads all rows for the requested dimensions before returning the requested rows.

## ClusterItem and Cluster

`ClusterItem` inherits the Item API and adds `cluster_item.cluster`, which returns a `Cluster` object:

- `cluster.names`: Cluster names.
- `cluster.indices`: Point-index arrays, one per cluster.
- `cluster.cluster_guids`: Cluster IDs.
- `cluster.colors`: Cluster RGBA colors.

These lists use the same cluster order.

## ImageItem

`ImageItem` inherits the Item API and adds:

- `image_item.image`: Image data as a NumPy array; cached after first access.
