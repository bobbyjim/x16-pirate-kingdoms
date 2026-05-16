#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "core/ship.h"
#include "core/calendar_engine.h"
#include "adapters/debug/debug_adapter.h"

// Simple REPL for testing core logic with debug adapter

int main()
{
    char cmd[256];
    
    debug_init();
    
    printf("\nType 'help' for commands, 'quit' to exit.\n\n");
    
    while (1) {
        printf("> ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        
        // Remove newline
        cmd[strcspn(cmd, "\n")] = 0;
        
        if (strlen(cmd) == 0) continue;
        
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        
        if (strcmp(cmd, "help") == 0) {
            printf("\nAvailable commands:\n");
            printf("  ship <0-7>       - Show ship data\n");
            printf("  calendar         - Show calendar\n");
            printf("  map              - Show map state\n");
            printf("  mapregion <dim>  - Show map region (default 50)\n");
            printf("  mapfull          - Show full 256x256 map (64x64 display)\n");
            printf("  quit/exit        - Exit\n\n");
            continue;
        }
        
        debug_input_command(cmd);
    }
    
    return 0;
}
