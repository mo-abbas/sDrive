import sys
#sys.path.insert(0, 'F:/caffe/x64/Release/')
sys.path.insert(0, '/mnt/eg5_data/caffe/python/')
import caffe
import lmdb
import cv2
import PIL
import numpy as np
import argparse

datasetListFilename = None
datasetLmdbFilename = None
testOutputFolder = None

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
        img1Limits = 3 * datum.width * datum.height
        img2Limits = img1Limits * 2
        image1 = \
            np.fromstring(datum.data[:img1Limits], dtype=np.uint8) \
            .reshape(3, datum.height, datum.width)
        image2 = \
            np.fromstring(datum.data[img1Limits:img2Limits], dtype=np.uint8) \
            .reshape(3, datum.height, datum.width)
        disparity = \
            np.fromstring(datum.data[img2Limits:], dtype=np.uint16) \
            .reshape(datum.height, datum.width)

        image1 = np.rollaxis(np.rollaxis(image1, 2), 2)
        image2 = np.rollaxis(np.rollaxis(image2, 2), 2)

        disparityAsFloat = -1.0 * np.array(disparity, dtype=np.float32) / 32.0
        fileNames = key.split('\t')
        cv2.imwrite('%s/%07d-imgL.ppm' % (testOutputFolder, idx), image1)
        cv2.imwrite('%s/%07d-imgR.ppm' % (testOutputFolder, idx), image2)
        write_pfm(disparityAsFloat, '%s/%07d-gt.pfm' % (testOutputFolder, idx))
        idx += 1

        # no need for extracting all the data during testing.
        if idx > 4:
            return

def write_pfm(image, path):
    file = open(path, 'wb')
    image = np.flipud(image)

    height, width = image.shape

    file.write('Pf\n')
    file.write('%d %d\n' % (width, height))

    endian = image.dtype.byteorder

    scale = 1.0
    if endian == '<' or (endian == '=' and sys.byteorder == 'little'):
        scale = -1.0

    file.write('%f\n' % scale)
    image.tofile(file)

def read_pfm(path):
    data = open(path, 'rb')     #r -> read, b->binary
    data.readline()     #contains "Pf", read and ignore
    dimensions = data.readline().strip().split()
    width, height = map(int, dimensions)
    scale = float(data.readline().strip())
    endian = '<' if (scale < 0) else '>'

    image = np.fromfile(data, endian + 'f')
    shape = (height, width, 1)

    image = np.reshape(image, shape)    # the image is loaded as a one long array
    image = np.flipud(image)    # pfm origin is at bottom left

    return image

def read_example(line):
    paths = line.split('\t')

    # make the images channel major
    leftImage = np.rollaxis(cv2.imread(paths[0]), 2)
    rightImage =np.rollaxis(cv2.imread(paths[1]), 2)

    height = leftImage.shape[1]
    width = leftImage.shape[2]

    # the -ve here is from the definition of the disparity
    # (left location - right location) which will result in -ve numbers
    # instead of the saved positive values
    # the 32 is used to decrease the lost precision when converting to int16
    disparity = -1.0 * read_pfm(paths[2]) * 32.0

    # replace nans with max int16
    disparity[np.isnan(disparity)] = np.iinfo(np.int16).max

    leftImageLinear = np.array(leftImage.ravel(), dtype=np.uint8)
    rightImageLinear = np.array(rightImage.ravel(), dtype=np.uint8)
    disparityLinear = np.array(disparity.ravel(), dtype=np.int16)

    leftImageBytes = leftImageLinear.tobytes()
    rightImageBytes = rightImageLinear.tobytes()
    disparityBytes = disparityLinear.tobytes()
    data = leftImageBytes + rightImageBytes + disparityBytes

    if len(data) != 8 * width * height:
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
        datum.channels = 7  # 3 for each image and a channel for the disparity
        datum.height = shape[0]
        datum.width = shape[1]
        datum.data = data
        datum.label = 0     # labels aren't used

        disparityFileName = line.split('\t')[-1].split('/')[-1].strip()
        key = ('%07d' % lineNumber) + '-' + disparityFileName
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

if __name__ == "__main__":
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