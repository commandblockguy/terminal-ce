#ifndef H_SERIAL
#define H_SERIAL

#include <stdbool.h>

#include "terminal.h"

bool init_serial(struct terminal_state *term);
void process_serial(struct terminal_state *term);

#endif //H_SERIAL
