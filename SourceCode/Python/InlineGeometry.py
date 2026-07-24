# PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
from copy import deepcopy
from sys import argv
from glm import *
from math import *
from enum import Enum, auto

# Constants
HSQRT2 = sqrt(2) / 2
SUBDIV_NUM = 3
FILE_NAME = "InlineGeometry.h"

# Types and functions
class Vertex:
   def __init__(self, pos = vec3(), t = vec3(), n = vec3(), uv = vec3()):
      self.pos = pos
      self.t = t
      self.n = n
      self.uv = uv

def RotateX_HalfPi(v : Vertex):
   temp = deepcopy(v)
   v.pos.y = -temp.pos.z
   v.pos.z = temp.pos.y
   v.t.y = -temp.t.z
   v.t.z = temp.t.y
   v.n.y = -temp.n.z
   v.n.z = temp.n.y

def LerpPoint(v1 : Vertex, v2 : Vertex):
   v3 = Vertex()
   v3.pos = normalize(v1.pos + v2.pos);
   v3.t = v1.t + v2.t
   v3.n = v1.n + v2.n
   v3.uv = (v1.uv + v2.uv) / 2;
   return v3

def LerpPointSphere(v1 : Vertex, v2 : Vertex):
   v3 = LerpPoint(v1, v2)
   v3.pos = normalize(v3.pos)
   # ====== Tangent Handling ======
   if v3.t == vec3(0,0,0): # Top and botom vertices
      if v1.pos.x < 0:
         v3.t = vec3(1,0,0)
      else:
         v3.t = vec3(-1,0,0)
   else:
      if v3.pos.z == 0: # Verge vertices
         if(v3.pos.x < 0):
            v3.t = vec3(0,0,-1)
         else:
            v3.t = vec3(0,0,1)
      else: # Other vertices
         v3.t = normalize(v3.t)
   # ====== Normal Handling =======
   if v3.n == vec3(0,0,0):
      if v3.pos.y > 0:
         v3.n = vec3(0,1,0)
      else:
         v3.n = vec3(0,-1,0)
   else:
      v3.n = normalize(v3.n)
   # ==============================
   return v3

def LerpPointDisc(v1 : Vertex, v2 : Vertex):
   v3 = LerpPoint(v1, v2)
   v3.pos = normalize(v3.pos)
   v3.t = normalize(v3.t)
   v3.n = normalize(v3.n)
   return v3

def SubDivSphere(v : list[Vertex], i : list[int]):
   newI : list[int]  = []
   iCount = len(i)
   for j in range(0, iCount, 3):
         i0 = i[j]
         i1 = i[j+1]
         i2 = i[j+2]
         v0 = LerpPointSphere(v[i0], v[i1])
         v1 = LerpPointSphere(v[i1], v[i2])
         v2 = LerpPointSphere(v[i2], v[i0])
         iOffset = len(v)
         v.extend([v0, v1, v2])
         newI.extend([i0, iOffset, iOffset+2, iOffset, i1, iOffset + 1, iOffset + 2, iOffset + 1, i2, iOffset, iOffset +1, iOffset + 2])
   i.clear()
   i.extend(newI)

def SubDivDisc(v : list[Vertex]):
   newV : list[Vertex] = []
   vCount = len(v)
   for j in range(0, vCount - 1):
      v3 = LerpPointDisc(v[j], v[j+1])
      newV.extend([v[j], v3])
   newV.append(v[-1])
   v.clear()
   v.extend(newV)

def SeralizeHead():
   mode = "w"
   with open(FILE_NAME, mode, encoding="utf-8") as f:
      f.write("// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.\n")
      f.write("#include <array>\n\n")
      f.write("// Internal linkage.\n")
      f.write("namespace\n{\n")

def SeralizeRear():
   mode = "a"
   with open(FILE_NAME, mode, encoding="utf-8") as f:
      f.write("}")

