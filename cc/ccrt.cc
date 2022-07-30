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
 * __cxa_pure_virtual is part of the C++ ABI. In C++ it's possible to create a
 * class that has pure virtual functions. Pure virtual functions don't have to
 * have a definition. So when compiler generates a vtbl for such classes it
 * uses __cxa_pure_virtual as a placeholder.
 *
 * Naturally, calling a pure virtual function is a mistake, but through
 * incorrect use of the C++ language it's still possible to sometimes call a
 * pure virtual function. So implementation should allow us to catch such
 * cases.
 */
extern "C" void __cxa_pure_virtual() {
    while (true);
}

/*
 * TODO: figure out why it's needed and what to do with it.
 *
 * It appears that C++ compiler generates calls to the operator delete for so
 * called deleting destructors. When we define a virtual destructor, compiler
 * treats it somewhat differently from other virtual functions - it actually
 * generates 2 functions: the destructor of the object itself and a deleting
 * destructor.
 *
 * Deleting destructor executes the normal destructor procedure and calls
 * delete operator that typically deallocates memory. One caveat here is that
 * delete operator can be overloaded and the purpose of the deleting destructor
 * is to call the right delete operator in this case.
 */
void operator delete(void*) {}

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
