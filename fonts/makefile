FONTPACKNAME?=Gohufont

all: $(FONTPACKNAME).8xv

$(FONTPACKNAME).bin: gohufont-11b.fnt gohufont-11.fnt
	convfont -o fontpack -N "Gohufont" -P "ASCII" \
	-D "A small monospace font" \
	-V "4 August 2019" \
	-f gohufont-11b.fnt -l 9 -x 5 -c 2 -a 1 -b 1 -w bold -s sans-serif -s upright -s monospaced \
	-f gohufont-11.fnt  -l 9 -x 5 -c 2 -a 1 -b 1 -w normal -s sans-serif -s upright -s monospaced \
	$(FONTPACKNAME).bin

$(FONTPACKNAME).8xv: $(FONTPACKNAME).bin
	convhex -a -v -n $(FONTPACKNAME) $(FONTPACKNAME).bin $(FONTPACKNAME).8xv

clean:
	rm Gohufont.8xv
	rm Gohufont.bin

.PHONY: clean