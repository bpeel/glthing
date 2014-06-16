bin_PROGRAMS = \
	glthing \
	$(NULL)

AM_CPPFLAGS = \
	$(EPOXY_CFLAGS) \
	$(SDL2_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLTHING_EXTRA_CFLAGS) \
	$(NULL)

glthing_SOURCES = \
	main.c \
	gol.c \
	shader-data.c \
	$(NULL)

glthing_LDADD = \
	$(EPOXY_LIBS) \
	$(SDL2_LIBS) \
	$(GL_LIBS) \
	$(GLTHING_EXTRA_LIBS) \
	$(NULL)

EXTRA_DIST = \
	fragment-shader.glsl \
	vertex-shader.glsl \
	$(NULL)
