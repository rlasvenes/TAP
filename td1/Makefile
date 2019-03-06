CC = gcc
CFLAGS = -Wall -g -Wno-unused-function -Wno-deprecated-declarations
LDLIBS = -lm

ifeq ($(shell uname -s), Darwin)
    LF = /Library/Frameworks
    LDLIBS += -F $(LF)/
    LDLIBS += -framework OpenGL -framework GLUT -framework SDL2
    LDFLAGS += -I $(LF)/SDL2.framework/Headers/
else
    LDLIBS += -lglut -lGLU -lGL -lSDL2
endif

tsp: tsp_main.c tools.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test_heap: test_heap.c heap.c
	$(CC) $(CFLAGS) $^ -o $@

a_star: a_star.c tools.c heap.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f tsp
	rm -f test_heap
	rm -f a_star
	rm -fr *.dSYM/
