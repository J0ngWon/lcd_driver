obj-m := lcd.o

# C 코드에 커널 헤더를 사용할 때 필요한 플래그 (C++ 아니라고 가정)
KBUILD_CFLAGS += -Wall -O2

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
