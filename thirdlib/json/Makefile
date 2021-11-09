CXX=g++
JSONBASE=jsoncpp-src-0.6.0-rc2
LIBJSON=$(JSONBASE)/src/lib_json
INCLUDES=-I$(JSONBASE)/include
SOURCES=$(LIBJSON)/json_reader.cpp $(LIBJSON)/json_writer.cpp $(LIBJSON)/json_value.cpp
OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))

.PHONY: all
all :
	if [ ! -e "$(JSONBASE)" ]; then tar zxvf $(JSONBASE).tar.gz; fi
	make build

build: libjsoncpp.a

libjsoncpp.a : $(OBJECTS)
	ar -crv $@ $(OBJECTS)

$(OBJECTS) : %o : %cpp
	$(CXX) -o $@ -c $< $(INCLUDES)
.PHONY: install
install:
	mkdir -p lib
	cp libjsoncpp.a lib/
	cp -r $(JSONBASE)/include .
.PHONY: uninstall
uninstall:
	rm -rf include
	rm -rf lib
	rm -rf jsoncpp-src-0.6.0-rc2
.PHONY: clean
clean:
	rm -f libjsoncpp.a
	rm -rf $(OBJECTS)
