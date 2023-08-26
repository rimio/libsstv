import sys
import numpy as np
import datetime

f = open('src/luts.c', 'w')

# header
f.write('/*\n')
f.write(' * Copyright (c) 2018-{} Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>\n'.format(datetime.datetime.now().date().strftime("%Y")))
f.write(' *\n')
f.write(' * libsstv is free software; you can redistribute it and/or modify\n')
f.write(' * it under the terms of the MIT license. See LICENSE for details.\n')
f.write(' */\n\n')
f.write('#include "luts.h"\n\n')

x = np.linspace(0.0, 2 * np.pi, num=1024, endpoint=False)
sn = np.sin(x)

# SSTV_SIN_INT10_INT8
f.write('int8_t SSTV_SIN_INT10_INT8[1024] = { ')
for s in sn * 127:
    f.write(str(int(np.around(s))) + ', ')
f.write('};\n\n')

# SSTV_SIN_INT10_UINT8
f.write('uint8_t SSTV_SIN_INT10_UINT8[1024] = { ')
for s in (sn + 1) / 2 * 255:
    f.write(str(int(np.around(s))) + ', ')
f.write('};\n\n')

# SSTV_SIN_INT10_INT16
f.write('int16_t SSTV_SIN_INT10_INT16[1024] = { ')
for s in sn * 32767:
    f.write(str(int(np.around(s))) + ', ')
f.write('};\n\n')

f.close()
