/*
 * Astra Utils (Timer)
 * http://cesbo.com/astra
 *
 * Copyright (C) 2012-2015, Andrey Dyldin <and@cesbo.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Module Name:
 *      timer
 *
 * Module Options:
 *      interval    - number, sets the interval between triggers, in seconds
 *      callback    - function, handler is called when the timer is triggered
 */

#include <astra.h>
#include <core/timer.h>
#include <luaapi/luaapi.h>

struct module_data_t
{
    MODULE_LUA_DATA();

    int idx_self;
    int idx_callback;

    asc_timer_t *timer;
};

static void timer_callback(void *arg)
{
    module_data_t *const mod = (module_data_t *)arg;
    lua_State *const L = MODULE_L(mod);

    lua_rawgeti(L, LUA_REGISTRYINDEX, mod->idx_callback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, mod->idx_self);
    lua_call(L, 1, 0);
}

static int method_close(lua_State *L, module_data_t *mod)
{
    if(mod->idx_callback)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, mod->idx_callback);
        mod->idx_callback = 0;
    }

    if(mod->idx_self)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, mod->idx_self);
        mod->idx_self = 0;
    }

    ASC_FREE(mod->timer, asc_timer_destroy);

    return 0;
}

static void module_init(lua_State *L, module_data_t *mod)
{
    int interval = 0;
    module_option_integer(L, "interval", &interval);
    asc_assert(interval > 0, "[timer] option 'interval' must be greater than 0");

    lua_getfield(L, MODULE_OPTIONS_IDX, "callback");
    asc_assert(lua_isfunction(L, -1), "[timer] option 'callback' is required");
    mod->idx_callback = luaL_ref(L, LUA_REGISTRYINDEX);

    // store self in registry
    lua_pushvalue(L, 3);
    mod->idx_self = luaL_ref(L, LUA_REGISTRYINDEX);

    mod->timer = asc_timer_init(interval * 1000, timer_callback, mod);
}

static void module_destroy(module_data_t *mod)
{
    if(mod->idx_self)
        method_close(MODULE_L(mod), mod);
}

MODULE_LUA_METHODS()
{
    { "close", method_close },
};
MODULE_LUA_REGISTER(timer)
