.org $bff8
.segment "CODE"

;
; This routine is __SIX__ bytes long.
;
MEMORY_COPY	= $fee7

;
;  This should sit in the same RAM bank as the sprite registers.
;
entry:		   			; JSR $bff8 gets you here.
	wai        			; Wait for vsync
	jmp MEMORY_COPY  	; copy to VERA
	; use MEMORY_COPY's RTS as our RTS.
