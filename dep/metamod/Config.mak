MODNAME = metamod

EXTRA_CFLAGS = -DMETAMOD_CORE

SRCFILES = api_info.cpp commands_meta.cpp conf_meta.cpp dllapi.cpp \
	engine_api.cpp engineinfo.cpp game_support.cpp h_export.cpp linkent.cpp linkgame.cpp \
	linkplug.cpp log_meta.cpp meta_eiface.cpp metamod.cpp mhook.cpp mlist.cpp \
	mplayer.cpp mplugin.cpp mqueue.cpp mreg.cpp mutil.cpp osdep.cpp reg_support.cpp \
	sdk_util.cpp studioapi.cpp support_meta.cpp thread_logparse.cpp vdate.cpp

INFOFILES = info_name.h vers_meta.h
RESFILE = res_meta.rc

GENERATED = games.h

#STLFILES = mreg.cpp

ifeq "$(OS)" "linux"
MKLINKGAME := $(shell ./mklinkgame.sh)
endif
