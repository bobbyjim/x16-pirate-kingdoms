#include <conio.h>

#include "../../present/gfx.h"
#include "../../present/sound.h"

int main(void)
{
    gfx_init();
    sound_init();

    clrscr();
    cputsxy(2, 2, "Pirate Kingdoms X16 frontend stub");
    cputsxy(2, 4, "Expected boot flow:");
    cputsxy(4, 5, "1. BASIC loader shows splash screen");
    cputsxy(4, 6, "2. map preloaded into banked RAM");
    cputsxy(4, 7, "3. PKX16 starts with that contract");
    cputsxy(2, 9, "See platform/x16/BOOT.BAS for base bank/address.");
    cputsxy(2, 11, "Press any key to return.");

    cgetc();
    return 0;
}