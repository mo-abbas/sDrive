import numpy as np
import sys
from PIL import Image
from dataset_to_lmdb import readPFM, writePFM

width = 960.0
baseline = 1.0

def depth_to_disparity(pfmPath):
    depthNormalized, _ = readPFM(pfmPath)
    zNear = 0.3
    zFar = 600.0
    depthNoMean = (depthNormalized - 0.5) * 2.0;
    depth = 2.0 * zFar * zNear / (zFar + zNear - depthNoMean * (zFar - zNear))
    depth[depth >= zFar - 1] = float('Inf')
	print depth[317,733]
	
    depthFlipped = np.flipud(depth)
    disparity = width * baseline / (depthFlipped * 2)
    Image.fromarray(disparity.astype(np.uint8)).save('disp.png')

if __name__ == "__main__":
     depth_to_disparity(sys.argv[1])
