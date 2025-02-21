square:
        addi    sp,sp,-32
        sd      ra,24(sp)
        sd      s0,16(sp)
        addi    s0,sp,32
        mv      a5,a0
        sw      a5,-20(s0)
        lw      a5,-20(s0)
        mulw    a5,a5,a5
        sext.w  a5,a5
        mv      a0,a5
        ld      ra,24(sp)
        ld      s0,16(sp)
        addi    sp,sp,32
        jr      ra
