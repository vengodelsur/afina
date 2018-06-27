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

void Engine::yield() {}

void Engine::sched(void *routine_) {}

}  // namespace Coroutine
}  // namespace Afina
