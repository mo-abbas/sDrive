# python script to create file list for preparing image set

import os


f = open("imglist", "w")
defaultPath = "/mnt/eg5_data/road/data_road/training/image_2/"
defaultPath2 = "/mnt/eg5_data/road/data_road/training/gt_image_2/"
for i in range(0, 95):
    f.write(defaultPath + "um_" + str(i).zfill(6) + "f.png\t" + defaultPath2 + "um_roadf_" + str(i).zfill(6) + ".bmp\n")
for i in range(0, 96):
    f.write(defaultPath + "umm_" + str(i).zfill(6) + "f.png\t" + defaultPath2 + "umm_roadf_" + str(i).zfill(6) + ".bmp\n")
for i in range(0, 98):
    f.write(defaultPath + "uu_" + str(i).zfill(6) + "f.png\t" + defaultPath2 + "uu_roadf_" + str(i).zfill(6) + ".bmp\n")

f.close()
