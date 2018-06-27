#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

// Assumption: stack grows down (praises to x86)

namespace Afina {
namespace Coroutine {

// current coroutine uses context to save stack
void Engine::Store(context &ctx) {
    char stack_cursor;
    ctx.Low = &stack_cursor;  // Low is coroutine stack start address as stored
                              // in context
    ctx.Hight = StackBottom;  // Hight is coroutine stack end address as stored
                              // in context, StackBottom is the same for Engine
                              // member (it's where functions turn cooperative)
    uint32_t stack_size = ctx.Hight - ctx.Low;
    if (stack_size > std::get<1>(ctx.Stack)) {
        ctx.ExtendStack(stack_size);  // See include/afina/coroutine/Engine.h
    }
    memcpy(std::get<0>(ctx.Stack), ctx.Low,
           stack_size);  // Stack is stored as tuple of start address and size
}

// get stack by context and grant control to coroutine
void Engine::Restore(context &ctx) {
    char stack_cursor;
    if (&stack_cursor >= ctx.Low) {
        Restore(ctx);
    }

    char *stack_buffer = std::get<0>(ctx.Stack);
    uint32_t stack_size = std::get<1>(ctx.Stack);

    memcpy(ctx.Low, stack_buffer, stack_size);
    longjmp(ctx.Environment, 1);  // loads context saved by latest setjmp;
                                  // status 1 will be returned by setjmp
}

// find non-active routine and run it
void Engine::yield() {
    context *ready_routine =
        alive;  // alive is a list of routines ready to be scheduled
    while (ready_routine && ready_routine == cur_routine) {
        ready_routine = ready_routine->next;
    }  // routine to be run should differ from current one
    if (ready_routine != nullptr) {
        sched(ready_routine);
    }
    return;
}

// stop current routine and run the one given as argument
void Engine::sched(void *routine_) {
    if (routine_ == nullptr or routine_ == cur_routine) {
        return;
    }

    if (cur_routine != nullptr) {
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }  // only coroutine can call sched
        Store(*cur_routine);
    }

    cur_routine = reinterpret_cast<context *>(routine_);
    Restore(*cur_routine);
}

}  // namespace Coroutine
}  // namespace Afina
