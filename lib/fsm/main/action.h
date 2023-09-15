#ifndef ACTION_H_
#define ACTION_H_

#include "cppQueue.h"
#include "types.h"
#include <Arduino.h>
#include "constants.h"

namespace action
{
    inline cppQueue queue (sizeof (ActData), constants::global::queueCnt);

    inline void push(ActType type, bool forceExcu = false) {
        ActData act;
        act.type = type;
        act.forceExcu = forceExcu;
        
        queue.push(&act);
    }

    inline ActData pop() {
        ActData act;
        queue.pop(&act);

        return act;
    }
}

#endif // ACTIONS_H_
