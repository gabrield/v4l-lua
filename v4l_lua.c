/*
 Copyright (c) 2018 Francisco Castro <fcr@adinet.com.uy>
 Copyright (c) 2011 Gabriel Duarte <gabrield@impa.br>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* Lua herders */
#include "lua.h"
#include "lauxlib.h"
#include "luaconf.h"
#include "lualib.h"
/* V4L2 */
#include "core.h"

#define V4L_MT "v4l-lua device"

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
/* Compatibility for Lua 5.1.
 *
 * luaL_setfuncs() is used to create a module table where the functions have
 * json_config_t as their first upvalue. Code borrowed from Lua 5.2 source. */
static void luaL_setfuncs (lua_State *l, const luaL_Reg *reg, int nup) {
    int i;

    luaL_checkstack(l, nup, "too many upvalues");
    for (; reg->name != NULL; reg++) {  /* fill the table with given functions */
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(l, -nup);
        lua_pushcclosure(l, reg->func, nup);  /* closure with those upvalues */
        lua_setfield(l, -(nup + 2), reg->name);
    }
    lua_pop(l, nup);  /* remove upvalues */
}
#endif

static struct device * device_check(lua_State * L, int narg) {
    struct device * dev = luaL_checkudata(L, narg, V4L_MT);
    if (dev->fd < 0)
        luaL_error(L, "device already closed");
    return dev;
}

static int v4l_device_getframe(lua_State * L) {
    struct device * dev = device_check(L, 1);
    int i, imgsize = dev->w * dev->h * 3;
    unsigned char * img;

    img = newframe(dev);

    lua_createtable(L, imgsize, 0);

    for (i = 0; i < imgsize; ++i) {
        lua_pushnumber(L, img[i]);
        lua_rawseti(L, -2, i+1);
    }

    return 1;
}

static int v4l_device_getframestr(lua_State * L) {
    struct device * dev = device_check(L, 1);
    lua_pushlstring(L, (char*)newframe(dev), dev->w * dev->h * 3);
    return 1;
}

static int v4l_device_close(lua_State * L) {
    close_device(device_check(L, 1));
    return 0;
}

static int v4l_device_width(lua_State * L) {
    lua_pushnumber(L, device_check(L, 1)->w);
    return 1;
}

static int v4l_device_height(lua_State * L) {
    lua_pushnumber(L, device_check(L, 1)->h);
    return 1;
}

static int v4l_device_fd(lua_State * L) {
    lua_pushnumber(L, device_check(L, 1)->fd);
    return 1;
}

static int v4l_device___gc(lua_State * L) {
    close_device(luaL_checkudata(L, 1, V4L_MT));
    return 0;
}

static void errorfn_handler(void * errorfn_data, char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    lua_pushvfstring((lua_State *)errorfn_data, fmt, args);
    va_end(args);
    lua_error((lua_State *)errorfn_data);
}

static int v4l_open(lua_State * L) {
    const char * path;
    struct device * dev = lua_newuserdata(L, sizeof (struct device));
    int w, h;

    path = luaL_checkstring(L, 1);
    w = luaL_optinteger(L, 2, 720);
    h = luaL_optinteger(L, 3, 480);

    if (open_device(dev, path, L, errorfn_handler) < 0)
        return luaL_error(L, "device error");

    luaL_getmetatable(L, V4L_MT);
    lua_setmetatable(L, -2);

    init_device(dev, w, h);
    start_capturing(dev);
    return 1;
}

int LUA_API luaopen_v4l(lua_State *L) {
    const luaL_Reg driver[] = {
        {"open", v4l_open},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, V4L_MT)) {
        const luaL_Reg methods[] = {
            {"getframe", v4l_device_getframe},
            {"getframestr", v4l_device_getframestr},
            {"close", v4l_device_close},
            {"width", v4l_device_width},
            {"height", v4l_device_height},
            {"fd", v4l_device_fd},
            {"__gc", v4l_device___gc},
            {NULL, NULL},
        };
        luaL_setfuncs(L, methods, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1); /* discard metatable */

    lua_createtable(L, sizeof driver / sizeof driver[0], 0);
    luaL_setfuncs(L, driver, 0);

    return 1;
}
/* vi: set et sw=4: */
