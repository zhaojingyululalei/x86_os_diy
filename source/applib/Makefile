include ../../Makefile.common

# 手动配置项
PROJECT := stdlib
INCLUDE := ../os_config ../kernel ../tools ../lib
SRCDIR := 
LIBDIR := 

# 查找库文件并生成链接选项
LIB := $(shell find $(LIBDIR) -name '*.a')
LIBS := $(patsubst %,-L%,${LIBDIR})
LFLAGS := $(patsubst lib%.a,-l%,${LIB})
LFLAGS += -melf_i386 -Ttext=0x8000

# 路径配置
BUILD := ../../build
IMAGE_DIR := ../../image
BUILDS := ${BUILD}/${PROJECT}
OUT := lib$(PROJECT).a  # 输出静态库文件

INCLUDES := $(patsubst %,-I%,${INCLUDE})

# 查找源文件
C_SRCS := $(shell find ./ -name '*.c')
ASM_SRCS := $(shell find ./ -name '*.S')

# 判断 SRCDIR 是否为空，并相应地添加源文件
ifeq ($(strip $(SRCDIR)),)
else
    C_SRCS += $(shell find $(SRCDIR) -name '*.c')
    ASM_SRCS += $(shell find $(SRCDIR) -name '*.S')
endif

# 将所有源文件合并到 SRCS 中，并生成目标文件列表
SRCS := $(ASM_SRCS) $(C_SRCS)
OBJS := $(addsuffix .o, $(basename $(SRCS)))
DEPS := $(addsuffix .d, $(basename $(SRCS)))

# 创建构建目录的命令
MD := mkdir -p

all: $(BUILDS) $(IMAGE_DIR) $(OUT)
	@echo "Executing 'all' complete!"
	$(OBJDUMP_TOOL) -x -d -S -m i386 $(OUT) > $(BUILDS)/$(PROJECT)_dist.txt
	$(READELF_TOOL) -a $(OUT) > $(BUILDS)/$(PROJECT)_elf.txt

# 创建构建目录
$(BUILDS):
	-${MD} ${BUILDS}
	@echo "Build directory created."

# 创建镜像目录
$(IMAGE_DIR):
	-${MD} ${IMAGE_DIR}
	@echo "Image directory created."

# 链接目标文件生成静态库文件
$(OUT): $(OBJS)
	$(AR) rcs $@ $(OBJS)

# 自动包含依赖文件
-include $(DEPS)

# 编译 C 文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -c $< -o $@

# 编译汇编文件
%.o: %.S
	$(CC) $(ASFLAGS) $(INCLUDES) -MMD -c $< -o $@

.PHONY: clean test

# 测试规则
test:
	@echo "OBJS: $(OBJS)"

# 清理规则
clean:
	-rm -f $(OBJS) $(DEPS)
	-rm -f $(BUILDS)/*.txt
	-rm -f *.a
