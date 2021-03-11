
all: label.exe

UPX=upx --8086
# if you don't want to use UPX set
#     UPX=-rem (DOS)
#   or
#     UPX=true (Unix)
# if you use UPX: then options are
#     --8086 for 8086 compatibility
#   or
#     --best for smallest

ifneq ($(findstring NOCATS,$(CFLAGS)),)
	KITTEN:=
else
	KITTEN:=../kitten/kitten.c
endif

label.exe _label.exe: prf.c label.c $(KITTEN)
	$(CC) $(CFLAGS)$@ $^ $(LDFLAGS)
	$(UPX) $@

clean:
	$(RM) label.exe
	$(RM) _label.exe
	$(RM) *.o
	$(RM) *.obj
