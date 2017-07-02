import sys
import caffe
import cv2
import os
import numpy as np

MEAN_VALUE = np.array([103.939, 116.779, 123.68])

GPU_ID = 0
model = 'stitchedmodel.caffemodel'
caffe.set_device(GPU_ID)
caffe.set_mode_gpu()

classifier = caffe.Classifier('fcn8_deploy.prototxt', model)
print(classifier.blobs['data_r'].shape)
#test_data = open('fixed_data/test.list', 'r').readlines()
paths = ['test_img2.png', 'gt_test_img2.png']
average_error_percentage = 0
iteration = 0

#for line in test_data:
#   paths = line.strip().split('\t')
image = cv2.imread(paths[0])
gt = cv2.imread(paths[1])

#image = cv2.resize(image, (1248, 384))
image = np.array([np.rollaxis(image, 2)])
image = image.astype(np.float32)
image[0][0] = np.subtract(image[0][0], MEAN_VALUE[0])
image[0][1] = np.subtract(image[0][1], MEAN_VALUE[1])
image[0][2] = np.subtract(image[0][2], MEAN_VALUE[2])

predictions = classifier.forward(**{"data_r": image})['loss']
predictions = np.squeeze(predictions)[1]

#predictions = cv2.resize(predictions, (540, 960))

#print predictions.head()
#print np.sum(predictions)

#predictions[predictions > 0.5] = 1
#predictions[predictions < 1] = 0

#print paths[0]
cv2.imwrite('test2.png', np.multiply(predictions, 255))

#error = np.abs(predictions - gt)
#print predictions.shape

#numberOfPixels = predictions.shape[0] * predictions.shape[1]
#error_percentage = np.sum(error) * 100.0 / numberOfPixels
#average_error_percentage += error_percentage
#print "Error of image %d = %f" % (iteration, error_percentage)

#iteration += 1
#break

#print "Average error = %f" % (average_error_percentage / iteration)
