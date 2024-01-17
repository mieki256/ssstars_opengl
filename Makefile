# use MinGW (gcc 6.3.0)

ssstars_opengl.scr: ssstars_opengl.o resource.o Makefile
	g++ $< resource.o -o $@ -static -lstdc++ -lgcc -lscrnsave -lopengl32 -lglu32 -lgdi32 -lcomctl32 -lshlwapi -lwinmm -mwindows

ssstars_opengl.o: ssstars_opengl.cpp texture.h fontdata_profont.h Makefile
	g++ -c $< -o $@ -O3

resource.o: resource.rc resource.h icon.ico Makefile
	windres $< $@

texture.h: texture.png
	xxd -i $< > $@

fontdata_profont.h: font_profont.png fontpng2bits.py Makefile
	python fontpng2bits.py -i $< --label fontdata > $@

.PHONY: cleanall
cleanall:
	rm -f *.scr
	rm -f *.o

.PHONY: clean
clean:
	rm -f *.o
