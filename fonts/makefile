all: Gohufont.8xv TomThumb.8xv

Gohufont.bin: gohufont-11b.fnt gohufont-11.fnt
	convfont -o fontpack -N "Gohufont" -P "ASCII" \
	-D "A small monospace font" \
	-V "4 August 2019" \
	-f gohufont-11b.fnt -l 9 -x 5 -c 2 -a 1 -b 1 -w bold -s sans-serif -s upright -s monospaced \
	-f gohufont-11.fnt  -l 9 -x 5 -c 2 -a 1 -b 1 -w normal -s sans-serif -s upright -s monospaced \
	Gohufont.bin

TomThumb.bin: tomthumb4x6-6.fnt
	convfont -o fontpack -N "TomThumb" -P "ASCII" \
	-D "A tiny monospace font" \
	-V "13 September 2019" \
	-f tomthumb4x6-6.fnt -l 5 -x 2 -c 1 -a 1 -b 1 -w normal -s sans-serif -s upright -s monospaced \
	TomThumb.bin

%.8xv: %.bin
	convhex -a -v -n $* $< $@

clean:
	rm -f $(wildcard *.8xv)
	rm -f $(wildcard *.bin)

.PHONY: clean