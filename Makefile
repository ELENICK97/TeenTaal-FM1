# Host tools (the same DSP sources as the firmware). The firmware itself: tools/fw.sh (fm1-base's tools/build.py).
#   make            build-host/render and build-host/regress
#   make test       tools/test/run_tests.sh
CC ?= cc
CFLAGS = -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -I. -Iplatform/fm1-base/include
DSP_SRC = $(wildcard dsp/*.c reftone/*.c tanpura/*.c tabla/*.c taal/*.c mixer/*.c)
DSP_HDR = $(wildcard dsp/*.h reftone/*.h tanpura/*.h tabla/*.h taal/*.h mixer/*.h) tools/common/scen.h
OUT = build-host

all: $(OUT)/render $(OUT)/tabla_demo $(OUT)/regress $(OUT)/tanpura_ref $(OUT)/tanpura_var $(OUT)/tstring_dump $(OUT)/tanpura_stress

$(OUT)/render: tools/render/render.c tools/common/scen.c $(DSP_SRC) $(DSP_HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/render/render.c tools/common/scen.c $(DSP_SRC) -lm

$(OUT)/tabla_demo: tools/render/tabla_demo.c tools/common/scen.c $(DSP_SRC) $(DSP_HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/render/tabla_demo.c tools/common/scen.c $(DSP_SRC) -lm

$(OUT)/regress: tools/test/regress.c tools/common/scen.c $(DSP_SRC) $(DSP_HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/test/regress.c tools/common/scen.c $(DSP_SRC) -lm

$(OUT)/tanpura_var: tools/test/tanpura_var.c tools/common/scen.c $(DSP_SRC) $(DSP_HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/test/tanpura_var.c tools/common/scen.c $(DSP_SRC) -lm

$(OUT)/tstring_dump: tools/test/tstring_dump.c tanpura/tstring.c tanpura/tstring.h
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/test/tstring_dump.c tanpura/tstring.c -lm

$(OUT)/tanpura_stress: tools/test/tanpura_stress.c $(DSP_SRC) $(DSP_HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ tools/test/tanpura_stress.c $(DSP_SRC) -lm

# the step 2a reference tanpura (double precision, host only: not firmware code)
$(OUT)/tanpura_ref: tools/reference/tanpura_ref.c
	@mkdir -p $(OUT)
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -o $@ $< -lm

test: all
	$(OUT)/tanpura_ref -selftest
	$(OUT)/tabla_demo -selftest
	~/.venvs/teentaal/bin/python tools/tabla_data/build_samples.py --check
	python3 tools/taal_data/build_taal.py --check
	$(OUT)/tanpura_stress quick
	tools/test/run_tests.sh

clean:
	rm -rf $(OUT)

.PHONY: all test clean
