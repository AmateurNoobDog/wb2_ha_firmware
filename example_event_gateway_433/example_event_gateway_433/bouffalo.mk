ifdef BL60X_SDK_PATH

COMPONENT_ADD_INCLUDEDIRS := .

COMPONENT_SRCS := main.c \
                  event_handler.c \
                  uart_433.c

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

CFLAGS += -Wno-unused-local-typedefs

endif
