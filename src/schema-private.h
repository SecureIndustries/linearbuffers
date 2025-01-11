
#include <stdint.h>
#include "queue.h"

TAILQ_HEAD(schema_attributes, schema_attribute);
struct schema_attribute {
        TAILQ_ENTRY(schema_attribute) list;
        char *name;
        char *value;
};

TAILQ_HEAD(schema_enum_fields, schema_enum_field);
struct schema_enum_field {
        TAILQ_ENTRY(schema_enum_field) list;
        char *name;
        char *value;
        struct schema_attributes attributes;
};

TAILQ_HEAD(schema_enums, schema_enum);
struct schema_enum {
        TAILQ_ENTRY(schema_enum) list;
        char *name;
        char *type;
        char *Type;
        char *TYPE;
        struct schema_enum_fields fields;
        struct schema_attributes attributes;
};

TAILQ_HEAD(schema_table_fields, schema_table_field);
struct schema_table_field {
        TAILQ_ENTRY(schema_table_field) list;
        char *name;
        char *type;
        char *Type;
        char *value;
        uint32_t container;
        struct schema_attributes attributes;
};

TAILQ_HEAD(schema_tables, schema_table);
struct schema_table {
        TAILQ_ENTRY(schema_table) list;
        char *name;
        uint32_t type;
        uint64_t nfields;
        struct schema_table_fields fields;
        struct schema_attributes attributes;
};

struct schema {
        char *namespace;
        uint32_t count_type;
        uint32_t offset_type;
        char *NAMESPACE;
        struct schema_enums enums;
        struct schema_tables tables;
        struct schema_attributes attributes;
};
