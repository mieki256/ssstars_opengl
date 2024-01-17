#!python
# -*- mode: python; Encoding: utf-8; coding: utf-8 -*-
# Last updated: <2024/01/13 17:37:28 +0900>
"""
font png image to c header file

Usage: python fontpng2bits.py -i input.png --label label_name > image.h

Windows10 x64 22H2 + Python 3.10.10 64bit + Pillow 10.1.0
"""

import argparse
import os
import sys
from PIL import Image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--infile", required=True, help="PNG image file")
    parser.add_argument("--label", help="symbol name")
    args = parser.parse_args()
    infile = args.infile

    if os.path.isfile(infile):
        print("/* infile: %s */" % infile)
    else:
        print("Error: Not found %s" % infile)
        sys.exit()

    if not args.label:
        label = infile.replace(".", "_")
        label = label.replace(" ", "_")
    else:
        label = args.label

    im = Image.open(infile).convert("L")
    im.point(lambda x: 0 if x < 128 else x)
    imgw, imgh = im.size
    w = int(imgw / 16)
    h = int(imgh / 6)
    print("/* source image size = %d x %d */\n" % (imgw, imgh))

    print("static int %s_width = %d;" % (label, w))
    print("static int %s_height = %d;" % (label, h))

    alen = int(w / 8) + (1 if (w % 8) != 0 else 0)
    clen = alen * h
    cnum = 16 * 6
    print("static int %s_chr_len = %d;\n" % (label, clen))

    print("static unsigned char %s[%d][%d] = {" % (label, cnum, clen))

    for c in range(16 * 6):
        bx = int(c % 16) * w
        by = int(c / 16) * h

        print("  {")

        print("    // code = 0x%02x" % (c + 0x20))

        for y in range(h - 1, -1, -1):
            buf = []
            for i in range(alen):
                buf.append(0)

            for x in range(w):
                v = im.getpixel((bx + x, by + y))
                if v > 128:
                    buf[int(x / 8)] |= 1 << (7 - (x % 8))

            s = ""
            for v in buf:
                s += "0x%s," % format(v, "02x")
            print("    %s" % s)

        print("  },")

    print("};")


if __name__ == "__main__":
    main()
