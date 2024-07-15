import mvstudio_core
from .item import Item
import numpy.typing as npt
import numpy as np
import functools

Image = npt.NDArray[np.float32]
Images = list[Image] 

class ImageMixin:
    """Mixin for Item to create an Image Item"""
    @functools.cached_property
    def image(self) -> np.ndarray:
        """If this is an image return the image data in a numpy array
            otherwise return np.empty([0])
        """
        # information is in the image metadata
        # TODO cache image array and add cache clear option
        if self.type is not Item.ItemType.Image:
            return np.empty([0])
        id = mvstudio_core.find_image_dataset(self.datasetId)
        if len(id) > 0:
            array =  mvstudio_core.get_image_item(self.datasetId)
            return array
        return np.empty([0]) 

class ImageItem(ImageMixin, Item):
    """
    ImageItem adds the Image property the basic Item
    Images are numpy arrays sahpes to match the image
    meta data
    """
    def __init__(self, guid_tuple, name, hierarchy_id):
        super().__init__(guid_tuple, name, hierarchy_id)