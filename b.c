/* storage class check */

auto int auto_file_scoped;
register auto int register_auto_file_scoped;
static auto int static_auto_file_scoped;
extern auto int extern_auto_file_scoped;
typedef auto int typedef_auto_file_scoped;

auto int auto_func(
auto int x)
{
    auto int auto_local;
    register auto int register_auto_local;
    static auto int static_auto_local;
    extern auto int extern_auto_local;
    typedef auto int typedef_auto_local;
    return 0;
}

register int register_file_scoped;
static register int static_register_file_scoped;
extern register int extern_register_file_scoped;
typedef register int typedef_register_file_scoped;

register int register_func(
register int x)
{
    register int register_local;
    static register int static_register_local;
    extern register int extern_register_local;
    typedef register int typedef_register_local;
    return 0;
}

static int static_file_scoped;
extern static int extern_static_file_scoped;
typedef static int typedef_static_file_scoped;

static int static_func(
static int x)
{
    static int static_local;
    extern static int extern_static_local;
    typedef static int typedef_static_local;
    return 0;
}

extern int extern_file_scoped;
typedef extern int typedef_extern_file_scoped;

extern int extern_func(
extern int x)
{
    extern int extern_local;
    typedef extern int typedef_extern_local;
    return 0;
}

typedef int typedef_file_scoped;

typedef int typedef_func(
typedef int x);

int func()
{
    typedef int typedef_local;
    return 0;
}

typedef typedef int I;

signed signed int a;
