#!python
# -*- mode: python; Encoding: utf-8; coding: utf-8 -*-
# Last updated: <2024/01/08 11:02:26 +0900>
"""
Drawing process like a star screensaver by PyOpenGL

Windows10 x64 22H2 + Python 3.10.10 64bit + PyOpenGL 3.1.6
"""


import sys
import math
import random
import time
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
from PIL import Image

IMG_NAME = "star.png"

SCRW, SCRH = 1280, 720
FPS = 60
OBJ_MAX = 1000

# USE_DEPTHMASK = True
USE_DEPTHMASK = False

USE_FOG = True

scr_w, scr_h = SCRW, SCRH
window = 0
texture = 0

speed = 1.0
dist = 200.0
fovy = 90.0

objw = []

rec_time = 0
count_fps = 0
count_frame = 0


class obj:
    """star object class"""

    def __init__(self, speed, dist):
        """constructor

        Args:
            speed (float): move z speed
            dist (float): range of z value to be generated
        """
        self.dist = dist
        self.z = random.random() * (-1.0 * self.dist)
        self.spd = speed
        self.init_xy_pos()

        # init texture area
        self.kind = random.randint(0, 3)
        self.tx = 0.5 * float(self.kind % 2)
        self.ty = 0.5 * float(int(self.kind / 2) % 2)
        self.tw = 0.5
        self.th = 0.5

    def init_xy_pos(self):
        global scr_w, scr_h, fovy
        h = self.z * math.tan(math.radians(fovy / 2.0))
        aspect = float(scr_w) / float(scr_h)
        self.x = random.uniform(-h * aspect, h * aspect)
        self.y = random.uniform(-h, h)

    def update(self):
        self.z += self.spd
        if self.z >= 0.0:
            self.z -= self.dist
            self.init_xy_pos()
            return

        global scr_w, scr_h, fovy

        # get position on screen
        sz = float(scr_h / 2) / math.tan(math.radians(fovy / 2.0))
        sx = self.x * sz / self.z
        sy = self.y * sz / self.z
        wh = float(scr_w / 2) * 1.2
        hh = float(scr_h / 2) * 1.2
        if sx < -wh or wh < sx or sy < -hh or hh < sy:
            # outside display area
            self.z = -self.dist - 15.0
            self.init_xy_pos()
            return


def init_objs():
    global objw, speed, dist
    for i in range(OBJ_MAX):
        objw.append(obj(speed, dist))

    init_count_fps()


def update_objs():
    for o in objw:
        o.update()


def init_count_fps():
    global rec_time, count_fps, count_frame
    rec_time = time.time()
    count_fps = 0
    count_frame = 0


def calc_fps():
    global rec_time, count_fps, count_frame

    count_frame += 1
    t = time.time() - rec_time
    if t >= 1.0:
        rec_time += 1.0
        count_fps = count_frame
        count_frame = 0
    elif t < 0.0:
        rec_time = time.time()
        count_fps = 0
        count_frame = 0


def load_texture():
    global texture

    # load image by using PIL
    im = Image.open(IMG_NAME)
    w, h = im.size
    # print("Image: %d x %d, %s" % (w, h, im.mode))

    if im.mode == "RGB":
        # RGB convert to RGBA
        im.putalpha(alpha=255)
    elif im.mode == "L" or im.mode == "P":
        # Grayscale, Index Color convert to RGBA
        im = im.convert("RGBA")

    raw_image = im.tobytes()

    ttype = GL_RGBA
    if im.mode == "RGB":
        ttype = GL_RGB
        # print("Set GL_RGB")
    elif im.mode == "RGBA":
        ttype = GL_RGBA
        # print("Set GL_RGBA")

    glBindTexture(GL_TEXTURE_2D, glGenTextures(1))

    # glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4)

    # set texture
    glTexImage2D(
        GL_TEXTURE_2D,  # target
        0,  # MIPMAP level
        ttype,  # texture type (RGB, RGBA)
        w,  # texture image width
        h,  # texture image height
        0,  # border width
        ttype,  # texture type (RGB, RGBA)
        GL_UNSIGNED_BYTE,  # data is unsigne char
        raw_image,  # texture data pointer
    )

    glClearColor(0, 0, 0, 0)
    glShadeModel(GL_SMOOTH)

    # set texture repeat
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)

    # set texture filter
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE)
    # glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL)
    # glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE)
    # glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND)


def set_billboard_matrix():
    m = glGetDoublev(GL_MODELVIEW_MATRIX)
    m[0][0] = m[1][1] = m[2][2] = 1.0
    m[0][1] = m[0][2] = 0.0
    m[1][0] = m[1][2] = 0.0
    m[2][0] = m[2][1] = 0.0
    glLoadMatrixd(m)


