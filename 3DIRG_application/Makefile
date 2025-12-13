# MIT License

# Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

#######################################################
#directory stuffs
TOP ?= $(shell pwd)
EXECUTABLE ?= trilli_app

SRC_DIR=$(TOP)/src
SW_DIR=$(SRC_DIR)/sw

#######################################################

#######################################################
#sw stuffs
CC:=g++ 
#Add -DHW_REG to enable the HW registration
INCL_DRVR = -I$(DRVR_DIR) -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include
CFLAGS=--std=c++17
CXXFLAGS = -std=c++17 -Wno-deprecated-declarations
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include -I$(SW_DIR)/include -I$(SW_DIR)/include/image_utils
LDFLAGS := -L$(XILINX_XRT)/lib
LDFLAGS += $(LDFLAGS) -lxrt_coreutil
LIBS = `pkg-config --libs opencv`
CFLAGS = `pkg-config --cflags opencv`

OPENCVLIB=`pkg-config --cflags --libs opencv`
PKG_CONFIG_PATH:='/usr/local/lib64/pkgconfig/':$(PKG_CONFIG_PATH)
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):/usr/local/lib64
core_code := $(wildcard $(SW_DIR)/*.hpp)
core_code += $(wildcard $(SW_DIR)/*.cpp)

MAIN_CODE=$(SW_DIR)/main.cpp 
MAIN_CODE += $(SW_DIR)/include/image_utils/image_utils.cpp

HOST_SRCS := $(SW_DIR)/main.cpp
HOST_SRCS += $(SW_DIR)/include/image_utils/image_utils.cpp
HOST_SRCS += $(SW_DIR)/include/software_mi/software_mi.cpp

build_host: $(EXECUTABLE)
$(EXECUTABLE): $(HOST_SRCS)
	$(CC) -o $(EXECUTABLE) $^ $(CXXFLAGS) -DHW_REG $(LDFLAGS) $(CFLAGS) $(LIBS) -O2

sw_test:$(MAIN_CODE) $(core_code)
	@rm -f ./overlay_hw.xclbin
	@rm -f ./overlay_hw_emu.xclbin
	@ln -s ../hw/overlay_hw.xclbin
	@ln -s ../hw/overlay_hw_emu.xclbin
	$(CC) $(CFLAGS) $(MAIN_CODE) -o $(EXECUTABLE) $(OPENCVLIB)

run_sw_test: sw_test
	./$(EXECUTABLE) ./CT_small/png/ ./PET_small/png/ ./output/ 246

clean:
	rm -f $(EXECUTABLE)
	rm -f ./overlay_hw.xclbin
	rm -f ./overlay_hw_emu.xclbin