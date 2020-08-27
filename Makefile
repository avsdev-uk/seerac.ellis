
DST = seerac-ellis

SRC_DIR = src
BUILD_DIR = build

R ?= /usr/bin/R

LDFLAGS ?= -std=gnu99 -Wl,-z,relro
CFLAGS ?= -std=gnu99 -DNDEBUG -DWITHOUT_R -fpic -g -O2 -Wall -pedantic -fstack-protector-strong -D_FORTIFY_SOURCE=2 

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CUSRC = $(wildcard $(SRC_DIR)/*.cu)

DIR := basename `pwd`


$(DST): $(BUILD_DIR) cuda $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(BUILD_DIR)/$(DST)_cuda.a -lcudart; \
	cp -f $(DST) $(SRC_DIR)/ellis

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

cuda:
	nvcc -Xcompiler -fPIC -arch=sm_35 -lib $(CUSRC) -o $(BUILD_DIR)/$(DST)_cuda.a

.PHONY: clean
clean:
	rm -fR build src/ellis

.PHONY: Rpackage
Rpackage:
	$(R) --vanilla -q -e "devtools::build(binary = T)"

.PHONY: Rsrcpackage
Rsrcpackage:
	$(R) --vanilla -q -e "devtools::build(binary = F)"

.PHONY: Rinstall
Rinstall:
	$(R) --vanilla -q -e "devtools::install()"