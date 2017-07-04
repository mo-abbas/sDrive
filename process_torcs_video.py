"""
 possible params MAX_NUM_FRAMES
"""
import os
import cv2
import caffe 
import numpy as np
import progressbar

VID_HEIGHT = 544
VID_WIDTH = 960
VIDEO1 = '/media/mahmoud/01D17E7170A327C0/workdir/new_videos/back_left.mp4'
VIDEO2 = '/media/mahmoud/01D17E7170A327C0/workdir/new_videos/front_left.mp4'
VIDEO3 = '/media/mahmoud/01D17E7170A327C0/workdir/new_videos/left-K.mp4'

VIDEO_O1 = '/media/mahmoud/01D17E7170A327C0/workdir/new_videos/back_left_raw_'
VIDEO_O2 = '/media/mahmoud/01D17E7170A327C0/workdir/new_videos/front_left_raw_'
VIDEO_O3 = '/media/mahmoud/01D17E7170A327C0/workdir/new_videos/left-K_raw_'
MEAN_VALUE = np.array([103.939, 116.779, 123.68])
def predict_video(net, vid_in, vid_out, fps = 25):    
    """
        transform a video to raw
    """
    vdr = cv2.VideoCapture(vid_in) 
    count = 0
    total_length = 50
    #bar = progressbar.ProgressBar(maxval=total_length, widgets=[progressbar.Bar('=', '[', ']'), ' ', progressbar.Percentage()])
    #bar.start()
    print ("started converting " + vid_in[vid_in.rfind('/') + 1:])
    while count < total_length:
        success, image = vdr.read()
        if not success:
            print("miss read frame.")
        image = cv2.resize(image, (960, 544))
        image = np.array([np.rollaxis(image, 2)])
        image = image.astype(np.float32)
        image[0][0] = np.subtract(image[0][0], MEAN_VALUE[0])
        image[0][1] = np.subtract(image[0][1], MEAN_VALUE[1])
        image[0][2] = np.subtract(image[0][2], MEAN_VALUE[2])
        net.blobs['data_r'].data[...] = image
        predictions = net.forward(**{"data_r": image})['loss']

        predictions = np.squeeze(predictions)[1]
        predictions[predictions > 0.5] = 1
        predictions[predictions < 1] = 0
        ss = str(count)
        ss = ss.zfill(5)
        fs = vid_out + ss + '.png'
        cv2.imwrite(fs, np.multiply(predictions, 255))
        count += 1  
        #bar.update(count)
            
    vdr.release()
    #bar.finish()
    os.system('ffmpeg -f image2 -r ' + str(fps) + ' -i ' + vid_out + '_%05d.png -c:v libx264 -crf 20 ' + vid_out + '_raw.mp4')


def main():
    net = caffe.Classifier('fcn8_deploy.prototxt', 'stitchedmodel.caffemodel')
    #predict_video(net, VIDEO1, VIDEO_O1)
    #predict_video(net, VIDEO2, VIDEO_O2)
    predict_video(net, VIDEO3, VIDEO_O3, 10)

if __name__ == "__main__":
    main()