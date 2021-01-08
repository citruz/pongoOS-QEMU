#include <pongo.h>

static inline dt_prop_t *skip_props(dt_prop_t *prop, uint32_t nprop) {
    for (uint32_t i = 0; i < nprop; i++) {
        prop = (dt_prop_t *)(prop->val + ((prop->len + 0x3) & ~0x3));
    } 
    return prop;
}

static dt_node_t *skip_node(dt_node_t *cur_node) {

    // skip props
    dt_prop_t *cur_prop = (dt_prop_t *)&cur_node->prop;
    cur_prop = skip_props(cur_prop, cur_node->nprop);

    // skip childs
    uint32_t nchilds = cur_node->nchld;
    cur_node = (dt_node_t *)cur_prop;
    for (uint32_t i = 0; i < nchilds; i++) {
        cur_node = skip_node(cur_node);
    }

    return cur_node;
}

// create property and set size, but don't copy bytes
dt_prop_t *dt_add_prop_empty(dt_node_t *node, char *name, uint32_t len, void *end) {
    if (strlen(name) >= DT_KEY_LEN) {
        panic("name \"%s\" too long", name);
    }

    dt_prop_t *cur_prop = (dt_prop_t *)&node->prop;
    // skip existing props
    cur_prop = skip_props(cur_prop, node->nprop);

    // bounds check
    if (((uint64_t)cur_prop + sizeof(dt_prop_t) + len) > (uint64_t)end) {
        panic("device tree overflow in %s", __func__);
    }

    strncpy(cur_prop->key, name, DT_KEY_LEN);
    cur_prop->key[DT_KEY_LEN - 1] = '\0';
    cur_prop->len = len;

    node->nprop += 1; 

    return cur_prop;
}

// this will overwrite any children. add props first, then children
bool dt_add_prop(dt_node_t *node, char *name, const void *val, uint32_t len, void *end) {
    dt_prop_t *new_prop = dt_add_prop_empty(node, name, len, end);

    if (!new_prop) {
        return false;
    }

    memcpy(new_prop->val, val, len);

    return true;
}

bool dt_add_string_prop(dt_node_t *node, char *name, char *val, void *end) {
    return dt_add_prop(node, name, val, strlen(val) + 1, end);
}

bool dt_add_u32_prop(dt_node_t *node, char *name, uint32_t val, void *end) {
    return dt_add_prop(node, name, &val, sizeof(val), end);
}

bool dt_add_u64_prop(dt_node_t *node, char *name, uint64_t val, void *end) {
    return dt_add_prop(node, name, &val, sizeof(val), end);
}

bool dt_add_child(dt_node_t *node, char *name, dt_node_t **store_node, void *end) {
    // determine the end of the node
    dt_node_t *new_node = skip_node(node);

    // bounds check
    if ((uint64_t)new_node + sizeof(dt_node_t) > (uint64_t)end) {
        panic("device tree overflow in %s", __func__);
    }
    new_node->nprop = 0;
    new_node->nprop = 0;
    node->nchld += 1;

    if (!dt_add_string_prop(new_node, "name", name, end)) {
        return false;
    }

    *store_node = new_node;
    return true;
}