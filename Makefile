NAME      := solunar
VERSION   := 2.0a
LIBS      := -lm ${EXTRA_LIBS} 
KLIB      := klib
KLIB_INC  := $(KLIB)/include
KLIB_LIB  := $(KLIB)
LIBSOL    := libsolunar
LIBSOL_INC := $(LIBSOL)/include
LIBSOL_LIB := $(LIBSOL)
TARGET	  := $(NAME)
SOURCES   := $(shell find src/ -type f -name *.c)
OBJECTS   := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	  := $(OBJECTS:.o=.deps)
DESTDIR   := /
PREFIX    := /usr
BINDIR    := $(DESTDIR)/$(PREFIX)/bin
MANDIR    := $(DESTDIR)/$(PREFIX)/share/man/man1/
CFLAGS    := -g -O0 -fpie -fpic -Wall -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -I $(LIBSOL_INC) -I $(KLIB_INC) ${EXTRA_CFLAGS} -ffunction-sections -fdata-sections

LDFLAGS :=  -pie -Wl,--gc-sections ${EXTRA_LDFLAGS}

$(TARGET): $(OBJECTS) 
	echo $(SOURCES)
	make -C klib
	make -C libsolunar 
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBSOL)/libsolunar.a $(KLIB)/klib.a $(LIBS) 

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	$(RM) -r build/ $(TARGET) 
	make -C klib clean
	make -C libsolunar clean

install: $(TARGET)
	mkdir -p $(BINDIR) $(MANDIR)
	strip $(TARGET)
	install -m 755 $(TARGET) ${BINDIR}
	install -m 644 man1/* $(MANDIR)

-include $(DEPS)

.PHONY: clean

