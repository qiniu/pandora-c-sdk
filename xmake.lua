-- project
set_project("pandora-curl")

-- version
set_version("0.0.1")

cpputest_home = "/usr/local/"

-- set warning all as error
set_warnings("all", "error")

-- set language: c99
set_languages("c99")

-- the debug or check or coverage mode
if is_mode("debug", "check", "coverage") then

    -- enable the debug symbols
    set_symbols("debug")

    -- disable optimization
    set_optimize("none")

    -- add defines for debug
    add_defines("__tb_debug__")

    -- attempt to enable some checkers for pc
    if is_mode("check") and is_arch("i386", "x86_64") then
        add_cxflags("-fsanitize=address", "-ftrapv")
        add_mxflags("-fsanitize=address", "-ftrapv")
        add_ldflags("-fsanitize=address")
    end

    -- enable coverage
    if is_mode("coverage") then
        add_cxflags("--coverage")
        add_cxflags("-ftest-coverage")
        add_mxflags("--coverage")
        add_ldflags("--coverage")
    end
end

-- the release mode
if is_mode("release") then

    -- set the symbols visibility: hidden
    set_symbols("hidden")

    -- enable fastest optimization
    set_optimize("fastest")

    -- strip all symbols
    set_strip("all")
end

-- add option: CppUTest
option("CppUTest")
    set_default(false)
    set_showmenu(true)
    set_category("option")
    set_description("Specify CppUTest Home")

-- add target
target("run_all_tests")

    -- set kind
    set_kind("binary")

    -- add define for getaddrinfo
    if is_os("linux") then
        add_defines("_POSIX_C_SOURCE=200112L")
    end
    -- add include directories
    add_includedirs("include", "mocks", cpputest_home .. "include")

    -- add files
    add_files("source/**.c", "test/**.cpp")

    -- add link library
    add_links("CppUTest")
    add_links("curl")

    -- add link directories
    add_linkdirs(cpputest_home .. "lib")

-- add target
target("pandora-curl")

    -- set kind
    set_kind("static")

    -- add define for getaddrinfo
    if is_os("linux") then
        add_defines("_POSIX_C_SOURCE=200112L")
    end

    -- add include directories
    add_includedirs("include", "mocks", cpputest_home .. "include")

    -- add files
    add_files("source/**.c")
