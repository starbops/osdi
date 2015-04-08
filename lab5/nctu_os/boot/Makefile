#Its for create a simple bootloader 

BOOT_OBJS = boot/boot.o boot/main.o

boot/%.o: boot/%.c
	$(CC) $(CFLAGS) -Os -c -o $@ $<

boot/%.o: boot/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

boot/boot: $(BOOT_OBJS)
	@echo + ld boot/boot
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@.out $^
	$(OBJDUMP) -S $@.out >$@.asm
	$(OBJCOPY) -S -O binary -j .text $@.out $@
	perl boot/sign.pl $(OBJDIR)/boot/boot
