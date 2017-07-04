"""
    script should be run from location of gt_images
    script transfroms imges to binary black and white with 2 classes only
"""
import cv2
import os
import numpy as np


BLACK = np.array([0, 0, 0])
WHITE = np.array([255, 255, 255])
PINK = np.array([255, 0, 255])
RED = np.array([0, 0, 255])



def main():
    """
    process files.
    """
    files = os.popen('ls ').read().split('\n')
    for f in files:
        img = cv2.imread(f)

        assert len(img.shape) == 3
        assert img.shape[0] == 1248 and img.shape[1] == 348 and img.shape[2] == 3

        for i in range(img.shape[0]):
            for j in range(img.shape[1]):
                if img[i][j] == PINK or img[i][j] == RED:
                    img[i][j] = BLACK
                else:
                    img[i][j] = WHITE


        cv2.imwrite("fxd_" + f, img)



if __name__ == "__main__":
    main()

# # defaultPath = ""
# # defaultPath2 = ""
# defaultPath = "/mnt/eg5_data/road/data_road/training/image_2/"
# defaultPath2 = "/mnt/eg5_data/road/data_road/training/gt_image_2/"
# for i in range(96):
#     im = Image.open(defaultPath2 + "umm_road_" + str(i).zfill(6) + ".png")
#     pix = im.load()
#     npixs = np.ones((384, 1248))
#     for j in range(im.size[0]):
#         for k in range(im.size[1]):
#             if pix[j, k] == (255,0,255):
#                 npixs[k, j] = 255
#             else:
#                 npixs[k, j] = 0
#     cv2.imwrite(defaultPath2 + "umm_roadf_" + str(i).zfill(6) + ".bmp", npixs)

# for i in range(95):
#     im = Image.open(defaultPath2 + "um_road_" + str(i).zfill(6) + ".png")
#     pix = im.load()
#     npixs = np.ones((384, 1248))
#     for j in range(im.size[0]):
#         for k in range(im.size[1]):
#             if pix[j, k] == (255,0,255):
#                 npixs[k, j] = 255
#             else:
#                 npixs[k, j] = 0
#     cv2.imwrite(defaultPath2 + "um_roadf_" + str(i).zfill(6) + ".bmp", npixs)  

# for i in range(98):
#     im = Image.open(defaultPath2 + "uu_road_" + str(i).zfill(6) + ".png")    
#     pix = im.load()
#     npixs = np.ones((384, 1248))
#     for j in range(im.size[0]):
#         for k in range(im.size[1]):
#             if pix[j, k] == (255,0,255):
#                 npixs[k, j] = 255
#             else:
#                 npixs[k, j] = 0
#     cv2.imwrite(defaultPath2 + "uu_roadf_" + str(i).zfill(6) + ".bmp", npixs)      


# def process(strp, rn):
#     for i in range(rn):
#         im = cv2.imread(strp + str(i).zfill(6) + ".png")
#         im2 = cv2.resize(im, (1248, 384))
#         tstr = (str) (strp + str(i).zfill(6) + "f.png")
#         cv2.imwrite(tstr, im2)


# process(defaultPath + "um_", 95)
# process(defaultPath + "umm_", 96)
# process(defaultPath + "uu_", 98)