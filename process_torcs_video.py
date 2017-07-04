"""
    multithreaded predictor for videos
"""
import os
import cv2
import caffe 
import numpy as np
import argparse
import threading


ap = argparse.ArgumentParser(description = 'process videos with caffe models. currently the program supports mp4 videos only.')

ap.add_argument('h', help = "indicate frame height in pix", type = int)
ap.add_argument('w', help = "indicate frame width in pix", type = int)
ap.add_argument('fps', help = "indicate path to output video", type = int)
ap.add_argument('net', help = "indicate path to prototxt file that contains caffe net.", type = str)
ap.add_argument('model', help = "indicate path to caffemodel file that contains caffe weights.", type = str)

ap.add_argument('-i', help = "indicate path to input video", type = str)
ap.add_argument('-o', help = "indicate path to output video", type = str)
ap.add_argument('-s', action = "store_true", help = "if passed to program subtracts vgg means from video frames")
ap.add_argument('-all', action = "store_true", help = "if passed, process all videos in CURRENT DIRECTORY.")
ap.add_argument('-frms', help = "if passed, process the given number of frames. otherwise, process whole video.",type = int)

MEAN_VALUE = np.array([103.939, 116.779, 123.68])
THREAD_COUNT = 8

class multithreaded_video_processor(threading.Thread):
    def __init__(self, threadID, name, counter, *args):
        """
        net, width, height, sub_vgg_mean, vid_out
        """
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.net = args[0]
        self.width = args[1]
        self.height = args[2]
        self.sub_vgg_mean = args[3]
        self.vid_out = args[4]
        self.frames = args[5]
    
    def run(self):
        print "thread " + self.threadID + " started."
        count = self.threadID
        for fr in self.frames:
            image = cv2.resize(fr, (self.width, self.height))
            original_image = np.array([np.rollaxis(image, 2)])
            image = original_image.astype(np.float32)
            if self.sub_vgg_mean:
                image[0][0] = np.subtract(image[0][0], MEAN_VALUE[0])
                image[0][1] = np.subtract(image[0][1], MEAN_VALUE[1])
                image[0][2] = np.subtract(image[0][2], MEAN_VALUE[2])
            
            self.net.blobs['data_r'].data[...] = image
            predictions = self.net.forward(**{"data_r": image})['loss']

            predictions = np.squeeze(predictions)[1]
            predictions[predictions > 0.5] = 1
            predictions[predictions < 1] = 0

            original_image[0][1][predictions != 0] = 255
            original_image = original_image[0]
            original_image = np.swapaxes(original_image, 0, 1)
            original_image = np.swapaxes(original_image, 1, 2)
            
            ss = str(count)
            ss = ss.zfill(5)
            fs = self.vid_out + ss + '.png'
            cv2.imwrite(fs, original_image)
            count += THREAD_COUNT

def process_video(net, vid_in, vid_out, height, width, frames_count = -1, fps = 25, sub_vgg_mean = True):    
    """
        transform a video to raw
    """

    vdr = cv2.VideoCapture(vid_in) 
    count = 0
    total_length = frames_count
    if frames_count != -1:
        total_length = vdr.get(cv2.CAP_PROP_FRAME_COUNT)
    print "started converting " + vid_in[vid_in.rfind('/') + 1:] + '\n'
    
    count = 0
    frames = []
    while count < total_length:
        success, image = vdr.read()
        if not success:
            print("miss read frame.")
            continue
        frames.append(image)
        count += 1

    threads = []
    for i in range(THREAD_COUNT):
        frames_per_thread = frames[i::THREAD_COUNT]
        nthread = multithreaded_video_processor(i, str(i), i, net, width, height, sub_vgg_mean, vid_out, frames)
        threads.append(nthread)
        nthread.start()

    for i in range(THREAD_COUNT):
        threads[i].join()

    vdr.release()
    os.system('ffmpeg -f image2 -r ' + str(fps) + ' -i ' + vid_out + '_%05d.png -c:v libx264 -crf 20 ' + vid_out + 'raw.mp4')
    os.system('rm *' + vid_out[:len(vid_out) - 4] + '*.png')

def main():
    args = ap.parse_args()
    if not args.i and not args.o and not args.all:
        print "can't process no videos. must specify either -i or -o or -all"
        exit(0)
    net = caffe.Classifier(args.net, args.model)
    if args.all:
        vids = os.popen('ls | grep mp4').read().split('\n')
        for vid in vids:            
            if len(vid) > 0:
                v_o = vid[vid.rfind('/') + 1:len(vid) - 4] + '_'
                process_video(net, vid, v_o, args.h, args.w, args.frms, args.fps, args.s)
    else:        
        v_o = args.o
        v_o = v_o[v_o.rfind('/') + 1:len(v_o) - 4] + '_'
        process_video(net, args.i, v_o, args.h, args.w, args.frms, args.fps, args.s)
    

if __name__ == "__main__":
    main()




# image = cv2.resize(image, (width, height))
# original_image = np.array([np.rollaxis(image, 2)])
# image = original_image.astype(np.float32)
# if sub_vgg_mean:
#     image[0][0] = np.subtract(image[0][0], MEAN_VALUE[0])
#     image[0][1] = np.subtract(image[0][1], MEAN_VALUE[1])
#     image[0][2] = np.subtract(image[0][2], MEAN_VALUE[2])

# net.blobs['data_r'].data[...] = image
# predictions = net.forward(**{"data_r": image})['loss']

# predictions = np.squeeze(predictions)[1]
# predictions[predictions > 0.5] = 1
# predictions[predictions < 1] = 0

# original_image[0][1][predictions != 0] = 255
# original_image = original_image[0]
# original_image = np.swapaxes(original_image, 0, 1)
# original_image = np.swapaxes(original_image, 1, 2)

# ss = str(count)
# ss = ss.zfill(5)
# fs = vid_out + ss + '.png'
# cv2.imwrite(fs, original_image)