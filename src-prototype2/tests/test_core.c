#include "test.h"
#include "../core/ship.h"
#include "../core/calendar_engine.h"

// Test suite for ship module
int test_ship_data()
{
    ShipData *ship0 = getShipData(0);
    ASSERT_STR_EQ(ship0->name, "dromon");
    ASSERT_EQ(ship0->speed, 5);
    ASSERT_EQ(ship0->people_capacity, 150);
    
    ShipData *ship7 = getShipData(7);
    ASSERT_STR_EQ(ship7->name, "genoese");
    ASSERT_EQ(ship7->speed, 12);
    ASSERT_EQ(ship7->acceleration, 4);
    
    return 1;  // Success
}

// Test suite for calendar module
int test_calendar_names()
{
    const char *haab = calendar_get_haab_name(0);
    ASSERT_STR_EQ(haab, "  pop");
    
    const char *tzol = calendar_get_tzolkin_name(0);
    ASSERT_STR_EQ(tzol, "ha'     ");
    
    return 1;
}

int test_calendar_haab()
{
    CalendarHaab haab;
    calendar_get_haab(&haab);
    
    // Just verify the date is reasonable (0-based)
    ASSERT_NEQ(haab.trecena, 255);  // Should be valid
    
    return 1;
}

// Main test runner
int main()
{
    int passed = 0, failed = 0;
    
    TEST_START();
    
    if (test_ship_data()) {
        printf("✓ test_ship_data passed\n");
        passed++;
    } else {
        failed++;
    }
    
    if (test_calendar_names()) {
        printf("✓ test_calendar_names passed\n");
        passed++;
    } else {
        failed++;
    }
    
    if (test_calendar_haab()) {
        printf("✓ test_calendar_haab passed\n");
        passed++;
    } else {
        failed++;
    }
    
    printf("\n%d passed, %d failed\n", passed, failed);
    
    return (failed == 0) ? 0 : 1;
}
