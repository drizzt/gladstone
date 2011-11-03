# plugindir is set in configure
g729_ref_srcs=\
			  $(G729_PATH)/acelp_ca.c\
			  $(G729_PATH)/basic_op.c\
			  $(G729_PATH)/bits.c\
			  $(G729_PATH)/cod_ld8a.c\
			  $(G729_PATH)/dspfunc.c\
			  $(G729_PATH)/filter.c\
			  $(G729_PATH)/gainpred.c\
			  $(G729_PATH)/lpc.c\
			  $(G729_PATH)/lpcfunc.c\
			  $(G729_PATH)/lspgetq.c\
			  $(G729_PATH)/oper_32b.c\
			  $(G729_PATH)/p_parity.c\
			  $(G729_PATH)/pitch_a.c\
			  $(G729_PATH)/pre_proc.c\
			  $(G729_PATH)/pred_lt3.c\
			  $(G729_PATH)/qua_gain.c\
			  $(G729_PATH)/qua_lsp.c\
			  $(G729_PATH)/tab_ld8a.c\
			  $(G729_PATH)/util.c\
			  $(G729_PATH)/taming.c\
			  $(G729_PATH)/cor_func.c\
			  $(G729_PATH)/de_acelp.c\
			  $(G729_PATH)/dec_gain.c\
			  $(G729_PATH)/dec_lag3.c\
			  $(G729_PATH)/dec_ld8a.c\
			  $(G729_PATH)/lspdec.c\
			  $(G729_PATH)/post_pro.c\
			  $(G729_PATH)/postfilt.c\
			  $(G729_PATH)/calcexc.c\
			  $(G729_PATH)/qsidlsf.c\
			  $(G729_PATH)/dec_sid.c\
			  $(G729_PATH)/qsidgain.c\
			  $(G729_PATH)/tab_dtx.c\
			  $(G729_PATH)/dtx.c\
			  $(G729_PATH)/vad.c

plugin_LTLIBRARIES = libgstg729.la 

# sources used to compile this plug-in
libgstg729_la_SOURCES = gstg729plugin.c gstg729enc.c gstg729dec.c $(g729_ref_srcs)

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstg729_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) -I$(G729_PATH)
libgstg729_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS)

libgstg729_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L$(G729_PATH)
libgstg729_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstg729enc.h gstg729dec.h g729common.h
