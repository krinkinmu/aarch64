/*
 * C++ compilers sometimes generate calls to C++ runtime functions like
 * "__cxa_atexit". Such functions are not part of the C++ standard, but instead
 * they belong to a C++ ABI.
 *
 * Since we are not using standard compiler implementation of C++ runtime we
 * have to provide all the required functions outselves.
 */
typedef void (*ctor_t)(void);


/*
 * __cxa_atexit is part of the C++ ABI. It registers a destructor that should
 * be called when program exits. For example, when you have a static object
 * with a constructor its constructor should be called before the "main" and
 * its destructor should be called at the program exit. So what compilers can
 * do is to generate a code that will construct the static object and then
 * register a destructor using __cxa_atexit.
 *
 * At the moment we do not care about destroying anything when the kernel
 * exits, so the current implementation is doing nothing.
 */
extern "C" int __cxa_atexit(void (*destroy)(void*), void* arg, void* dso) {
    return 0;
}

/*
 * __constructors is not defined in any ABI I know of, it's just a helper
 * function that bootstrap code can call to run constructors of the static
 * objects.
 */
extern "C" void __constructors(ctor_t *from, ctor_t *to)
{
    for (ctor_t *ctor = from; ctor != to; ++ctor) {
        (*ctor)();
    }
}