def draw_gl():
    glClearColor(0.0, 0.0, 0.0, 0.0)  # background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    glLoadIdentity()  # Reset The View

    # set camera
    ex, ey, ez = 0.0, 0.0, 0.0
    tx, ty, tz = 0.0, 0.0, -10.0
    gluLookAt(ex, ey, ez, tx, ty, tz, 0, 1, 0)

    glEnable(GL_BLEND)
    glEnable(GL_TEXTURE_2D)

    # glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glBlendFunc(GL_ONE, GL_ONE)

    glColor4f(1.0, 1.0, 1.0, 1.0)  # color

    # draw objects

    if USE_DEPTHMASK:
        glDepthMask(GL_TRUE)
        glDepthFunc(GL_LESS)
    else:
        glDepthMask(GL_FALSE)
        glDepthFunc(GL_LEQUAL)

    v = 2.0
    for o in objw:
        tx, ty, tw, th = o.tx, o.ty, o.tw, o.th

        glPushMatrix()
        glTranslatef(o.x, o.y, o.z)  # translate

        set_billboard_matrix()

        glBegin(GL_QUADS)
        glTexCoord2f(tx, ty)  # set u, v
        glVertex3f(-v, -v, 0)  # Top Left
        glTexCoord2f(tx + tw, ty)
        glVertex3f(v, -v, 0.0)  # Top Right
        glTexCoord2f(tx + tw, ty + th)
        glVertex3f(v, v, 0.0)  # Bottom Right
        glTexCoord2f(tx, ty + th)
        glVertex3f(-v, v, 0.0)  # Bottom Left
        glEnd()
        glPopMatrix()

    glDisable(GL_TEXTURE_2D)

    if not USE_DEPTHMASK:
        glDepthMask(GL_TRUE)
        glDepthFunc(GL_LESS)

    # draw text
    font = OpenGL.GLUT.GLUT_BITMAP_HELVETICA_18
    glRasterPos3f(-0.2, 1.0, -1.08)
    if glutBitmapString:
        glutBitmapString(font, bytes(f"{count_fps}/{FPS} FPS", "utf-8"))
    else:
        string = f"{count_fps}/{FPS}FPS"
        for i in range(len(string)):
            glutBitmapCharacter(font, ord(string[i]))

    glutSwapBuffers()

    calc_fps()


def init_viewport_and_pers(width, height):
    global scr_w, scr_h, dist

    # Prevent A Divide By Zero If The Window Is Too Small
    if height == 0:
        height = 1

    scr_w, scr_h = width, height

    # Reset The Current Viewport And Perspective Transformation
    glViewport(0, 0, width, height)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()  # Reset The Projection Matrix

    # Calculate The Aspect Ratio Of The Window
    aspect = float(width) / float(height)
    znear = 0.1
    zfar = dist + 50.0
    gluPerspective(fovy, aspect, znear, zfar)
    glMatrixMode(GL_MODELVIEW)


def InitGL(width, height):
    global dist

    glClearColor(0.0, 0.0, 0.0, 0.0)  # background color (R, G, B, A)
    glClearDepth(1.0)  # Enables Clearing Of The Depth Buffer
    glEnable(GL_DEPTH_TEST)  # Enables Depth Testing
    glDepthFunc(GL_LESS)  # The Type Of Depth Test To Do
    glShadeModel(GL_SMOOTH)  # Enables Smooth Color Shading

    # set fog
    if USE_FOG:
        glEnable(GL_FOG)
        glFogi(GL_FOG_MODE, GL_LINEAR)
        glFogf(GL_FOG_START, dist * 0.75)
        glFogf(GL_FOG_END, dist)
        glFogfv(GL_FOG_COLOR, [0.0, 0.0, 0.0, 0.0])

    init_viewport_and_pers(width, height)


def resize_gl(width, height):
    """Windows resize

    Args:
        width (Integer): window width
        height (Integer): Window height
    """

    init_viewport_and_pers(width, height)


def on_timer(value):
    """Update

    Args:
        value (Integer): mill seconds
    """

    update_objs()
    glutPostRedisplay()  # redraw
    glutTimerFunc(int(1000 / FPS), on_timer, 0)


def key_pressed(key, x, y):
    ESCAPE = b"\x1b"
    if key == ESCAPE or key == b"q":
        # ESC key or 'q' key to exit
        if glutLeaveMainLoop:
            glutLeaveMainLoop()
        else:
            sys.exit()


def main():
    global window

    init_objs()

    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)

    glutInitWindowSize(SCRW, SCRH)
    # glutInitWindowPosition(0, 0)

    window = glutCreateWindow(b"Like ssstar")

    glutDisplayFunc(draw_gl)
    glutReshapeFunc(resize_gl)
    glutKeyboardFunc(key_pressed)
    # glutFullScreen()

    # glutIdleFunc(draw_gl)
    glutTimerFunc(int(1000 / FPS), on_timer, 0)

    InitGL(SCRW, SCRH)

    load_texture()

    glutMainLoop()


if __name__ == "__main__":
    print("Hit ESC or 'q' key to quit.")
    main()
