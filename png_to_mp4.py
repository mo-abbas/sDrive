import os

videos = ['back_left', 'back_right', 'left_left', 'left_right', 'front_left', 'front_right', 'right_left', 'right_right']

for video in videos:
	os.system('ffmpeg -f image2 -r 25 -i ' + video + '_%05d.png -c:v libx264 -crf 20 ' + video + '.mp4')