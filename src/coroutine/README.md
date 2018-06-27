## Memo for the newbie author

Just read this (in Russian):

Coroutines [Wikipedia](https://ru.wikipedia.org/wiki/%D0%A1%D0%BE%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D0%B0)

Coroutines [Stackoverflow](https://ru.stackoverflow.com/questions/496002/%D0%A1%D0%BE%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D1%8B-%D0%BA%D0%BE%D1%80%D1%83%D1%82%D0%B8%D0%BD%D1%8B-coroutine-%D1%87%D1%82%D0%BE-%D1%8D%D1%82%D0%BE)

Cooperative multitasking [Wikipedia](https://ru.wikipedia.org/wiki/%D0%A1%D0%BE%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D0%B0)

setjmp & longjmp [Stackoverflow](https://ru.stackoverflow.com/questions/592529/%D0%97%D0%B0%D1%87%D0%B5%D0%BC-%D0%BD%D1%83%D0%B6%D0%BD%D1%8B-%D1%84%D1%83%D0%BD%D0%BA%D1%86%D0%B8%D0%B8-setjmp-longjmp)

setjmp [cppreference](https://ru.cppreference.com/w/cpp/utility/program/setjmp)

longjmp [cppreference](https://ru.cppreference.com/w/cpp/utility/program/longjmp)


The only information about jmpbuf you're going to find is "erm, it's an array type storing the information of a calling environment. Filled by setjmp and restored by longjmp".

