import caffe
import time
from scipy.misc import imread
import numpy as np

GPU_ID = 0
test_size = 4370
result_file = 'result.npy'
model = 'models/old/DispNetCorr1D_CVPR2016.caffemodel'
caffe.set_mode_gpu()
caffe.set_device(GPU_ID)

def read_pfm(path):
    data = open(path, 'rb')     #r -> read, b->binary
    data.readline()     #contains "Pf", read and ignore
    dimensions = data.readline().strip().split()
    width, height = map(int, dimensions)
    scale = float(data.readline().strip())
    endian = '<' if (scale < 0) else '>'

    image = np.fromfile(data, endian + 'f')
    shape = (height, width)

    image = np.reshape(image, shape)    # the image is loaded as a one long array
    image = np.flipud(image)    # pfm origin is at bottom left

    return image

def main():
    # Make classifier.
    classifier = caffe.Classifier2('deploy.prototxt', model)
    numberOfPixels = 960 * 540
    iterations = 0
    averageTime = 0.0
    results = np.zeros((test_size, 3))

    test_data_list = open('test_data.list')
    for line in test_data_list.readlines():
        files = line.strip().split('\t')

        # np.rollaxis is used to make the images channel major not row major
        left_image = np.array([np.rollaxis(imread(files[0]), 2)])
        right_image = np.array([np.rollaxis(imread(files[1]), 2)])
        disparity = read_pfm(files[2])

        start = time.time()
        predictions = classifier.forward(**{"img0": left_image, "img1": right_image})
        averageTime += time.time() - start

        error = np.abs(disparity - predictions['out'])
        onePixelError = np.sum(error > 1)
        threePixelError = np.sum(error > 3)
        fivePixelError = np.sum(error > 5)

        results[iterations, 0] = onePixelError * 100. / numberOfPixels
        results[iterations, 1] = threePixelError * 100. / numberOfPixels
        results[iterations, 2] = fivePixelError * 100. / numberOfPixels

        iterations += 1
        if iterations % 100 == 0:
            print("Classified %d images." % iterations)

    averageResults = np.sum(results, axis = 0)
    print("Test finished.")
    print("Average classification time is %.2f s." % (averageTime / iterations))
    print("Average one pixel error is %.2f" % (averageResults[0] / iterations))
    print("Average three pixel error is %.2f" % (averageResults[1] / iterations))
    print("Average five pixel error is %.2f" % (averageResults[2] / iterations))
    print("Saving results as %s." % result_file)

    np.save(result_file, results)

if __name__ == '__main__':
    main()
