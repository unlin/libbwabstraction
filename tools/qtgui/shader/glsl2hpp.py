import os
from functools import reduce

def glsl2hpp(hpp_file, glsl_filepath):
    glsl_var_name = glsl_filepath.replace('.', '_')
    hpp_file.write('const char* %s = ' % glsl_var_name)
    with open(glsl_filepath, 'r') as glsl_file:
        lns = glsl_file.readlines()
    max_len = len(reduce(lambda x, y: x if len(x) > len(y) else y, lns))
    for ln in lns:
        outln = ln[:-1].replace('\t', '    ')
        outln += ' ' * (max_len - len(outln))
        print(outln)
        hpp_file.write('\n"%s\\n"' % outln)
    hpp_file.write(';\n\n')

if __name__ == '__main__':
    dirname = os.path.dirname(__file__)
    hpp_filepath = os.path.join(dirname, 'shaders.cpp')
    with open(hpp_filepath, 'w') as hpp_file:
        glsl2hpp(hpp_file, 'colored.fs.glsl')
        glsl2hpp(hpp_file, 'colored.vs.glsl')
        glsl2hpp(hpp_file, 'manmadeobj.fs.glsl')
        glsl2hpp(hpp_file, 'manmadeobj.vs.glsl')
        glsl2hpp(hpp_file, 'texquad.fs.glsl')
        glsl2hpp(hpp_file, 'texquad.vs.glsl')