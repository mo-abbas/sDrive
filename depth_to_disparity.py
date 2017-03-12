import numpy as np
import sys
from PIL import Image
from pfm_viewer import readPFM

width = 960.0
baseline = 1.0

def main(pfmPath):
    depthNormalized, _ = readPFM(pfmPath)
    zNear = 0.3
    zFar = 600.0
    depthNoMean = (depthNormalized - 0.5) * 2.0;
    depth = 2.0 * zFar * zNear / (zFar + zNear - depthNoMean * (zFar - zNear))
    depth[depth == zFar] = float('Inf')
    depthFlipped = np.flipud(depth)
    disparity = width * baseline / (depthFlipped * 2)
    print disparity[317,733]
    Image.fromarray(disparity.astype(np.uint8)).save('disp.png')

#main('front_00002_disp.pfm')

if __name__ == "__main__":
     main(sys.argv[1])
