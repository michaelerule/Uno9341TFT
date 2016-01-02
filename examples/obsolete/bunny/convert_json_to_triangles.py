#!/usr/bin/env ipython
'''
Meshlab can output meshes as JSON formmatted triangle and vertex data.
We need to wrangle this into the triangle format for the Arduino3D 
models.

JSON is similar enough to Python that we can parse it directly just by
definind the values true, false, and null, to their Python equivalents. 

Cat a JSON model over STDIN

Outputs C header file for object data. 

Example usage
cat ./bunny_ultralowpoly_1.json | ./convert_json_to_triangles.py
'''

import sys, os
import numpy as np
from numpy import *

def recenter(p):
    '''
    Move points such that min(points) = -max(points)
    '''
    return p-np.min(p)-(np.max(p)-np.min(p))/2.0

def dense_matrix_print_format(x,WIDTH=100):
    lines = ''
    s = ''
    for e in x:
        s += str(e)+','
        if len(s)>WIDTH:
            lines+=s+'\n'
            s=''
    lines+=s+'\n'
    return lines

# Parse JSON using eval. 
false = False
true  = True
null  = None
data = ''.join(sys.stdin.readlines())
model = eval(data)

# Hard coded locations of the data -- may not be to standard, brittle.
vertices  = array(model['vertices'][0]['values'])
normals   = array(model['vertices'][1]['values'])
triangles = array(model['connectivity'][0]['indices'])

# some sanity checks
print '// THERE MUST BE NO MORE THAN 256 VERTICES'
assert len(vertices)%3   == 0
assert len(triangles)%3  == 0
assert len(vertices)//3  <= 256
assert np.max(triangles) <= 256

MAX_SCALE = 127.0
NORMAL_SCALE = 127.0;

NVERTICES  = len(vertices)//3
NTRIANGLES = len(triangles)//3
triangles = uint8(triangles)


#compute face normals
facenormals = []
for i in range(NTRIANGLES):
    t1,t2,t3 = triangles[i*3:][:3]
    p ,q ,r  = [vertices[t*3:][:3] for t in [t1,t2,t3]]
    # Normal (unnormalized) is the cross product of edges
    nx = (q[1]-p[1])*(r[2]-p[2])-(r[1]-p[1])*(q[2]-p[2])
    ny = (q[2]-p[2])*(r[0]-p[0])-(r[2]-p[2])*(q[0]-p[0])
    nz = (q[0]-p[0])*(r[1]-p[1])-(r[0]-p[0])*(q[1]-p[1])
    n = array([nx,ny,nz])
    n *= NORMAL_SCALE / sqrt(dot(n,n))
    facenormals.extend(n)
facenormals = array(facenormals)
facenormals = int8(facenormals)

# quantize the vertices to fit into int8
x = vertices[0::3]
y = vertices[1::3]
z = vertices[2::3]
x = recenter(x)
y = recenter(y)
z = recenter(z)
scale = np.max([np.max(x),np.max(y),np.max(z)])
print '// scale=',scale
rescale = MAX_SCALE / scale
x *= rescale
y *= rescale
z *= rescale
x = int8(x+0.5)
y = int8(y+0.5)
z = int8(z+0.5)
vertices = int8(vertices)
vertices[0::3] = x
vertices[1::3] = y
vertices[2::3] = z


#quantize the vertex normals
normals = int8(NORMAL_SCALE*normals)




print '#define NTRIANGLES %d'%NTRIANGLES
print '#define NVERTICES  %d'%NVERTICES
print 'PROGMEM const int8_t vertices[NVERTICES*3]={'
print dense_matrix_print_format(vertices)+'};'
print 'PROGMEM const int8_t normals[NVERTICES*3]={'
print dense_matrix_print_format(normals)+'};'
print 'PROGMEM const uint8_t triangles[NTRIANGLES*3]={'
print dense_matrix_print_format(triangles)+'};'
print 'PROGMEM const int8_t facenormals[NTRIANGLES*3]={'
print dense_matrix_print_format(facenormals)+'};'

