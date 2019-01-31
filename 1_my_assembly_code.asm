	__CONFIG 0x3f18

	andwf 0x70,.1
	clrw
	clrf .8
	nop
	btfsc 0x12,0x3
	addlw .13
	goto .2
	retfie
	return
    END
