PROJECT_NAME := libutl.a
OBJECT_DIRECTORY = _build
PRJ_PATH = .
OUTPUT_BINARY_DIRECTORY = .
OUTPUT_FILENAME := $(PROJECT_NAME)

######################################
#CFLAGS_USER
#CFLAGS_USER += -DPTARM_DEBUG_MEM
######################################

include ../options.mak

#GNU_PREFIX := arm-none-eabi-

MK := mkdir
RM := rm -rf

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO :=
else
NO_ECHO := @
endif

# Toolchain commands
CC       		:= "$(GNU_PREFIX)gcc"
AS       		:= "$(GNU_PREFIX)as"
AR       		:= "$(GNU_PREFIX)ar" -r
LD       		:= "$(GNU_PREFIX)ld"
NM       		:= "$(GNU_PREFIX)nm"
OBJDUMP  		:= "$(GNU_PREFIX)objdump"
OBJCOPY  		:= "$(GNU_PREFIX)objcopy"
SIZE    		:= "$(GNU_PREFIX)size"

#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

#sources project
C_SOURCE_FILES += $(PRJ_PATH)/utl_misc.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_buf.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_push.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_net.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_opts.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_str.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_jsonrpc.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_addr.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_log.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_time.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_rng.c
C_SOURCE_FILES += $(PRJ_PATH)/utl_int.c


#includes common to all targets
INC_PATHS += -I.
INC_PATHS += -I../libs/install/include

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY))

######################################
#CFLAGS
######################################
# cpu
#CFLAGS += -mcpu=cortex-m0
#CFLAGS += -mthumb -mabi=aapcs
#CFLAGS += -mfloat-abi=soft

CFLAGS += -W -Wall -Wno-error=unused-function -fno-common

# keep every function in separate section. This will allow linker to dump unused functions
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
#CFLAGS += -flto -fno-builtin

# for mbedtls
CFLAGS += -I../libs/mbedtls_config -DMBEDTLS_CONFIG_FILE='<config-ptarm.h>'

# others
CFLAGS += $(CFLAGS_USER)

ifeq ("$(BUILD_PTARMD)","LIB")
	CFLAGS += -fPIC
endif

######################################
#CFLAGS only
######################################

CFLAGS += -std=gnu99


######################################
#LDFLAGS
######################################
# keep every function in separate section. This will allow linker to dump unused functions
#LDFLAGS += -Xlinker -Map=$(OBJECT_DIRECTORY)/$(OUTPUT_FILENAME).map

# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections


######################################
#target
######################################

#default target - first one defined
default: debug

#building all targets
all:
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e cleanobj
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e debug

#target for printing all targets
help:
	@echo following targets are available:
	@echo 	debug release


C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

vpath %.c $(C_PATHS)

OBJECTS = $(C_OBJECTS)

lib:
	$(MAKE) -C ../libs

debug: CFLAGS += -DDEBUG -DPTARM_DEBUG
debug: CFLAGS += -ggdb3 -O0
debug: LDFLAGS += -ggdb3 -O0
debug: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo [DEBUG]Linking target: $(OUTPUT_FILENAME)
	@echo [DEBUG]CFLAGS=$(CFLAGS)
	$(NO_ECHO)$(LD) -r $(OBJECTS) -o ptarm_tmp.o
	$(NO_ECHO)$(OBJCOPY) --localize-hidden ptarm_tmp.o ptarm_lib.o
	$(NO_ECHO)$(AR) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME) ptarm_lib.o
	$(NO_ECHO)$(RM) ptarm_tmp.o ptarm_lib.o

#release: CFLAGS += -DNDEBUG
release: CFLAGS += -O3
#release: CFLAGS +=-Werror
release: LDFLAGS += -O3
release: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo [RELEASE]Linking target: $(OUTPUT_FILENAME)
	$(NO_ECHO)$(LD) -r $(OBJECTS) -o ptarm_tmp.o
	$(NO_ECHO)$(OBJCOPY) --localize-hidden ptarm_tmp.o ptarm_lib.o
	$(NO_ECHO)$(AR) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME) ptarm_lib.o
	$(NO_ECHO)$(RM) ptarm_tmp.o ptarm_lib.o

## Create build directories
$(BUILD_DIRECTORIES):
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling C file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) -c -o $@ $<


# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME): $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME)
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME)

cppcheck:
	cppcheck --enable=all --suppress=missingIncludeSystem -D__ORDER_LITTLE_ENDIAN__ -DDEBUG -DPTARM_DEBUG -DPTARM_USE_PRINTFUNC -DPTARM_DEBUG_MEM $(INC_PATHS) $(PRJ_PATH)/*.c

clean:
	$(RM) $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME) .Depend
	$(MAKE) -C tests clean

test:
	$(MAKE) -C tests
	$(MAKE) -C tests exec

################################

.Depend:
ifneq ($(MAKECMDGOALS),clean)
	@$(foreach SRC,$(C_SOURCE_FILES),$(CC) $(CFLAGS) $(INC_PATHS) -MM -MT $(OBJECT_DIRECTORY)/$(notdir $(SRC:%.c=%.o)) $(SRC) >> .Depend; )
endif

-include .Depend
