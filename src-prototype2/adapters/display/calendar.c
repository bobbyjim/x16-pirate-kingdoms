#include <conio.h>
#include <stdio.h>
#include "../../core/calendar_engine.h"

// Display adapter: renders calendar to X16 screen

static char todaysDate[20];

void calendar_init()
{
    // Initialize if needed
}

void calendar_tick()
{
    // Called each frame to update calendar state
    // (Note: in a real app, this might be triggered by timer interrupt)
}

void calendar_display()
{
    CalendarDate date;
    calendar_get_long_count(&date);
    
    // Format and display using conio
    sprintf(todaysDate, "%02d.%02d.%02d.%02d", 
        date.kin,
        date.winal,
        date.katun,
        date.baktun);
    
    cputsxy(20, 52, todaysDate);
}
