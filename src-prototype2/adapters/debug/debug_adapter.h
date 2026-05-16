#ifndef _DEBUG_ADAPTER_H_
#define _DEBUG_ADAPTER_H_

// Debug adapter for local testing and development
// Replaces UI/display code with simple text I/O

void debug_init();
void debug_print_ship_state(int ship_index);
void debug_print_calendar_state();
void debug_print_map_state();
void debug_print_map_region(int dimension);
void debug_print_map_full(void);
void debug_input_command(char *cmd);

#endif
