//
//  Copyright (C) 2014  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "object.h"

#include <string.h>

DEFINE_ARRAY(tree);
DEFINE_ARRAY(netid);
DEFINE_ARRAY(type);
DEFINE_ARRAY(range);

static const char *item_text_map[] = {
   "I_IDENT",    "I_VALUE",      "I_SEVERITY", "I_MESSAGE", "I_TARGET",
   "I_LITERAL",  "I_IDENT2",     "I_DECLS",    "I_STMTS",   "I_PORTS",
   "I_GENERICS", "I_PARAMS",     "I_GENMAPS",  "I_WAVES",   "I_CONDS",
   "I_TYPE",     "I_SUBKIND",    "I_DELAY",    "I_REJECT",  "I_POS",
   "I_REF",      "I_FILE_MODE",  "I_ASSOCS",   "I_CONTEXT", "I_TRIGGERS",
   "I_ELSES",    "I_CLASS",      "I_RANGE",    "I_NAME",    "I_NETS",
   "I_OPS",      "I_CONSTR",     "I_BASE",     "I_ELEM",    "I_FILE",
   "I_ACCESS",   "I_RESOLUTION", "I_RESULT",   "I_UNITS",   "I_LITERALS",
   "I_DIMS",     "I_FIELDS",     "I_TEXT_BUF"
};

void object_lookup_failed(const char *name, const char **kind_text_map,
                          int kind, imask_t mask)
{
   int item;
   for (item = 0; (mask & (1 << item)) == 0; item++)
      ;

   assert(item < ARRAY_LEN(item_text_map));
   fatal_trace("%s kind %s does not have item %s", name,
               kind_text_map[kind], item_text_map[item]);
}

void item_without_type(imask_t mask)
{
   int item;
   for (item = 0; (mask & (1 << item)) == 0; item++)
      ;

   assert(item < ARRAY_LEN(item_text_map));
   fatal_trace("item %s does not have a type", item_text_map[item]);
}

uint32_t object_index(const object_t *object)
{
   assert(object != NULL);
   assert(object->index != UINT32_MAX);

   return object->index;
}

void object_change_kind(const object_class_t *class, object_t *object, int kind)
{
   if (kind == object->kind)
      return;

   bool allow = false;
   for (size_t i = 0; (class->change_allowed[i][0] != -1) && !allow; i++) {
      allow = (class->change_allowed[i][0] == object->kind)
         && (class->change_allowed[i][1] == kind);
   }

   if (!allow)
      fatal_trace("cannot change %s kind %s to %s", class->name,
                  class->kind_text_map[object->kind],
                  class->kind_text_map[kind]);

   const imask_t old_has = class->has_map[object->kind];
   const imask_t new_has = class->has_map[kind];

   const int old_nitems = class->object_nitems[object->kind];
   const int new_nitems = class->object_nitems[kind];

   const int max_items = MAX(old_nitems, new_nitems);

   item_t tmp[max_items];
   memcpy(tmp, object->items, sizeof(item_t) * max_items);

   int op = 0, np = 0;
   for (imask_t mask = 1; np < new_nitems; mask <<= 1) {
      if ((old_has & mask) && (new_has & mask))
         object->items[np++] = tmp[op++];
      else if (old_has & mask)
         ++op;
      else if (new_has & mask)
         memset(&(object->items[np++]), '\0', sizeof(item_t));
   }

   object->kind = kind;
}