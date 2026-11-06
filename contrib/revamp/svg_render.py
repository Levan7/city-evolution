import ctypes as C
r=C.CDLL('librsvg-2.so.2');c=C.CDLL('libcairo.so.2');g=C.CDLL('libgobject-2.0.so.0')
r.rsvg_handle_new_from_file.argtypes=[C.c_char_p,C.c_void_p];r.rsvg_handle_new_from_file.restype=C.c_void_p
r.rsvg_handle_render_cairo.argtypes=[C.c_void_p,C.c_void_p];r.rsvg_handle_render_cairo.restype=C.c_int
c.cairo_image_surface_create.argtypes=[C.c_int,C.c_int,C.c_int];c.cairo_image_surface_create.restype=C.c_void_p
c.cairo_create.argtypes=[C.c_void_p];c.cairo_create.restype=C.c_void_p
c.cairo_surface_write_to_png.argtypes=[C.c_void_p,C.c_char_p]
c.cairo_destroy.argtypes=[C.c_void_p];c.cairo_surface_destroy.argtypes=[C.c_void_p];g.g_object_unref.argtypes=[C.c_void_p]
def render(p,q,w,h):
 handle=r.rsvg_handle_new_from_file(str(p).encode(),None);assert handle
 surface=c.cairo_image_surface_create(0,w,h);cr=c.cairo_create(surface)
 assert r.rsvg_handle_render_cairo(handle,cr)
 assert c.cairo_surface_write_to_png(surface,str(q).encode())==0
 c.cairo_destroy(cr);c.cairo_surface_destroy(surface);g.g_object_unref(handle)
