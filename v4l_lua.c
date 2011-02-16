/*
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
#include <lua.h>
#include <lauxlib.h>
#include <luaconf.h>
#include <lualib.h>
/* V4L2 */
#include "core.h"



typedef unsigned char uint8;
                 
static int w(lua_State *L)
{
    lua_pushnumber(L, getwidth());
    return 1;
}

static int h(lua_State *L)
{
    lua_pushnumber(L, getheight());
    return 1;
}
                    
static int opencamera(lua_State *L)
{
    int fd = -1;
    const char *device;
     
    if(!lua_gettop(L))
        return luaL_error(L, "set a device");

    device = luaL_checkstring(L, 1);
    fd = open_device(device);

	if(fd > 0)
    {
        init_device();
        start_capturing();
        lua_pushinteger(L, fd);
    }
    else
    {
		printf("device error\n");
        return luaL_error(L, "device error");
    }

    return 1;
}

static int closecamera(lua_State *L)
{
    int fd = -1;
    int dev = 0;

    if(!lua_gettop(L))
        return luaL_error(L, "set a device");

    fd = luaL_checkinteger(L, 1);
    
    uninit_device();
    dev = close_device(fd);
    lua_pushinteger(L, dev);
    
    return 1;
}

int save_pnm(unsigned char *buf, int x, int y, int depth)
{
    FILE *fp;
    char bewf[128];
    //buf = (unsigned char *)malloc(sizeof(x*y*depth));
    snprintf(bewf ,sizeof(bewf), "capture.pnm");
    /*printf("Allocating %dx%dx%d = %d of memory\n", x, y, depth, x*y*depth);*/
    printf("%d %d %d\n\n", buf[0], buf[1], buf[2]);
    if ((fp = fopen(bewf, "w+")) == NULL) {
        perror("open");
        exit(1);
    }

    if (depth == 3) {
        fprintf(fp, "P6\n%d %d\n255\n", x, y);
    }

    if (depth == 1) {
        fprintf(fp, "P5\n%d %d\n255\n", x, y);
    }

    fwrite((unsigned char*)buf, x * y * depth, 1, fp);
    fclose(fp);
    return 0;
}



static int get(lua_State *L)
{
   
    int IMGSIZE = (getwidth()*getheight()*3);
    uint8 *img;
    int i;
    img = (uint8*)malloc(sizeof(uint8)*(IMGSIZE));

    img = newframe();

    lua_createtable(L, IMGSIZE, 0);
    
    for(i = 0; i < IMGSIZE; ++i)
    {
        lua_pushnumber(L, img[i]);
        lua_rawseti(L, -2, i);
    }
    
   
    free(img); 
               
    return 1;
}

int LUA_API luaopen_v4l(lua_State *L)
{
    const luaL_Reg driver[] = 
    {
        {"open", opencamera},
        {"close", closecamera},
        {"widht", w},
        {"height", h},
        {"getframe", get},
        {NULL, NULL},
    };
    
    luaL_openlib (L, "v4l", driver, 0);
    lua_settable(L, -1);
    
    return 1;
}
