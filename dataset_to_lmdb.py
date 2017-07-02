import sys
sys.path.insert(0, '/mnt/eg5_data/caffe/python/')
import caffe
import lmdb
import cv2
import PIL
import numpy as np
import argparse
from scipy.misc import imread, imsave

initialMapSize = 10 * (2**20) # 10MB
batchSize = 100
def test():
    lmdbEnvironment = lmdb.open(datasetLmdbFilename)
    transaction = lmdbEnvironment.begin()
    cursor = transaction.cursor()
    datum = caffe.io.caffe_pb2.Datum()
    idx = 0
    for key, value in cursor:
        datum.ParseFromString(value)
        imgLimits = 3 * datum.width * datum.height
        image = \
            np.fromstring(datum.data[:imgLimits], dtype=np.uint8) \
            .reshape(3, datum.height, datum.width)
        label = \
            np.fromstring(datum.data[imgLimits:], dtype=np.uint8) \
            .reshape(datum.height, datum.width)
        image = np.rollaxis(np.rollaxis(image, 2), 2)
        fileNames = key.split('\t')
        cv2.imwrite('%s/%07d-img.png' % (testOutputFolder, idx), image)
        cv2.imwrite('%s/%07d-lbl.png' % (testOutputFolder, idx), label)
        idx += 1
        # no need for extracting all the data during testing.
        if idx > 4:
            return

def read_example(line):
    paths = line.split('\t')

    #make the image channels majors
    image = np.rollaxis(cv2.imread(paths[0]), 2)
    label = imread(paths[1])	#use scipy to read the binary image
    height = image.shape[1]
    width = image.shape[2]

    # rightImage = rightImage[0]
    imageLinear = np.array(image.ravel(), dtype=np.uint8)
    labelLinear = np.array(label.ravel(), dtype=np.uint8)
    imageBytes = imageLinear.tobytes()
    labelBytes = labelLinear.tobytes()
    data = imageBytes + labelBytes

    #3 channels for the image and 1 channel for the labels
    if len(data) != 4 * width * height:
        raise Exception('invalid images were selected')
    return data, (height, width)

def commit(lmdbEnvironment, transactionData):
    transaction = lmdbEnvironment.begin(write=True)
    try:
        for example in transactionData:
            transaction.put(example[0], example[1])
        transaction.commit()
    except lmdb.MapFullError:   # database full, double the size and commit again
        transaction.abort()
        currentMapSize = lmdbEnvironment.info()['map_size']
        lmdbEnvironment.set_mapsize(currentMapSize * 2)
        commit(lmdbEnvironment, transactionData)
def convert():
    dataList = open(datasetListFilename, 'r')
    lmdbEnvironment = lmdb.open(datasetLmdbFilename, map_size=initialMapSize)
    transactionData = []
    lineNumber = 0
    for line in dataList:
        data, shape = read_example(line.strip())
        datum = caffe.proto.caffe_pb2.Datum()
        datum.channels = 4  # 3 for each image and a channel for the labels
        datum.height = shape[0]
        datum.width = shape[1]
        datum.data = data
        datum.label = 0     # labels aren't used
        key = ('%08d' % lineNumber)
        value = datum.SerializeToString()
        transactionData.append((key, value))
        lineNumber += 1
        if lineNumber % batchSize == 0:
            commit(lmdbEnvironment, transactionData)
            transactionData = []
            print "Finished", lineNumber, "lines."
    if lineNumber % batchSize != 0:
        commit(lmdbEnvironment, transactionData)
        print "Finished", lineNumber, "lines."
    lmdbEnvironment.close()
    print "Done"

parser = argparse.ArgumentParser()
parser.add_argument("type", choices=["main", "test"],
        help="Operation done by the script")
parser.add_argument("lmdbFilename",
        help="The path of the lmdb database folder")
parser.add_argument("-l", "--list",
        help="The path for the data list to insert in the database (ignored during test)")
parser.add_argument("-t", "--test",
        help="The path for the test output folder")
args = parser.parse_args()
datasetListFilename = args.list
datasetLmdbFilename = args.lmdbFilename
testOutputFolder = args.test
if args.type == "main":
    if not datasetListFilename:
        print "Missing the path for the data list"
    else:
        convert()
elif args.type == "test":
    if not testOutputFolder:
        print "Missing the path for the test output folder"
    else:
        test()
