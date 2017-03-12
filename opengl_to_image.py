import sys
import numpy as np
from PIL import Image

def opengl_to_image(srcPath, outputPath):
    source = np.asarray(Image.open(srcPath))
    height, width, _ = source.shape
    size = width * height * 3

    flattenedSource = source.ravel()
    one   = flattenedSource[range(0, size, 3)]
    two   = flattenedSource[range(1, size, 3)]
    three = flattenedSource[range(2, size, 3)]

    merged = np.append(one, two)
    merged = np.append(merged, three).reshape((height, width, 3))
    merged = np.flipud(merged)
    Image.fromarray(merged).save(outputPath)

opengl_to_image(sys.argv[1], sys.argv[2])
