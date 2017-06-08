import bge
import os

print("hello!")
print(os.getcwd())

bge.constraints.exportBulletFile("arena-2.bullet")