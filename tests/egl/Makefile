all:
	gcc gles2_help.c simple-gles2.c -g -o simple-gles2 `pkg-config --cflags --libs x11 egl glesv2 gl` -DUSE_INTERNAL_LOG
clean:
	rm -f simple-gl
