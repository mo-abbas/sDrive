import tensorflow as tf
import caffe
import sys
import numpy as np 
import cv2

KITTIMODEL = "/media/mahmoud/01D17E7170A327C0/workdir/KittiSeg/RUNS/KittiSeg_pretrained/model.ckpt-15999"
KITTIMETA = "/media/mahmoud/01D17E7170A327C0/workdir/KittiSeg/RUNS/KittiSeg_pretrained/model.ckpt-15999.meta"

#def main():
# importing kittiSeg tensorflow model
session = tf.Session()
saver = tf.train.import_meta_graph(KITTIMETA)
saver.restore(session, KITTIMODEL)

graph = tf.get_default_graph()
allKittiVariables = tf.global_variables()
allWeights = session.run(allKittiVariables)

image = cv2.imread('test_img.png')
gt = cv2.imread('gt_test_img.png')

predictions = session.run(image)