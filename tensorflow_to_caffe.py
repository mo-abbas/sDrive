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

net = caffe.Net('fcn8.prototxt', caffe.TEST)

""" 
'conv1_1', 'conv1_2', 'conv2_1', 'conv2_2', 'conv3_1', 'conv3_2', 'conv3_3', 'conv4_1', 
'conv4_2', 'conv4_3', 'conv5_1', 'conv5_2', 'conv5_3', 'fc6_t', 'fc7_t', 'score_fr', 
'upscore2', 'score_pool4', 'upscore_pool4', 'score_pool3', 'upscore8'
"""

dfw = allWeights[0]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[1]
net.params['conv1_1'][0].data[...] = dfw
net.params['conv1_1'][1].data[...] = dfb

dfw = allWeights[2]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[3]
net.params['conv1_2'][0].data[...] = dfw
net.params['conv1_2'][1].data[...] = dfb

dfw = allWeights[4]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[5]
net.params['conv2_1'][0].data[...] = dfw
net.params['conv2_1'][1].data[...] = dfb

dfw = allWeights[6]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[7]
net.params['conv2_2'][0].data[...] = dfw
net.params['conv2_2'][1].data[...] = dfb

dfw = allWeights[8]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[9]
net.params['conv3_1'][0].data[...] = dfw
net.params['conv3_1'][1].data[...] = dfb

dfw = allWeights[10]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[11]
net.params['conv3_2'][0].data[...] = dfw
net.params['conv3_2'][1].data[...] = dfb


dfw = allWeights[12]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[13]
net.params['conv3_3'][0].data[...] = dfw
net.params['conv3_3'][1].data[...] = dfb

dfw = allWeights[14]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[15]
net.params['conv4_1'][0].data[...] = dfw
net.params['conv4_1'][1].data[...] = dfb

dfw = allWeights[16]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[17]
net.params['conv4_2'][0].data[...] = dfw
net.params['conv4_2'][1].data[...] = dfb

dfw = allWeights[18]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[19]
net.params['conv4_3'][0].data[...] = dfw
net.params['conv4_3'][1].data[...] = dfb

dfw = allWeights[20]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[21]
net.params['conv5_1'][0].data[...] = dfw
net.params['conv5_1'][1].data[...] = dfb

dfw = allWeights[22]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[23]
net.params['conv5_2'][0].data[...] = dfw
net.params['conv5_2'][1].data[...] = dfb

dfw = allWeights[24]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[25]
net.params['conv5_3'][0].data[...] = dfw
net.params['conv5_3'][1].data[...] = dfb

dfw = allWeights[26]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[27]
net.params['fc6_t'][0].data[...] = dfw
net.params['fc6_t'][1].data[...] = dfb

dfw = allWeights[28]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[29]
net.params['fc7_t'][0].data[...] = dfw
net.params['fc7_t'][1].data[...] = dfb


dfw = allWeights[30]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[31]
net.params['score_fr'][0].data[...] = dfw
net.params['score_fr'][1].data[...] = dfb

dfw = allWeights[32]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
net.params['upscore2'][0].data[...] = dfw

dfw = allWeights[33]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[34]
net.params['score_pool4'][0].data[...] = dfw
net.params['score_pool4'][1].data[...] = dfb

dfw = allWeights[35]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
net.params['upscore_pool4'][0].data[...] = dfw

dfw = allWeights[36]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
dfb = allWeights[37]
net.params['score_pool3'][0].data[...] = dfw
net.params['score_pool3'][1].data[...] = dfb

dfw = allWeights[38]
dfw = np.rollaxis(dfw, 3)
dfw = np.rollaxis(dfw, 3, 1)
net.params['upscore8'][0].data[...] = dfw

img = cv2.imread('test_img.png')
gt = cv2.imread('gt_test_img.png')

img = np.array([np.rollaxis(img, 2)])



# if __name__ == "__main__":
# 	main()