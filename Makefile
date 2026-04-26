#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

.SECONDARY:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	source source/views source/controllers source/core
DATA		:=	assets/models
INCLUDES	:=	include source
GRAPHICS	:=	assets/graphics
SFX       	:=  assets/sfx
NITRODATA   :=  nitrofiles

GAME_TITLE	   := Persona 3 Dual
GAME_SUBTITLE1 := A Fan Recreation
GAME_SUBTITLE2 := Demake by Taha Rashid
export GAME_ICON := $(CURDIR)/../icon.bmp

#---------------------------------------------------------------------------------
# Python tool configuration
#---------------------------------------------------------------------------------
TOOLS_DIR       := $(CURDIR)/tools
VENV_PYTHON     := $(HOME)/.venv/bin/python3

ASSETS_DIALOGUE := $(CURDIR)/assets/dialogue
ASSETS_MUSIC    := $(CURDIR)/assets/music
ASSETS_VIDEO    := $(CURDIR)/assets/video
ASSETS_MODELS   := $(CURDIR)/assets/models
ASSETS_MAPS     := $(CURDIR)/assets/maps

NITRO_MUSIC     := $(CURDIR)/nitrofiles/music
NITRO_VIDEO     := $(CURDIR)/nitrofiles/video

#---------------------------------------------------------------------------------
# Per-tool flags (override on the command line, e.g. make VIDEO_FPS=24)
#
#   VIDEO_BITS   — colour depth for video encoding: 8 or 16    [default: 8]
#   VIDEO_FPS    — frames per second for video encoding         [default: 15]
#   VIDEO_SIZE   — output resolution WxH                        [default: 256x192]
#   MODEL_TEXSIZE— fallback texture size when not in filename   [default: 256 256]
#   DLG_FLAGS    — extra flags forwarded to dlg2dialogue.py     [default: (none)]
#   MAP_FLAGS    — extra flags forwarded to texture2collision.py[default: (none)]
#---------------------------------------------------------------------------------
VIDEO_BITS    ?= 8
VIDEO_FPS     ?= 15
VIDEO_SIZE    ?= 256x192
MODEL_TEXSIZE ?= 256 256
DLG_FLAGS     ?=
MAP_FLAGS     ?=

