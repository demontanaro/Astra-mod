/*
 * Astra Module: Transmit
 * http://cesbo.com/astra
 *
 * Copyright (C) 2012-2014, Andrey Dyldin <and@cesbo.com>
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
 *      transmit
 *
 * Module Options:
 *      upstream    - object, stream instance returned by module_instance:stream()
 *
 * Module Methods:
 *      set_upstream(object)
 *                  - set upstream module instance
 */

#include <astra.h>
#include <luaapi/stream.h>

struct module_data_t
{
    MODULE_STREAM_DATA();
};

static int method_set_upstream(lua_State *L, module_data_t *mod)
{
    if(lua_type(L, 2) == LUA_TLIGHTUSERDATA)
    {
        module_stream_t *const st = (module_stream_t *)lua_touserdata(L, 2);
        __module_stream_attach(st, &mod->__stream);
    }

    return 0;
}

static void on_ts(module_data_t *mod, const uint8_t *ts)
{
    module_stream_send(mod, ts);
}

static void module_init(lua_State *L, module_data_t *mod)
{
    __uarg(L);

    module_stream_init(mod, on_ts);
}

static void module_destroy(module_data_t *mod)
{
    module_stream_destroy(mod);
}

MODULE_STREAM_METHODS()
MODULE_LUA_METHODS()
{
    MODULE_STREAM_METHODS_REF(),
    { "set_upstream", method_set_upstream },
};
MODULE_LUA_REGISTER(transmit)
