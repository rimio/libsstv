import matplotlib.pyplot as plt
import numpy as np
import wave
import sys


spf = wave.open('test.wav','r')

#Extract Raw Audio from Wav File
signal = spf.readframes(-1)
signal = np.fromstring(signal, 'Int16')
signal = signal.astype(np.float32)


#If Stereo
if spf.getnchannels() == 2:
    print('Just mono files')
    sys.exit(0)

plt.figure(1)
plt.title('Signal Wave...')
plt.plot(signal)
plt.show()

plt.specgram(signal, Fs=48000)
plt.show()