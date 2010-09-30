ARCH_URI = 'http://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-G.729-200701-I!!SOFT-ZST-E&type=items'
target_dir = $(G729_PATH)/DECODER.MAK
target_makefile = $(G729_PATH)/Makefile

$(target_dir) : arch.zip
	unzip arch.zip && touch $(target_dir)

arch.zip :
	wget $(ARCH_URI) -O arch.zip

$(target_makefile) : $(target_dir)
	./tolower.sh $(G729_PATH)

if APPLY_PATCHES
patches.stamp : $(target_makefile)
	./apply_patches.sh
endif

clean:
	rm -rf Soft G729E.*

if APPLY_PATCHES
all : patches.stamp
else
all : $(target_makefile)
endif
