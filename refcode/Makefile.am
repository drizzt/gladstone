ARCH_URI = 'https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-G.729-201206-I!!SOFT-ZST-E&type=items'
target_makefile = $(G729_PATH)decoder.mak

$(target_makefile) : arch.zip
	unzip arch.zip
	touch $(target_makefile)

arch.zip :
	wget $(ARCH_URI) -O arch.zip

if APPLY_PATCHES
patches.stamp : $(target_makefile)
	./apply_patches.sh
endif

clean:
	rm -rf Soft G729E.* patches.stamp

if APPLY_PATCHES
all : patches.stamp
else
all : $(target_makefile)
endif
