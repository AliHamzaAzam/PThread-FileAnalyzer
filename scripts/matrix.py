import numpy as np

data = np.random.rand(20000,20000)
# save the array as a npy file
np.save("../data/Task5.npy", data)
# print sum, max and min values in the array
data_sum = np.sum(data)
data_max = np.max(data)
data_min = np.min(data)
print ("Sum of all values in the array: ", data_sum)
print ("Maximum value in the array: ", data_max)
print ("Minimum value in the array: ", data_min)