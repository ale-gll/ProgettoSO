#include <stdlib.h>
#include "shared.h"

void set_message(Message *m, int msg_type, Object obj, int *stream_index, int *stream_objs_index) {
    m->msg_type = msg_type;
    m->obj = obj;
    m->stream_index = (stream_index == NULL) ? -1 : *stream_index;
    m->stream_objs_index = (stream_objs_index == NULL) ? -1 : *stream_objs_index;
}
