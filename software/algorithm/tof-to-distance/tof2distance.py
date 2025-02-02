import os
import matplotlib.pyplot as plt
import numpy as np
from sklearn.linear_model import LinearRegression

DATA_DIR = "distance"
xss = []
yss = []

for filename in os.listdir(DATA_DIR):
    print(filename)
    xs = []
    ys = []
    y = float(filename)
    f = open(DATA_DIR + "/" + filename, 'r')
    for i in range(0, 50):
        line = f.readline()
        print(str(i) + ": " + line)
        x = float(line)
        print(str(x) + ": " + str(y))
        xs.append(x)
        ys.append(y)
    plt.plot(xs,ys, 'x')
    xss.append(xs)
    yss.append(ys)

x = np.array(xss).reshape((-1, 1))
y = np.array(yss).reshape((-1))

model = LinearRegression()
model.fit(x, y)

x = np.arange(0, 3500, 1)
y = model.predict(np.array(x).reshape((-1, 1)))
plt.plot(x,y)

# x = [352]
# y = model.predict(np.array(x).reshape((-1, 1)))
# print(y)
print(model.coef_[0])
print(model.intercept_)

# x = 368
# y = model.coef_[0] * x + model.intercept_
# print(str(x) + ": " + str(y))

error_sum = 0
error_count = 0

for filename in os.listdir(DATA_DIR):
    print(filename)
    xs = []
    ys = []
    y = float(filename)
    f = open(DATA_DIR + "/" + filename, 'r')
    for i in range(0, 50):
        line = f.readline()
        print(str(i) + ": " + line)
        x = float(line)
        est_y = model.coef_[0] * x + model.intercept_
        error_sum += abs(est_y - y)
        error_count += 1

print("mean error:" + str(error_sum/error_count))
plt.show()