def SerializeData(name : str, v : list[Vertex], i : list[int]):
   mode = "a"
   with open(FILE_NAME, mode, encoding="utf-8") as f:
      f.write(f"   constexpr std::array<StandardVertex, {len(v)}> {name}V =\n")
      f.write("   {{\n")
      for vtx in v:
         f.write(f"      {{XMFLOAT4A{{{vtx.pos.x:.6f}f,{vtx.pos.y:.6f}f,{vtx.pos.z:.6f}f,0.f}}, {{}}, {{}}, XMVF2H({vtx.uv.x:.6f}f,{vtx.uv.y:.6f}f,0.f,0.f),"
                  f" {{}}, XMVF2H({vtx.n.x:.6f}f,{vtx.n.y:.6f}f,{vtx.n.z:.6f}f,0.f), XMVF2H({vtx.t.x:.6f}f,{vtx.t.y:.6f}f,{vtx.t.z:.6f}f,0.f)}},\n");
      f.write("   }};\n\n")
      f.write(f"   constexpr std::array<uint32_t, {len(i)}> {name}I =\n")
      f.write("   {\n")
      for j in range(len(i)):
         if j%24 == 0:
            f.write("      ")
         f.write(f"{i[j]},")
         if j%24 == 23 or j == len(i)-1:
            f.write("\n")
      f.write("   };\n\n")
   # Print info
   print(f"### LOG: {name} generated.\tTriangleCount={(len(i) / 3):.0f}")

# Generation logic
print("### *** PILLOW BASICS BUILTIN GEOMETRY DATA GENERATOR VER 1.0 ***")
print("### LOG: Use the 1st cmd arg to set SUBDIV_NUM.")
try:
   if len(argv) > 1:
      SUBDIV_NUM = int(argv[1])
except Exception as e:
        print(f"### LOG: Wrong cmd arg. SUBDIV_NUM keeps the default.")
print(f"### LOG: SUBDIV_NUM={SUBDIV_NUM}")
print(f"### LOG: Starting generation...")

SeralizeHead()

