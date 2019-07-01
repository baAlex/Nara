
.SUFFIXES: .c .o .dbg.o

CC=cc
RM=rm -f
CLOC=cloc --by-percent 'c'
RUBY=ruby

OUT=nara
OUT_DBG=nara-dbg

CFLAGS=-c -O3 -flto -mtune=generic
CFLAGS_DBG=-c -std=c11 -O0 -Wall -Wextra -pedantic -g -DDEBUG
LFLAGS=-lglfw -lGL -lm
LFLAGS_DBG=-lglfw -lGL -lm

FILES=./source/nara.c\
      ./source/context.c\
      ./source/context-gl.c\
      ./source/endianness.c\
      ./source/image.c\
      ./source/image-sgi.c\
      ./source/matrix.c\
      ./source/status.c\
      ./source/vector.c

default: debug
all: release debug

generated:
	$(RUBY) ./tools/to-hfile.rb ./source/shaders/white-fragment.glsl > ./source/shaders/white-fragment.h
	$(RUBY) ./tools/to-hfile.rb ./source/shaders/white-vertex.glsl > ./source/shaders/white-vertex.h

generated_clean:
	$(RM) ./source/shaders/white-fragment.h
	$(RM) ./source/shaders/white-vertex.h

.c.o:
	$(CC) $(CFLAGS) $< -o $@

.c.dbg.o:
	$(CC) $(CFLAGS_DBG) $< -o $@

release: generated $(FILES:.c=.o)
	$(CC) $(FILES:.c=.o) $(LFLAGS) -o $(OUT)

debug: generated $(FILES:.c=.dbg.o)
	$(CC) $(FILES:.c=.dbg.o) $(LFLAGS_DBG) -o $(OUT_DBG)

clean: generated_clean
	$(RM) $(OUT)
	$(RM) $(OUT_DBG)
	$(RM) $(FILES:.c=.o)
	$(RM) $(FILES:.c=.dbg.o)

stats:
	$(CLOC) $(FILES)
