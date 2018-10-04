# cython: c_string_type=str, c_string_encoding=ascii
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.map cimport map

class Dict(dict):
    def __init__(self, args = {}):
        super(Dict,self).__init__(args)

class List(list):
    def __init__(self, args = ()):
        super(List,self).__init__(args)

cdef extern object py_new_bool(int b):
    if b:
        return True
    return False

cdef extern object py_new_none():
    return None

cdef extern object py_new_string(string& s):
    return s

cdef extern object py_new_int(int n):
    return n

cdef extern object py_new_int64(long long n):
    return n

cdef extern object py_new_uint64(unsigned long long n):
    return n

cdef extern object py_new_float(double n):
    return float(n)

cdef extern object array_create():
    return List()

cdef extern void array_append(object arr, object v):
    arr.append(v)

cdef extern void array_append_string(object arr, string& s):
    arr.append(s)

cdef extern void array_append_int(object arr, int n):
    arr.append(n)

cdef extern void array_append_double(object arr, double n):
    arr.append(n)

cdef extern void array_append_uint64(object arr, unsigned long long n):
    arr.append(n)

cdef extern int array_length(object arr):
    return len(arr)

cdef extern object dict_create():
    return Dict()

cdef extern void dict_add(object d, object key, object value):
    d[key] = value