#---------------------------------------------------------------------------------
# Collect source files
#---------------------------------------------------------------------------------
DLG_FILES   := $(wildcard $(ASSETS_DIALOGUE)/*.dlg)
MP3_FILES   := $(wildcard $(ASSETS_MUSIC)/*.mp3)
MP4_FILES   := $(wildcard $(ASSETS_VIDEO)/*.mp4)
OBJ_FILES   := $(wildcard $(ASSETS_MODELS)/*.obj)
MAP_FILES   := $(wildcard $(ASSETS_MAPS)/*.png)

#---------------------------------------------------------------------------------
# Derive output paths
#---------------------------------------------------------------------------------
DIALOGUE_OUT := $(DLG_FILES:$(ASSETS_DIALOGUE)/%.dlg=$(CURDIR)/source/dialogue/%.h)
MUSIC_OUT    := $(MP3_FILES:$(ASSETS_MUSIC)/%.mp3=$(NITRO_MUSIC)/%.pcm)
VIDEO_OUT    := $(MP4_FILES:$(ASSETS_VIDEO)/%.mp4=$(NITRO_VIDEO)/%.vid)
MODEL_OUT    := $(OBJ_FILES:$(ASSETS_MODELS)/%.obj=$(CURDIR)/assets/models/%.bin)
MAP_OUT      := $(MAP_FILES:$(ASSETS_MAPS)/%.png=$(CURDIR)/source/maps/%.h)
OFFSET_OUT   := $(OBJ_FILES:$(ASSETS_MODELS)/%.obj=$(CURDIR)/source/maps/%_offsets.h)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv5te -mtune=arm946e-s -mthumb

CFLAGS  :=  -g -Wall -O3 -flto -ffunction-sections -fdata-sections\
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS =   -specs=ds_arm9.specs -g $(ARCH) -flto -Wl,--gc-sections -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS    := -lmm9 -lfat -lnds9

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS) $(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export TOPDIR   :=  $(CURDIR)

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

ifneq ($(strip $(NITRODATA)),)
    export NITRO_FILES  :=  $(CURDIR)/$(NITRODATA)
endif

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*))) soundbank.bin
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))

export SFXFILES	:=	$(foreach dir,$(notdir $(wildcard $(SFX)/*.*)),$(CURDIR)/$(SFX)/$(dir))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(PNGFILES:.png=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean assets dialogue music video models maps offsets help

#---------------------------------------------------------------------------------
# Main build — asset conversion always runs first (DEFAULT TARGET)
#---------------------------------------------------------------------------------
$(BUILD): assets
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# help — list all targets and flags
#---------------------------------------------------------------------------------
help:
	@echo ""
	@echo "  Persona 3 Dual — build targets"
	@echo "  ──────────────────────────────────────────────────────"
	@echo "  make              Build everything (assets + NDS ROM)"
	@echo "  make assets       Run all asset converters"
	@echo "  make dialogue     .dlg  →  source/dialogue/*.h + .cpp"
	@echo "  make music        .mp3  →  nitrofiles/music/*.pcm"
	@echo "  make video        .mp4  →  nitrofiles/video/*.vid"
	@echo "  make models       .obj  →  assets/models/*.bin"
	@echo "  make maps         .png  →  source/maps/*.h (collision)"
	@echo "  make offsets      .obj  →  source/maps/*_offsets.h (auto TILE_SIZE)"
	@echo "  make clean        Remove build artefacts"
	@echo ""
	@echo "  Overridable flags (pass on command line):"
	@echo "  VIDEO_BITS=$(VIDEO_BITS)        8 or 16 colour depth"
	@echo "  VIDEO_FPS=$(VIDEO_FPS)         frames per second"
	@echo "  VIDEO_SIZE=$(VIDEO_SIZE)   output resolution"
	@echo "  MODEL_TEXSIZE='$(MODEL_TEXSIZE)'  fallback tex size (no _WxH in filename)"
	@echo "  DLG_FLAGS='$(DLG_FLAGS)'         extra flags for dlg2dialogue.py"
	@echo "  MAP_FLAGS='$(MAP_FLAGS)'         extra flags for texture2collision.py"
	@echo ""
	@echo "  Examples:"
	@echo "  make video VIDEO_FPS=24 VIDEO_BITS=16"
	@echo "  make models MODEL_TEXSIZE='128 128'"
	@echo ""

#---------------------------------------------------------------------------------
# Asset conversion — create output dirs, then run each converter
#---------------------------------------------------------------------------------
assets: dirs dialogue music video models maps offsets

dirs:
	@mkdir -p $(CURDIR)/source/dialogue
	@mkdir -p $(CURDIR)/source/maps
	@mkdir -p $(NITRO_MUSIC)
	@mkdir -p $(NITRO_VIDEO)

# ── Dialogue ──────────────────────────────────────────────────────────────────
# Input:   assets/dialogue/<name>.dlg
# Output:  source/dialogue/<name>_dialogue.h  +  source/dialogue/<name>_dialogue.cpp
# Flags:   DLG_FLAGS  (e.g. --stdout for debug)
$(CURDIR)/source/dialogue/%.h: $(ASSETS_DIALOGUE)/%.dlg
	@echo "  DLG   $(notdir $<)"
	@cd $(CURDIR)/source/dialogue && \
	    $(VENV_PYTHON) $(TOOLS_DIR)/dlg2dialogue.py $< -o $* $(DLG_FLAGS)

dialogue: $(DIALOGUE_OUT)

# ── Music ─────────────────────────────────────────────────────────────────────
# Input:   assets/music/<name>.mp3
# Output:  nitrofiles/music/<name>.pcm   (s16le, 32 kHz, stereo)
$(NITRO_MUSIC)/%.pcm: $(ASSETS_MUSIC)/%.mp3
	@echo "  PCM   $(notdir $<)"
	@ffmpeg -i $< -f s16le -ar 32000 -ac 2 $@ -y -loglevel error

music: $(MUSIC_OUT)

# ── Video ─────────────────────────────────────────────────────────────────────
# Input:   assets/video/<name>.mp4
# Output:  nitrofiles/video/<name>.vid   (interleaved audio+video)
# Flags:   VIDEO_BITS, VIDEO_FPS, VIDEO_SIZE
$(NITRO_VIDEO)/%.vid: $(ASSETS_VIDEO)/%.mp4
	@echo "  VID   $(notdir $<)  [$(VIDEO_BITS)bpp @ $(VIDEO_FPS)fps $(VIDEO_SIZE)]"
	@$(VENV_PYTHON) $(TOOLS_DIR)/video2vid.py $< $(basename $@) \
		--bits $(VIDEO_BITS) --fps $(VIDEO_FPS) --size $(VIDEO_SIZE)

video: $(VIDEO_OUT)

# ── Models ────────────────────────────────────────────────────────────────────
# Input:   assets/models/<name>.obj           → --texsize 256 256  (MODEL_TEXSIZE)
#          assets/models/<name>_128x128.obj   → --texsize 128 128  (from filename)
#          assets/models/<name>_64x64.obj     → --texsize 64 64
# Output:  assets/models/<name>.bin
$(CURDIR)/assets/models/%.bin: $(ASSETS_MODELS)/%.obj
	@echo "  BIN   $(notdir $<)"
	$(eval _ts := $(shell echo "$*" | grep -oE '[0-9]+x[0-9]+$$' | tr 'x' ' '))
	$(eval _texsize := $(if $(_ts),$(_ts),$(MODEL_TEXSIZE)))
	@$(VENV_PYTHON) $(TOOLS_DIR)/obj2bin.py $< $@ --texsize $(_texsize)

models: $(MODEL_OUT)

# ── Collision maps ────────────────────────────────────────────────────────────
# Input:   assets/maps/<name>.png            → no crop
#          assets/maps/<name>_X_Y_W_H.png    → crop x y w h  (e.g. lobby_0_0_64_64.png)
# Output:  source/maps/<name>.h
# Flags:   MAP_FLAGS
$(CURDIR)/source/maps/%.h: $(ASSETS_MAPS)/%.png
	@echo "  MAP   $(notdir $<)"
	$(eval _crop_raw := $(shell echo "$*" | grep -oE '(_[0-9]+){4}$$'))
	$(eval _crop_args := $(if $(_crop_raw),$(shell echo "$(_crop_raw)" | tr -d '_' | \
		awk '{print substr($$0,1,1)," ",substr($$0,2,1)," ",substr($$0,3,1)," ",substr($$0,4,1)}' \
		| sed 's/  */ /g' | xargs | \
		python3 -c "import sys,re; t=sys.stdin.read().strip(); \
		nums=re.findall(r'[0-9]+','$(shell echo "$*" | grep -oE "([0-9]+_){3}[0-9]+$$")'); \
		print(' '.join(nums)) if nums else print('')"),))
	$(eval _crop_args := $(shell echo "$*" | grep -oE '([0-9]+_){3}[0-9]+$$' | tr '_' ' '))
	@$(VENV_PYTHON) $(TOOLS_DIR)/texture2collision.py $< $@ $(_crop_args) $(MAP_FLAGS)

maps: $(MAP_OUT)

# ── World offsets (auto TILE_SIZE / WORLD_OFFSET) ────────────────────────────
# Input:   assets/models/<name>.obj   (reads vertex bounds)
#          assets/maps/<name>.png     (auto-detected for tile count)
# Output:  source/maps/<name>_offsets.h
# Note:    also accepts _WxH in the obj filename for the texsize hint (ignored here,
#          the map PNG drives tile count).
$(CURDIR)/source/maps/%_offsets.h: $(ASSETS_MODELS)/%.obj
	@echo "  OFF   $(notdir $<)"
	@$(VENV_PYTHON) $(TOOLS_DIR)/obj2offsets.py $< -o $@

offsets: $(OFFSET_OUT)


#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba
	@rm -f  $(DIALOGUE_OUT) \
	        $(CURDIR)/source/dialogue/*_dialogue.cpp \
	        $(MUSIC_OUT) \
	        $(VIDEO_OUT) \
	        $(MODEL_OUT) \
	        $(MAP_OUT) \
	        $(OFFSET_OUT)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).nds	: 	$(OUTPUT).elf
$(OUTPUT).nds   :   $(shell find $(TOPDIR)/$(NITRODATA))
$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# rule to build soundbank from music files
#---------------------------------------------------------------------------------
soundbank.bin soundbank.h : $(SFXFILES)
#---------------------------------------------------------------------------------
	@mmutil $^ -d -osoundbank.bin -hsoundbank.h

#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.mp3.o	:	%.mp3
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.s %.h	: %.png %.grit
#---------------------------------------------------------------------------------
	grit $< -fts -o$*

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------