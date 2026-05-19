from glm import *
from math import *

# Constants
hx = sqrt(2) / 2

hy = sqrt(3) / 2

SUBDIV_NUM = 4

# Types and functions
class Vertex:   
   def __init__(self, pos = vec3(), t = vec3(), uv = vec3()):
      self.pos = pos
      self.t = t
      self.uv = uv

def MidPoint(v1 : Vertex, v2 : Vertex):
   v3 = Vertex()
   v3.pos = normalize(v1.pos + v2.pos);
   t = v1.t + v2.t
   if t == vec3(0,0,0):
      if v1.t.z < 1:
         v3.t = vec3(1,0,0)
      else:
         v3.t = vec3(-1,0,0)
   else:
      v3.t = normalize(t)
   v3.uv = (v1.uv + v2.uv) / 2;
   return v3

def SubDivide(v : list[Vertex], i : list[int]):
   newI : list[int]  = []
   iCount = len(i)
   for j in range(0, iCount, 3):
         v0 = MidPoint(v[i[j]], v[i[j+1]])
         v1 = MidPoint(v[i[j+1]], v[i[j+2]])
         v2 = MidPoint(v[i[j+2]], v[i[j]])
         vOffset = len(v)
         v.extend([v0, v1, v2])
         newI.extend([i[j], vOffset, vOffset+2, vOffset, i[j+1], vOffset + 1, vOffset + 2, vOffset + 1, i[j+2], vOffset, vOffset +1, vOffset + 2])
   i.clear()
   i.extend(newI)

def SubDivide2(v : list[Vertex]):
   newI : list[int]  = []
   newV : list[Vertex] = []
   vCount = len(v)
   for j in range(0, vCount - 1):
      newV.append(v[j])
      newV.append(MidPoint(v[j], v[j+1]))
   newV.append(v[-1])
   v.clear()
   v.extend(newV)
   vCount = len(v)
   for j in range(0, vCount - 2):
      newI.extend([0, j, j+2])
   return newI

# Generation logic

# Sphere
v = [
   Vertex(pos=vec3(0,0,-1), t=vec3(1,0,0), uv=vec2(0.5,0.5)),
   Vertex(pos=vec3(-hx, -hx, 0), t=vec3(0,0,-1), uv=vec2(0,0)),
   Vertex(pos=vec3(-hx, hx, 0), t=vec3(0,0,-1), uv=vec2(0,1)),
   Vertex(pos=vec3(hx, hx, 0), t=vec3(0,0,1), uv=vec2(1,1)),
   Vertex(pos=vec3(hx, -hx, 0), t=vec3(0,0,1), uv=vec2(1,0)),
   Vertex(pos=vec3(0,0,1), t=vec3(-1,0,0), uv=vec2(0.5,0.5)),
   Vertex(pos=vec3(hx, -hx, 0), t=vec3(0,0,1), uv=vec2(0,0)),
   Vertex(pos=vec3(hx, hx, 0), t=vec3(0,0,1), uv=vec2(0,1)),
   Vertex(pos=vec3(-hx, hx, 0), t=vec3(0,0,-1), uv=vec2(1,1)),
   Vertex(pos=vec3(-hx, -hx, 0), t=vec3(0,0,-1), uv=vec2(1,0)),
]
i = [0,1,2,0,2,3,0,3,4,0,4,1,5,6,7,5,7,8,5,8,9,5,9,6]

for j in range(SUBDIV_NUM):
    SubDivide(v, i)
    print(f" sphere stage={j+1} vCount={len(v)}, TriCount={(len(i) / 3):.0f}")

with open("Geometry.txt", "w", encoding="utf-8") as f:
   f.write(f"constexpr StandardVertex SphereV[{len(v)}] =\n")
   f.write("{\n")
   for vtx in v:
      f.write((f"   {{ XMFLOAT4A{{{vtx.pos.x:.5f}f,{vtx.pos.y:.5f}f,{vtx.pos.z:.5f}f,0.f}}, {{}}, {{}}, XMVF2H({vtx.uv.x:.5f}f,{vtx.uv.y:.5f}f,0.f,0.f),"
               f" {{}}, XMVF2H({vtx.pos.x:.5f}f,{vtx.pos.y:.5f}f,{vtx.pos.z:.5f}f,0.f), XMVF2H({vtx.t.x:.5f}f,{vtx.t.y:.5f}f,{vtx.t.z:.5f}f,0.f) }},\n"));
   f.write("}\n\n")
   f.write(f"constexpr uint32_t SphereI[{len(i)}] =\n")
   f.write("{\n")
   for j in range(len(i)):
      if j%24 == 0:
         f.write("   ")
      f.write(f"{i[j]},")
      if j%24 == 23:
         f.write("\n")
   f.write("}")

# Cylinder
v_Circle = [
   Vertex(pos=vec3(-hy,0,-0.5), t=vec3(hy,0,-0.5), uv=vec2(3,1)),
   Vertex(pos=vec3(0,0,1), t=vec3(-1,0,0), uv=vec2(2,1)),
   Vertex(pos=vec3(hy,0,-0.5), t=vec3(hy,0,0.5), uv=vec2(1,1)),
   Vertex(pos=vec3(-hy,0,-0.5), t=vec3(hy,0,-0.5), uv=vec2(0,1)),
]
i : list[int]

for j in range(SUBDIV_NUM):
    i = SubDivide2(v_Circle, i)
    print(f" circle stage={j+1} CircleVCount={len(v)}, TriCount={(len(i) / 3):.0f}")
discVCount = len(v_Circle)
v : list[Vertex] = []
v.extend(v_Circle, v_Circle, v_Circle, v_Circle)
for j in range(discVCount):
   # Top disc
   v[j].pos.y = 0.5
   # Bottom disc
   v[discVCount + j].pos.y = -0.5
   # Side face
   v[2*discVCount + j].pos.y = 0.5
   v[3*discVCount + j].pos.y = -0.5

# Cone

# Capsule