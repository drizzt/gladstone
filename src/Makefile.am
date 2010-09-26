# plugindir is set in configure
g729_ref_objs=\
			  $(G729_PATH)/acelp_ca.o\
			  $(G729_PATH)/basic_op.o\
			  $(G729_PATH)/bits.o\
			  $(G729_PATH)/cod_ld8a.o\
			  $(G729_PATH)/coder.o\
			  $(G729_PATH)/dspfunc.o\
			  $(G729_PATH)/filter.o\
			  $(G729_PATH)/gainpred.o\
			  $(G729_PATH)/lpc.o\
			  $(G729_PATH)/lpcfunc.o\
			  $(G729_PATH)/lspgetq.o\
			  $(G729_PATH)/oper_32b.o\
			  $(G729_PATH)/p_parity.o\
			  $(G729_PATH)/pitch_a.o\
			  $(G729_PATH)/pre_proc.o\
			  $(G729_PATH)/pred_lt3.o\
			  $(G729_PATH)/qua_gain.o\
			  $(G729_PATH)/qua_lsp.o\
			  $(G729_PATH)/tab_ld8a.o\
			  $(G729_PATH)/util.o\
			  $(G729_PATH)/taming.o\
			  $(G729_PATH)/cor_func.o\
			  $(G729_PATH)/decoder.o\
			  $(G729_PATH)/de_acelp.o\
			  $(G729_PATH)/dec_gain.o\
			  $(G729_PATH)/dec_lag3.o\
			  $(G729_PATH)/dec_ld8a.o\
			  $(G729_PATH)/lspdec.o\
			  $(G729_PATH)/post_pro.o\
			  $(G729_PATH)/postfilt.o

plugin_LTLIBRARIES = libgstg729.la 

# sources used to compile this plug-in
libgstg729_la_SOURCES = gstg729plugin.c gstg729enc.c gstg729dec.c 

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstg729_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) -I$(G729_PATH)
libgstg729_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS) libg729_ref.a

libgstg729_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L$(G729_PATH)
libgstg729_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstg729enc.h gstg729dec.h g729common.h

$(g729_ref_objs) : $(G729_PATH)/coder.mak $(G729_PATH)/decoder.mak
	make -C $(G729_PATH) -f coder.mak
	make -C $(G729_PATH) -f decoder.mak

libg729_ref.a : $(g729_ref_objs)
	$(AR) r libg729_ref.a $(g729_ref_objs) 