# Quad
v = [
   Vertex(pos=vec3(-HSQRT2, 0, -HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(0,0)),
   Vertex(pos=vec3(-HSQRT2, 0, HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(0,1)),
   Vertex(pos=vec3(HSQRT2, 0, HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(1,1)),
   Vertex(pos=vec3(HSQRT2, 0, -HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(1,0)),
]
i = [0, 1, 2, 0, 2, 3]
SerializeData("Quad", v, i)

# Cube
v = [
   # Front
   Vertex(pos=vec3(-HSQRT2,-HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,0,-1), uv=vec2(0,0)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,0,-1), uv=vec2(0,1)),
   Vertex(pos=vec3( HSQRT2, HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,0,-1), uv=vec2(1,1)),
   Vertex(pos=vec3( HSQRT2,-HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,0,-1), uv=vec2(1,0)),
   # Back
   Vertex(pos=vec3( HSQRT2,-HSQRT2, HSQRT2), t=vec3(-1,0,0), n=vec3(0,0,1), uv=vec2(0,0)),
   Vertex(pos=vec3( HSQRT2, HSQRT2, HSQRT2), t=vec3(-1,0,0), n=vec3(0,0,1), uv=vec2(0,1)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2, HSQRT2), t=vec3(-1,0,0), n=vec3(0,0,1), uv=vec2(1,1)),
   Vertex(pos=vec3(-HSQRT2,-HSQRT2, HSQRT2), t=vec3(-1,0,0), n=vec3(0,0,1), uv=vec2(1,0)),
   # Left
   Vertex(pos=vec3(-HSQRT2,-HSQRT2, HSQRT2), t=vec3(0,0,-1), n=vec3(-1,0,0), uv=vec2(0,0)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2, HSQRT2), t=vec3(0,0,-1), n=vec3(-1,0,0), uv=vec2(0,1)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2,-HSQRT2), t=vec3(0,0,-1), n=vec3(-1,0,0), uv=vec2(1,1)),
   Vertex(pos=vec3(-HSQRT2,-HSQRT2,-HSQRT2), t=vec3(0,0,-1), n=vec3(-1,0,0), uv=vec2(1,0)),
   # Right
   Vertex(pos=vec3( HSQRT2,-HSQRT2,-HSQRT2), t=vec3(0,0,1), n=vec3(1,0,0), uv=vec2(0,0)),
   Vertex(pos=vec3( HSQRT2, HSQRT2,-HSQRT2), t=vec3(0,0,1), n=vec3(1,0,0), uv=vec2(0,1)),
   Vertex(pos=vec3( HSQRT2, HSQRT2, HSQRT2), t=vec3(0,0,1), n=vec3(1,0,0), uv=vec2(1,1)),
   Vertex(pos=vec3( HSQRT2,-HSQRT2, HSQRT2), t=vec3(0,0,1), n=vec3(1,0,0), uv=vec2(1,0)),
   # Top
   Vertex(pos=vec3(-HSQRT2, HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(0,0)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2, HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(0,1)),
   Vertex(pos=vec3( HSQRT2, HSQRT2, HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(1,1)),
   Vertex(pos=vec3( HSQRT2, HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,1,0), uv=vec2(1,0)),
   # Bottom
   Vertex(pos=vec3(-HSQRT2,-HSQRT2, HSQRT2), t=vec3(1,0,0), n=vec3(0,-1,0), uv=vec2(0,0)),
   Vertex(pos=vec3(-HSQRT2,-HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,-1,0), uv=vec2(0,1)),
   Vertex(pos=vec3( HSQRT2,-HSQRT2,-HSQRT2), t=vec3(1,0,0), n=vec3(0,-1,0), uv=vec2(1,1)),
   Vertex(pos=vec3( HSQRT2,-HSQRT2, HSQRT2), t=vec3(1,0,0), n=vec3(0,-1,0), uv=vec2(1,0))
]
i = [
    0, 1, 2, 0, 2, 3,
    4, 5, 7, 5, 6, 7,
    8, 9,10, 8,10,11,
   12,13,15,13,14,15,
   16,17,18,16,18,19,
   20,21,23,21,22,23,
]
SerializeData("Cube", v, i)

# Sphere
# _________
# | \   / |
# |  \v/__|___<< v0
# |  /^\  |
# |_/___\_|
v.clear()
i.clear()
v = [
   Vertex(pos=vec3(0,0,-1), t=vec3(1,0,0), n=vec3(0,0,-1), uv=vec2(0.5,0.5)),
   Vertex(pos=vec3(-HSQRT2, -HSQRT2, 0), t=vec3(0,0,-1), n=vec3(-HSQRT2, -HSQRT2, 0), uv=vec2(0,0)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2, 0), t=vec3(0,0,-1), n=vec3(-HSQRT2, HSQRT2, 0), uv=vec2(0,1)),
   Vertex(pos=vec3(HSQRT2, HSQRT2, 0), t=vec3(0,0,1), n=vec3(HSQRT2, HSQRT2, 0), uv=vec2(1,1)),
   Vertex(pos=vec3(HSQRT2, -HSQRT2, 0), t=vec3(0,0,1), n=vec3(HSQRT2, -HSQRT2, 0), uv=vec2(1,0)),
   Vertex(pos=vec3(0,0,1), t=vec3(-1,0,0), n=vec3(0,0,1), uv=vec2(0.5,0.5)),
   Vertex(pos=vec3(HSQRT2, -HSQRT2, 0), t=vec3(0,0,1), n=vec3(HSQRT2, -HSQRT2, 0), uv=vec2(0,0)),
   Vertex(pos=vec3(HSQRT2, HSQRT2, 0), t=vec3(0,0,1), n=vec3(HSQRT2, HSQRT2, 0), uv=vec2(0,1)),
   Vertex(pos=vec3(-HSQRT2, HSQRT2, 0), t=vec3(0,0,-1), n=vec3(-HSQRT2, HSQRT2, 0), uv=vec2(1,1)),
   Vertex(pos=vec3(-HSQRT2, -HSQRT2, 0), t=vec3(0,0,-1), n=vec3(-HSQRT2, -HSQRT2, 0), uv=vec2(1,0)),
]
i = [0,1,2,0,2,3,0,3,4,0,4,1,5,6,7,5,7,8,5,8,9,5,9,6]
for j in range(SUBDIV_NUM):
    SubDivSphere(v, i)
vSphere = v.copy()
iSphere = i.copy()
SerializeData("Sphere", v, i)

# Cylinder
v.clear()
i.clear()
vDisc = [
   Vertex(pos=vec3(-HSQRT2, 0, -HSQRT2), t=vec3(HSQRT2,0,-HSQRT2), n=vec3(-HSQRT2, 0, -HSQRT2), uv=vec2(6,0)),
   Vertex(pos=vec3(-HSQRT2, 0, HSQRT2), t=vec3(-HSQRT2,0,-HSQRT2), n=vec3(-HSQRT2, 0, HSQRT2), uv=vec2(4.5,1)),
   Vertex(pos=vec3(HSQRT2, 0, HSQRT2), t=vec3(-HSQRT2,0,HSQRT2), n=vec3(HSQRT2, 0, HSQRT2), uv=vec2(3,1)),
   Vertex(pos=vec3(HSQRT2, 0, -HSQRT2), t=vec3(HSQRT2,0,HSQRT2), n=vec3(HSQRT2, 0, -HSQRT2), uv=vec2(1.5,0)),
   Vertex(pos=vec3(-HSQRT2, 0, -HSQRT2), t=vec3(HSQRT2,0,-HSQRT2), n=vec3(-HSQRT2, 0, -HSQRT2), uv=vec2(0,0)),
]
iSide = []
iTop = []
iBottom = []
for j in range(SUBDIV_NUM):
    SubDivDisc(vDisc)
discVNum = len(vDisc)
for j in range(4):
   v.extend(deepcopy(vDisc))
vSide = v[0:2 * discVNum] # Capsule can use it as well
for j in range(discVNum):
   ## Vertex buffer
   # Side face
   v[j].pos.y = 0.5
   v[discVNum + j].pos.y = -0.5
   v[j].uv.y = 1
   # Top disc
   v1 = v[2 * discVNum + j]
   v1.pos.y = 0.5
   v1.t = vec3(1,0,0)
   v1.n = vec3(0,1,0)
   v1.uv = v1.pos.xz + vec2(0.5,0.5)
   # Bottom disc
   v2 = v[3 * discVNum + j]
   v2.pos.y = -0.5
   v2.t = vec3(1,0,0)
   v2.n = vec3(0,-1,0)
   v2.uv = v2.pos.xz + vec2(0.5,0.5)
   v2.uv.y = 1 - v2.uv.y
   ## Index buffers
   # Side
   if j < discVNum - 1:
      iSide.extend([j, discVNum + j, discVNum + j + 1])
      iSide.extend([j, discVNum + j + 1, j + 1])
   # Top & bottom discs
   if j != 0 and j < discVNum - 2:
      iTop.extend([0, j, j + 1])
      iBottom.extend([discVNum, discVNum + j + 1, discVNum + j])
i.extend(iSide)
i.extend(iTop)
i.extend(iBottom)
SerializeData("Cylinder", v, i)

# Capsule
v.clear()
i.clear()
sideINum = len(iSide)
sphereVHalfNum = len(vSphere) / 2
# Rotate sphere's vertices
for j in range(0, len(vSphere)):
   RotateX_HalfPi(vSphere[j])
   if j < sphereVHalfNum:
      vSphere[j].pos.y += 0.5
   else:
      vSphere[j].pos.y -= 0.5
# Rotate sphere's indices
for j in range(0, len(iSphere)):
   iSphere[j] = iSphere[j] + sideINum
v.extend(vSide)
v.extend(vSphere)
i.extend(iSide)
i.extend(iSphere)
SerializeData("Capsule", v, i)

SeralizeRear()

print(f"### LOG: Completed, please see {FILE_NAME}.")