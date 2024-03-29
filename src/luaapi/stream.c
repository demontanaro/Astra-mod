/*
 * Astra Lua API (Stream Module)
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

#include <astra.h>
#include <luaapi/stream.h>

static
void stream_detach(module_stream_t *stream, module_stream_t *child)
{
    asc_list_remove_item(stream->children, child);
    child->parent = NULL;
}

void __module_stream_attach(module_stream_t *stream, module_stream_t *child)
{
    if (child->parent != NULL)
        stream_detach(child->parent, child);

    child->parent = stream;
    asc_list_insert_tail(stream->children, child);
}

void __module_stream_send(void *arg, const uint8_t *ts)
{
    module_stream_t *const stream = (module_stream_t *)arg;

    asc_list_for(stream->children)
    {
        module_stream_t *const i =
            (module_stream_t *)asc_list_data(stream->children);

        if (i->on_ts != NULL)
            i->on_ts(i->self, ts);
    }
}

void __module_stream_init(module_stream_t *stream)
{
    stream->children = asc_list_init();
}

void __module_stream_destroy(module_stream_t *stream)
{
    if (stream->parent != NULL)
        stream_detach(stream->parent, stream);

    asc_list_clear(stream->children)
    {
        module_stream_t *const i =
            (module_stream_t *)asc_list_data(stream->children);

        i->parent = NULL;
    }

    ASC_FREE(stream->children, asc_list_destroy);
}