# Get a list of edges for fast transparent mesh rendering
def add_edge(p1,p2,edge_set):
    if p1>p2:
        p1,p2 = p2,p1
    edge_set.add((p1,p2))
# compile triangle edges
triangle_edges = []
for i in range(0,NTRIANGLES): 
    p1,p2,p3 = triangles[i*3:][:3];
    this_triangle = set()
    add_edge(p1,p2,this_triangle)
    add_edge(p1,p3,this_triangle)
    add_edge(p3,p2,this_triangle)
    triangle_edges.append(this_triangle)
# construct set of unique edges
edge_set = set()
for triangle in triangle_edges:
    edge_set |= triangle
edges = sorted(list(edge_set))

NEDGES = len(edges)
print '#define NEDGES  %d'%NEDGES
print 'PROGMEM const uint8_t edges[NEDGES*2]={'
print dense_matrix_print_format(ravel(uint8(edges)))+'};'


print '''
Model model_data;
void init_model() {
    model_data.NVertices     = NVERTICES;
    model_data.NFaces        = NTRIANGLES;
    model_data.NEdges        = NEDGES;
    model_data.vertices      = vertices;
    model_data.edges         = edges;
    model_data.faces         = triangles;
    model_data.vertexNormals = normals;
    model_data.faceNormals   = facenormals;
}
'''

# code below previous used to generating mapping from edges to triangles.
# however, this takes up too much space in practice and is no longer used.
'''
if len(triangles)//3 <= 256:
    
    # Outline / silouette drawing requires generating some data structures 
    # offline, as there is not enough memory on the arduino to compute these
    # structures in RAM. To draw the outline, we need to draw all edges that
    # border two triangles with opposite signed Z components of their normal
    # vector. We have about 200 bytes of ram to spare. If we have 500 triangles,
    # we can precompute the face signs and store them in a bit vector. We need
    # about 64 bytes for this. Then, we need to iterate over all edges and
    # test whether they border a sign-change. To do this, we need a mapping 
    # from edges to triangles. A list of all unique edges, including start and
    # end vertex, and index in to the triangle array, is needed. For this, 
    # we need to limit the model to 256 triangles. 
    # 
    # Stages:
    # 1 construct the edge set. 
    #   For every triangle
    #   For all pairs
    #   Sort pair
    #   Add to edge set
    # 2 collect indecies from edges to triangles. 

    def add_edge(p1,p2,edge_set):
        if p1>p2:
            p1,p2 = p2,p1
        edge_set.add((p1,p2))

    # compile triangle edges
    triangle_edges = []
    for i in range(0,NTRIANGLES): 
        p1,p2,p3 = triangles[i*3:][:3];
        this_triangle = set()
        add_edge(p1,p2,this_triangle)
        add_edge(p1,p3,this_triangle)
        add_edge(p3,p2,this_triangle)
        triangle_edges.append(this_triangle)

    # construct set of unique edges
    edge_set = set()
    for triangle in triangle_edges:
        edge_set |= triangle
    edges = sorted(list(edge_set))

    # construct a map from edges to triangles that contain them 
    from collections import defaultdict
    triangle_map = defaultdict(list)
    edge_map = []
    for e in edges:
        for i,triangle in enumerate(triangle_edges):
            if e in triangle:
                triangle_map[e].append(i)
        # for valid models, ever edge belongs to two and only two triangles
        if len(triangle_map[e])==1: triangle_map[e].append(triangle_map[e][0])
        assert len(triangle_map[e])==2
        edge_map.extend(e)
        edge_map.extend(sorted(triangle_map[e]))

    NEDGES = len(edges)

    print '#define NEDGES  %d'%NEDGES
    print 'PROGMEM const uint8_t edgemap[NEDGES*4]={'
    print dense_matrix_print_format(edge_map)+'};'

'''



