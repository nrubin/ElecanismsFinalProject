# # import numpy as np
# # import matplotlib.pyplot as plt


# # r = np.arange(0, 3.0, 0.01)
# # theta = 2 * np.pi * r

# # ax = plt.subplot(111, polar=True)
# # ax.plot(theta, r, color='r', linewidth=3)
# # ax.set_rmax(2.0)
# # ax.grid(True)

# # ax.set_title("A line plot on a polar axis", va='bottom')
# # plt.show()

# import matplotlib.pyplot as plt
# import numpy as np

# # Generate some data...
# # Note that all of these are _2D_ arrays, so that we can use meshgrid
# # You'll need to "grid" your data to use pcolormesh if it's un-ordered points
# theta, r = np.mgrid[0:2*np.pi:20j, 0:1:10j]
# z = np.random.random(theta.size).reshape(theta.shape)


# fig, (ax1, ax2) = plt.subplots(ncols=2, subplot_kw=dict(projection='polar'))


# ax1.scatter(theta.flatten(), r.flatten(), c=z.flatten())
# ax1.set_title('Scattered Points')

# ax2.pcolormesh(theta, r, z)
# ax2.set_title('Cells')

# for ax in [ax1, ax2]:
#     ax.set_ylim([0, 1])
#     ax.set_yticklabels([])
# print type(r)
# print type(theta)

# plt.show()
import numpy as np
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import random

def fun(x, y):
  return x**2 + y

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
x = y = np.arange(-3.0, 3.0, 0.05)
X, Y = np.meshgrid(x, y)
zs = np.array([fun(x,y) for x,y in zip(np.ravel(X), np.ravel(Y))])
Z = zs.reshape(X.shape)

ax.plot_surface(X, Y, Z)

ax.set_xlabel('X Label')
ax.set_ylabel('Y Label')
ax.set_zlabel('Z Label')

plt.show()