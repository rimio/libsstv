import sys
import numpy as np
import matplotlib.pyplot as plt

f = open('test.csv')
lines = [int(l.replace('\n', '')) for l in f.readlines()[:-2]]
f.close()

s = np.array(lines)
plt.plot(s)
plt.show()
