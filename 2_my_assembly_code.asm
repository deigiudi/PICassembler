	__CONFIG 0x3f18

	bsf 0x2,.7
	comf .1,.1
	iorlw 0x80
	sleep
	movf 0x51
    END
